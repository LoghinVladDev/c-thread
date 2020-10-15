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

typedef struct {
    thread_t            ID;
    pthread_t           pthreadHandle;
    pthread_mutex_t     pthreadMutex;
    void              * createInfo;
} ThreadHandleInfo;

typedef struct ThreadHandleNode { // NOLINT(bugprone-reserved-identifier)
    ThreadHandleInfo data;
    struct ThreadHandleNode * next;
} ThreadHandleNode;

typedef ThreadHandleNode * ThreadHandleLinkedList;

static struct {
    bool               threadRunning;
} adminThreadInfo;

typedef struct ThreadArgumentNode {
    thread_t ID;
    void * data;
    uint64 size;
    struct ThreadArgumentNode * pNext;
    char * pTag;
} ThreadArgumentNode;

typedef struct ThreadMemoryPointerNode {
    thread_t ID;
    void * p;
    struct ThreadMemoryPointerNode * pNext;
} ThreadMemoryPointerNode;

static pthread_mutex_t threadMemoryLock;
static ThreadMemoryPointerNode * threadMemoryHashTable [ THREAD_ARGUMENT_HASH_SIZE ];

static pthread_mutex_t      threadArgumentsLock;
static ThreadArgumentNode * threadArgumentsHashTable [ THREAD_ARGUMENT_HASH_SIZE ];

static pthread_mutex_t        listLock;
static pthread_mutex_t        consoleLock;
static ThreadHandleLinkedList threadListHead    = NULL;
static thread_t               threadCounter     = THREAD_FIRST_ID;
static thread_t               adminThread       = THREAD_MAX;

static bool threadModuleInitialised = false;

static void             * internalCreateThreadNoArgs ( void* );
static void             * internalCreateThreadArgs ( void* );
static void             * internalCreateThreadFormat ( void* );
static void             * internalCreateThreadArgPtr ( void* );
static ThreadHandleInfo * getThreadHandleInfo (thread_t );
static ThreadHandleInfo * createThreadHandleInfo (thread_t );
static ThreadResult       removeThreadInfo (thread_t );
static void               clearThreadInfoList ();

static void               printThreadInfo ( const ThreadHandleInfo *, const char * );
static void               printProcesses ();
static void               printProcessesNotHidden ();
static bool               treadAdminThreadRequest ( const char* );
static void               printAdminHelpInfo ();
static void             * adminThreadMain ( void * );

static void               clearThreadArgumentsHashTable();
static void               clearThreadMemoryHashTable();

#endif //C_THREAD_C_THREAD_PRIVATE