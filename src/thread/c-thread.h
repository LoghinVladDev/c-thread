//
// Created by Vlad on 9/17/2020.
//

#ifndef C_THREAD_C_THREAD_H
#define C_THREAD_C_THREAD_H

#include <pthread.h>
#include <c-thread-types.h>

#define NULL_THREAD (thread_t) 0U

typedef enum {
    THREAD_STRUCTURE_TYPE_CREATE_INFO_ARGS,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_FORMAT,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_ARG_PTR,
    THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS
} ThreadStructureType;

typedef struct {
    ThreadStructureType sType;
    thread_t ID;
    int argc;
    char ** argv;
    void (* pFunc) (int, char **);
} ThreadCreateInfoArgs;

typedef struct {
    ThreadStructureType sType;
    thread_t ID;
    const char * format;
    char * argString;
    void (* pFunc) ( const char *, char * );
} ThreadCreateInfoFormat;

typedef struct {
    ThreadStructureType sType;
    thread_t ID;
    void (* pFunc) ( void );
} ThreadCreateInfoNoArgs;

typedef struct {
    ThreadStructureType sType;
    thread_t ID;
    void (* pFunc) (void *);
    void * data;
} ThreadCreateInfoArgPtr;

typedef enum {
    THREAD_SUCCESS,
    THREAD_NODE_NOT_EXIST,
    THREAD_ARG_NULL,
    THREAD_ARG_NOT_PRESENT,
    THREAD_MAX_CREATED,
    THREAD_INVALID_ATTRIBUTES,
    THREAD_ATTRIBUTE_PERMISSIONS_MISSING
} ThreadResult;

ThreadResult initThreadModule (); ///implemented
ThreadResult stopThreadModule (); ///implemented

ThreadResult createThreadArgs (thread_t *, void (*)(int, char** ), int, char ** ) ; ///implemented
#ifdef __linux__
//ThreadResult createThreadF ( thread_t *, void (*)(const char *), const char*, ... );
//ThreadResult threadScanF ( thread_t, const char*, ... );
#endif
ThreadResult createThread (thread_t *, void (*)(void) ); ///implemented
ThreadResult createThreadArgPtr (thread_t *, void (*)(void *), void * ); ///implemented
ThreadResult createCLIAdminThread (); ///implemented
bool         isAdminThreadRunning (); ///implemented

ThreadResult setArgument (thread_t, const char * , void *, uint64 ); ///implemented
ThreadResult getArgument (thread_t, const char * , void *, uint64 ); ///implemented
ThreadResult lock (thread_t ); ///implemented
ThreadResult unlock (thread_t ); ///implemented

ThreadResult startThread (thread_t ); ///implemented
ThreadResult killThread (thread_t ); ///implemented
ThreadResult stopThread (thread_t ); ///implemented

#ifdef __linux__
void threadPrintf ( const char *, ... ); ///implemented
void threadTryPrintf ( const char *, ... );  ///implemented
#endif
void threadPutS ( const char * ); ///implemented
void threadTryPutS ( const char * ); ///implemented

void * mallocThreadSafe (thread_t, uint64 ); ///implemented
void freeThreadSafe (thread_t, void * ); ///implemented
void * callocThreadSafe (thread_t, uint64, uint64 ); ///implemented

#endif // C_THREAD_C_THREAD_H
