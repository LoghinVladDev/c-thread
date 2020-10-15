//
// Created by loghin on 10/15/20.
//

#ifndef IS_TEMA1_C_THREAD_TYPES_H
#define IS_TEMA1_C_THREAD_TYPES_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef char sint8;
typedef short sint16;
typedef int sint32;
typedef long long sint64;

#ifndef _STDBOOL_H
#ifndef __cplusplus

typedef uint8 bool;
#define true 1
#define false 0

#endif
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef uint64 thread_t;


#endif //IS_TEMA1_C_THREAD_TYPES_H
