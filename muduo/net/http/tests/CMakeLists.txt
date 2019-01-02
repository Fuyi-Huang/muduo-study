cmake_minimum_required(VERSION 2.6)

# add headers directories
# ../(http)../(net)../(muduo)../(muduo-2.0.0-beta)
include_directories(../../../../)

# static libraries directories
link_directories("/usr/local/src/build/release-cpp11/lib/")

set(http_SRCS
  ../HttpServer.cc
  ../HttpResponse.cc
  ../HttpContext.cc
  )
add_library(muduo_http ${http_SRCS})
target_link_libraries(muduo_http 
    muduo_net
)

link_libraries( 
    muduo_base
    pthread
)

add_executable(httpserver_test HttpServer_test.cc)
target_link_libraries(httpserver_test 
    muduo_http
)