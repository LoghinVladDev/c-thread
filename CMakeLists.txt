cmake_minimum_required(VERSION 3.17)
project(c-thread)

set(CMAKE_C_STANDARD 99)
set(C_THREAD_LIBS -lpthread)

include_directories(src/thread)

add_executable(
        threadTest1
        src/tests/threadMainTest.c
        src/thread/c-thread.c
        src/thread/c-thread.h
        src/thread/c-thread-private.h
)

target_link_libraries( threadTest1 ${C_THREAD_LIBS} )
