cmake_minimum_required(VERSION 3.23)
project(zdb LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(zdb
    src/byte_buffer.cpp
    src/socket.cpp
    src/tcp_connection.cpp
    src/tcp_listener.cpp
    src/server.cpp
    src/client.cpp
    src/utils.cpp
    src/net.cpp
    src/payload.cpp
    src/parse.cpp
    src/kqueue.cpp
)

target_compile_features(zdb PUBLIC cxx_std_23)
target_include_directories(zdb PUBLIC include)

add_executable(server app/server.cpp)
target_link_libraries(server PRIVATE zdb)

add_executable(client app/client.cpp)
target_link_libraries(client PRIVATE zdb)
