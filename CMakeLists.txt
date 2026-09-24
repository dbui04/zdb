cmake_minimum_required(VERSION 3.23)
project(Database LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(database
    src/byte_buffer.cpp
    src/socket.cpp
    src/tcp_connection.cpp
    src/tcp_listener.cpp
    src/server.cpp
)

target_compile_features(database PUBLIC cxx_std_23)
target_include_directories(database PUBLIC include)

add_executable(server app/server.cpp)
target_link_libraries(server PRIVATE database)

add_executable(client app/client.cpp)
target_link_libraries(client PRIVATE database)
