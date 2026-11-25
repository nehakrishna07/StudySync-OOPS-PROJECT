#include "HttpServer.h"

// Send a full response, even when a socket write sends only part of it.
static bool sendAll(Socket socket, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int count = send(socket, data.data() + sent,
                         (int)std::min<size_t>(data.size() - sent, 16384), 0);
        if (count <= 0) return false;
        sent += count;
    }
    return true;
}

static std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return value;
}

static std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static void response(Socket client, int code, const std::string& body,
                     const std::string& mime) {
    const char* reason = code == 200 ? "OK" :
        code == 400 ? "Bad Request" : code == 401 ? "Unauthorized" :
        code == 403 ? "Forbidden" : code == 404 ? "Not Found" :
        code == 405 ? "Method Not Allowed" : code == 409 ? "Conflict" :
        code == 413 ? "Payload Too Large" : "Internal Server Error";
    const std::string headers =
        "HTTP/1.1 " + std::to_string(code) + " " + reason + "\r\n"
        "Content-Type: " + mime + "\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'self'; style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; font-src 'self' https://fonts.gstatic.com; script-src 'self'; connect-src 'self'\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n";
    sendAll(client, headers + body);
}

struct HttpRequest {
    std::string method;
    std::string path;
    std::string authorization;
    std::string payload;
};

// Return false if the request could not be read; an invalid request receives
// the same status and message as before the refactor.
static bool readRequest(Socket client, HttpRequest& request) {
    std::string raw;
    char buffer[8192];
    size_t separator = std::string::npos;
    while (raw.size() < 131072) {
        int count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        raw.append(buffer, count);
        separator = raw.find("\r\n\r\n");
        if (separator != std::string::npos) break;
    }
    if (separator == std::string::npos) {
        response(client, 413, "Request too large", "text/plain");
        return false;
    }

    std::istringstream headers(raw.substr(0, separator));
    std::string first, version, line;
    std::getline(headers, first);
    std::istringstream firstLine(first);
    firstLine >> request.method >> request.path >> version;
    if (version.rfind("HTTP/", 0) != 0) {
        response(client, 400, "Bad request", "text/plain");
        return false;
    }

    size_t length = 0;
    while (std::getline(headers, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const std::string key = lower(line.substr(0, colon));
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        if (key == "content-length") {
            try {
                length = (size_t)std::stoull(value);
            } catch (...) {
                response(client, 400, "Invalid content length", "text/plain");
                return false;
            }
        }
        if (key == "authorization") request.authorization = value;
    }
    if (length > 65536) {
        response(client, 413, "Body too large", "text/plain");
        return false;
    }

    request.payload = raw.substr(separator + 4);
    while (request.payload.size() < length) {
        int count = recv(client, buffer,
            (int)std::min<size_t>(sizeof(buffer), length - request.payload.size()), 0);
        if (count <= 0) return false;
        request.payload.append(buffer, count);
    }
    if (request.payload.size() > length) request.payload.resize(length);
    request.path = request.path.substr(0, request.path.find('?'));
    return true;
}

static void handleApiRequest(Socket client, App& app, const HttpRequest& request) {
    try {
        Json body = request.payload.empty() ? Json(Json::Object{}) :
                                                   Json::parse(request.payload);
        if (!body.isObject()) {
            response(client, 400, err("JSON object required").dump(),
                     "application/json; charset=utf-8");
            return;
        }
        int status = 200;
        const std::string bearer = request.authorization.rfind("Bearer ", 0) == 0 ?
            request.authorization.substr(7) : "";
        const Json result = app.handle(request.method, request.path, body, bearer, status);
        response(client, status, result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        std::cerr << "API error: " << error.what() << '\n';
        response(client, 400, err("Invalid request or database operation.").dump(),
                 "application/json; charset=utf-8");
    }
}

static void serveStaticFile(Socket client, const HttpRequest& request) {
    if (request.method != "GET" && request.method != "HEAD") {
        response(client, 405, "Method not allowed", "text/plain");
        return;
    }
    std::string path = request.path == "/" ? "/index.html" : request.path;
    if (path != "/index.html" && path != "/js/app.js" &&
        path != "/js/calendar.js" && path != "/js/pages.js" &&
        path != "/js/tasks.js" && path != "/js/features.js" &&
        path != "/css/style.css") {
        response(client, 404, "Not found", "text/plain");
        return;
    }
    const std::string content = readFile(fs::path(WEB_DIR) / path.substr(1));
    if (content.empty()) {
        response(client, 404, "Not found", "text/plain");
        return;
    }
    const std::string mime = path == "/css/style.css" ? "text/css; charset=utf-8" :
        path.rfind("/js/", 0) == 0 ? "application/javascript; charset=utf-8" :
                                     "text/html; charset=utf-8";
    response(client, 200, request.method == "HEAD" ? "" : content, mime);
}

void serve(Socket client, App& app) {
    HttpRequest request;
    if (!readRequest(client, request)) return;
    if (request.path.rfind("/api/", 0) == 0)
        handleApiRequest(client, app, request);
    else
        serveStaticFile(client, request);
}
