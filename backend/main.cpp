#include "server/HttpServer.h"
// Entry point: initialise sockets, open SQLite and listen locally on port 8080.
int main()
{
    try
    {
#ifdef _WIN32
        WSADATA w
        {
        };
        if(WSAStartup(MAKEWORD(2,2),&w)!=0)throw std::runtime_error("Winsock initialization failed");
#endif
        fs::path dbPath=fs::current_path()/"data"/"studysync.db";
        App app(dbPath);
        Socket server=socket(AF_INET,SOCK_STREAM,0);
        if(server==invalidSocket)throw std::runtime_error("Unable to create socket");
        int yes=1;
        setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&yes,sizeof(yes));
        sockaddr_in addr
        {
        };
        addr.sin_family=AF_INET;
        int port=8080;
        if(const char* v=std::getenv("STUDYSYNC_PORT"))
        {
            try
            {
                port=std::stoi(v);
            }
            catch(...)
            {
            }
        }
        if(port<1024||port>65535)port=8080;
        addr.sin_port=htons((unsigned short)port);
        addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(bind(server,(sockaddr*)&addr,sizeof(addr))!=0)throw std::runtime_error("Could not start StudySync on port " + std::to_string(port) + ". Close any other running copy and try again.");
        if(listen(server,16)!=0)throw std::runtime_error("Unable to listen");
        std::cout<<"StudySync running at http://127.0.0.1:"<<port<<"\nDatabase: "<<dbPath<<std::endl;
        while(true)
        {
            Socket client=accept(server,nullptr,nullptr);
            if(client==invalidSocket)continue;
            try
            {
                serve(client,app);
            }
            catch(const std::exception&e)
            {
                std::cerr<<"Request failed: "<<e.what()<<'\n';
            }
            closeSocket(client);
        }
    }
    catch(const std::exception&e)
    {
        std::cerr<<"Fatal: "<<e.what()<<'\n';
        return 1;
    }
    return 0;
}
