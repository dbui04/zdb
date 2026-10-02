cmake_minimum_required(VERSION 3.23)
project(zdb LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(zdb
    src/database/database.cpp
    src/net/client.cpp
    src/net/kqueue.cpp
    src/net/server.cpp
    src/net/socket.cpp
    src/net/tcp_connection.cpp
    src/net/tcp_listener.cpp
    src/protocol/serialisation.cpp
    src/util/byte_buffer.cpp
    src/util/time.cpp
)

target_compile_features(zdb PUBLIC cxx_std_23)
target_include_directories(zdb PUBLIC include)

add_executable(server app/server.cpp)
target_link_libraries(server PRIVATE zdb)

add_executable(client app/client.cpp)
target_link_libraries(client PRIVATE zdb)
