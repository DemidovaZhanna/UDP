cmake_minimum_required(VERSION 3.21)
project(udp)

set(CMAKE_CXX_STANDARD 17)

add_executable(udp main.cpp)

#for _WIN32 uncomment
#target_link_libraries(udp ws2_32 wsock32)
