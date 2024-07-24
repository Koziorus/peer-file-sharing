cmake_minimum_required(VERSION 3.8)

project(PeerFileSharing)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME} main.c bencode.c)
target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)