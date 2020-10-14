//
// Created by Vlad on 9/17/2020.
//

#ifndef C_THREAD_C_THREAD_H
#define C_THREAD_C_THREAD_H

#include <pthread.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef char sint8;
typedef short sint16;
typedef int sint32;
typedef long long sint64;

typedef uint64 Thread;

typedef enum {
    THREAD_STRUCTURE_TYPE_CREATE_INFO_ARGS,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_FORMAT,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_ARG_PTR,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS
} ThreadStructureType;

typedef struct {
    ThreadStructureType sType;
    Thread ID;
    int argc;
    char ** argv;
    void (* pFunc) (int, char **);
} ThreadCreateInfoArgs;

typedef struct {
    ThreadStructureType sType;
    Thread ID;
    const char * format;
    char * argString;
    void (* pFunc) ( const char *, char * );
} ThreadCreateInfoFormat;

typedef struct {
    ThreadStructureType sType;
    Thread ID;
    void (* pFunc) ( void );
} ThreadCreateInfoNoArgs;

typedef struct {
    ThreadStructureType sType;
    Thread ID;
    void (* pFunc) (void *);
    void * data;
} ThreadCreateInfoArgPtr;

typedef enum {
    THREAD_SUCCESS,
    THREAD_NODE_NOT_EXIST,
    THREAD_ARG_NULL,
    THREAD_MAX_CREATED,
    THREAD_INVALID_ATTRIBUTES,
    THREAD_ATTRIBUTE_PERMISSIONS_MISSING
} ThreadResult;

ThreadResult initThreadModule ();
ThreadResult stopThreadModule ();

ThreadResult createThreadArgs ( Thread *, void (*)(int, char** ), int, char ** ) ;
#ifdef __linux__
ThreadResult createThreadF ( Thread *, void (*)(const char *), const char*, ... );
ThreadResult threadScanF ( Thread, const char*, ... );
#endif
ThreadResult createThread ( Thread *, void (*)(void) );
ThreadResult createThreadArgPtr ( Thread *, void (*)(void *), void * );
ThreadResult createCLIAdminThread ();
uint8        isAdminThreadRunning ();

ThreadResult setArgument ( Thread, void *, uint64 );
ThreadResult getArgument ( Thread, void *, uint64 );
ThreadResult lock ( Thread );
ThreadResult unlock ( Thread );

ThreadResult startThread ( Thread );
ThreadResult killThread ( Thread );
ThreadResult stopThread ( Thread );

void threadPrintf ( const char *, ... );
void threadPutS ( const char * );
void threadTryPutS ( const char * );

#endif // C_THREAD_C_THREAD_H
