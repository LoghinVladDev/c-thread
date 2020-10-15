cmake_minimum_required(VERSION 3.10)
project(c-thread)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(
        C_THREAD_LIBS
        -lpthread
)

set(
        C_THREAD_C_SRC
        src/thread/c-thread.c
        src/thread/c-thread.h
        src/thread/c-thread-private.h
        src/thread/c-thread-types.h
)

set(
        C_THREAD_CPP_SRC
        ${C_THREAD_C_SRC}
        src/cpp/Thread.cpp
        src/cpp/Thread.h
)

set(
        C_THREAD_INCLUDE
        src/thread
        src/cpp
)

include_directories(${C_THREAD_INCLUDE})

add_executable(
        threadTest1
        ${C_THREAD_C_SRC}
)

target_link_libraries( threadTest1 ${C_THREAD_LIBS} )

add_executable(
        threadCppTest
        src/tests/threadCppTest.cpp
        ${C_THREAD_CPP_SRC}
)

target_link_libraries( threadCppTest ${C_THREAD_LIBS} )
