cmake_minimum_required(VERSION 3.8)

project(PeerFileSharing)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME} src/bencode.c src/network.c src/helper.c src/tracker_communication.c src/peer_communication.c src/main.c)
target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)
include_directories(src/headers)