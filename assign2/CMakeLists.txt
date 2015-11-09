cmake_minimum_required(VERSION 3.3)
project(assign2)

find_package (Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(host src/host.c src/common.h)
target_link_libraries (host ${CMAKE_THREAD_LIBS_INIT})

add_executable(producer src/producer.c src/common.h)
target_link_libraries (producer ${CMAKE_THREAD_LIBS_INIT})

add_executable(consumer src/consumer.c src/common.h)
target_link_libraries (consumer ${CMAKE_THREAD_LIBS_INIT})