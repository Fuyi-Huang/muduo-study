cmake_minimum_required(VERSION 2.6)

# add headers directories
# ../(examples)../(muduo-2.0.0-beta)
include_directories(../../)

# static libraries directories
link_directories("/usr/local/src/build/release-cpp11/lib/")

link_libraries( 
    muduo_net
    muduo_base
    pthread
)

add_executable(socks5 socks5.cc)
