//
// Created by Vlad on 9/17/2020.
//


#ifndef C_THREAD_C_THREAD_PRIVATE
#define C_THREAD_C_THREAD_PRIVATE

#include <c-thread-types.h>

#define THREAD_FIRST_ID (thread_t) 1U

#include <stdint.h>

#define THREAD_MAX (thread_t) UINT64_MAX

#ifndef THREAD_ARGUMENT_HASH_SIZE
#define THREAD_ARGUMENT_HASH_SIZE 256
#endif

#if THREAD_ARGUMENT_HASH_SIZE <= UINT8_MAX + 1
#define THREAD_KEY_TYPE uint8
#elif THREAD_ARGUMENT_HASH_SIZE <= UINT16_MAX + 1
#define THREAD_KEY_TYPE uint16
#elif THREAD_ARGUMENT_HASH_SIZE <= UINT32_MAX + 1
#define THREAD_KEY_TYPE uint32
#else
#define THREAD_KEY_TYPE uint64
#endif

#define CLI_BUFFER_SIZE 65536

#endif //C_THREAD_C_THREAD_PRIVATE