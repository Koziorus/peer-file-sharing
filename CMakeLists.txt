cmake_minimum_required(VERSION 3.8)

project(PeerFileSharing)

set(CMAKE_C_STANDARD 23) # for typed enums
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb") # -O0 -> clearer for valgrind, -ggdb -> for debug info optimized for gdb

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME} src/bencode.c src/network.c src/helper.c src/tracker_communication.c src/peer_communication.c src/main.c)
target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)
include_directories(src/headers)