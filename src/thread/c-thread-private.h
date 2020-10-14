//
// Created by Vlad on 9/17/2020.
//


#ifndef C_THREAD_C_THREAD_PRIVATE
#define C_THREAD_C_THREAD_PRIVATE

#include <c-thread.h>

#define NULL_THREAD (Thread) 0U
#define THREAD_FIRST_ID (Thread) 1U

#ifndef UINT64_MAX
#define UINT64_MAX ((uint64)~(uint64)0U)
#endif

#define THREAD_MAX (Thread) UINT64_MAX

#define THREAD_ARGUMENT_HASH_SIZE 256

#define CLI_BUFFER_SIZE 65536

#endif //C_THREAD_C_THREAD_PRIVATE