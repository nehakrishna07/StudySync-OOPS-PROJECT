cmake_minimum_required(VERSION 3.20)
project(StudySyncNoQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
add_executable(StudySyncWeb backend/main.cpp
    backend/models/Task.cpp
    backend/database/Database.cpp
    backend/server/HttpServer.cpp
    backend/services/App.cpp
    backend/services/TaskManager.cpp
    backend/services/Scheduler.cpp)
target_link_libraries(StudySyncWeb PRIVATE SQLite3::SQLite3)

if(WIN32)
    target_link_libraries(StudySyncWeb PRIVATE ws2_32 bcrypt)
else()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(StudySyncWeb PRIVATE OpenSSL::Crypto)
endif()

target_compile_definitions(StudySyncWeb PRIVATE WEB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/frontend")

if(MSVC)
    target_compile_options(StudySyncWeb PRIVATE /W4)
else()
    target_compile_options(StudySyncWeb PRIVATE -Wall -Wextra -Wpedantic)
endif()

