cmake_minimum_required(VERSION 3.22)
project(tcp_server_boilerplate)

set(EXEC server)
set(CMAKE_CXX_STANDARD 14)

include_directories(src)
file(GLOB SOURCES src/*.cpp)

add_executable(${EXEC} ${SOURCES})