//
// Created by loghin on 10/14/20.
//

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <c-thread.h>
#include <c-thread-private.h>

#ifdef __linux__
#include <stdarg.h>
#endif

typedef struct {
    Thread              ID;
    pthread_t           pthreadHandle;
    pthread_mutex_t     pthreadMutex;
    void              * createInfo;
} ThreadHandleInfo;

struct _ThreadHandleNode { // NOLINT(bugprone-reserved-identifier)
    ThreadHandleInfo data;
    struct _ThreadHandleNode * next;
};

static struct {
    uint8               threadRunning;
} adminThreadInfo;

typedef struct _ThreadHandleNode ThreadHandleNode;
typedef ThreadHandleNode * ThreadHandleLinkedList;

static pthread_mutex_t        listLock;
static pthread_mutex_t        consoleLock;
static ThreadHandleLinkedList threadListHead    = NULL;
static Thread                 threadCounter     = THREAD_FIRST_ID;
static Thread                 adminThread       = THREAD_MAX;

static void             * internalCreateThreadNoArgs ( void* );
static void             * internalCreateThreadArgs ( void* );
static void             * internalCreateThreadFormat ( void* );
static void             * internalCreateThreadArgPtr ( void* );
static ThreadHandleInfo * getThreadHandleInfo ( Thread );
static ThreadHandleInfo * createThreadHandleInfo ( Thread );
static ThreadResult       removeThreadInfo ( Thread );
static void               clearThreadInfoList ();

static void               printThreadInfo ( const ThreadHandleInfo *, const char * );
static void               printProcesses ();
static void               printProcessesNotHidden ();
static uint8              treadAdminThreadRequest ( const char* );
static void               printAdminHelpInfo ();
static void             * adminThreadMain ( void * );

ThreadHandleInfo * getThreadHandleInfo ( Thread ID ) {
    pthread_mutex_lock( & listLock );

    ThreadHandleLinkedList headCopy = threadListHead;
    while ( headCopy != NULL ) {
        if ( headCopy->data.ID == ID ) {

            pthread_mutex_unlock( & listLock );
            return (&headCopy->data);
        }
        headCopy = headCopy->next;
    }

    pthread_mutex_unlock( & listLock );
    return NULL;
}

ThreadHandleInfo * createThreadHandleInfo ( Thread ID ) {
    pthread_mutex_lock( & listLock );
    ThreadHandleLinkedList newHead = ( ThreadHandleNode * ) malloc ( sizeof ( ThreadHandleNode ) );

    newHead->data.ID            = ID;
    newHead->data.pthreadHandle = 0U;
    newHead->data.createInfo    = NULL;
    pthread_mutex_init ( & newHead->data.pthreadMutex, NULL );

    newHead->next = threadListHead;
    threadListHead = newHead;

    pthread_mutex_unlock( & listLock );

    return ( & threadListHead->data );
}

ThreadResult removeThreadInfo ( Thread ID ) {
    pthread_mutex_lock( & listLock );
    ThreadHandleLinkedList headCopy = threadListHead;

    if ( headCopy == NULL ) {
        pthread_mutex_unlock( & listLock );
        return THREAD_NODE_NOT_EXIST;
    }

    if ( headCopy->data.ID == ID ) {
        threadListHead = headCopy->next;
        free ( headCopy->data.createInfo );
        free ( headCopy );
        pthread_mutex_unlock( & listLock );
        return THREAD_SUCCESS;
    }

//    ThreadHandleNode * previous = NULL;
    while ( headCopy->next != NULL ) {
        if ( headCopy->next->data.ID == ID ) {
            ThreadHandleNode * p = headCopy->next;
            headCopy->next = headCopy->next->next;
            free ( p->data.createInfo );
            free ( p );
            pthread_mutex_unlock( & listLock );
            return THREAD_SUCCESS;
        }
//        previous = headCopy;
        headCopy = headCopy->next;
    }
    pthread_mutex_unlock( & listLock );
    return THREAD_NODE_NOT_EXIST;
}

void clearThreadInfoList () {

    pthread_mutex_lock( & listLock );
    while ( threadListHead != NULL ) {
        ThreadHandleNode * p = threadListHead;
        threadListHead = threadListHead->next;
        pthread_mutex_destroy( & p->data.pthreadMutex );
        pthread_cancel ( p->data.pthreadHandle );
        free ( p->data.createInfo );
        free ( p );
    }

    pthread_mutex_unlock( & listLock );
}

ThreadResult killThread ( Thread ID ) {
    if ( ID == THREAD_MAX )
        return THREAD_SUCCESS;
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_destroy( & pThreadInfo->pthreadMutex );
    pthread_cancel( pThreadInfo->pthreadHandle );

    removeThreadInfo( ID );

    return THREAD_SUCCESS;
}

ThreadResult stopThread ( Thread ID ) {
    if ( ID == THREAD_MAX )
        return THREAD_SUCCESS;
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_lock( & pThreadInfo->pthreadMutex );
    pthread_join( pThreadInfo->pthreadHandle, NULL );
    pthread_mutex_unlock( & pThreadInfo->pthreadMutex );

    removeThreadInfo( ID );

    return THREAD_SUCCESS;
}

ThreadResult lock ( Thread ID ) {
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_lock ( & pThreadInfo->pthreadMutex );

    return THREAD_SUCCESS;
}

ThreadResult unlock ( Thread ID ) {
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_unlock( & pThreadInfo->pthreadMutex );

    return THREAD_SUCCESS;
}

ThreadResult createThreadArgs ( Thread * pID, void (* pFunc) (int, char**), int argc, char ** argv ) {
    if ( pID == NULL || pFunc == NULL )
        return THREAD_ARG_NULL;

    * pID = threadCounter++;

    if ( threadCounter == THREAD_MAX ) {
        * pID = 0;
        return THREAD_MAX_CREATED;
    }

    __auto_type pThreadInfo = createThreadHandleInfo( * pID );
    __auto_type pThreadCreateInfo = (ThreadCreateInfoArgs *) malloc ( sizeof ( ThreadCreateInfoArgs) );

    pThreadCreateInfo->sType = THREAD_STRUCTURE_TYPE_CREATE_INFO_ARGS;
    pThreadCreateInfo->pFunc = pFunc;
    pThreadCreateInfo->ID    = * pID;
    pThreadCreateInfo->argc  = argc;
    pThreadCreateInfo->argv  = argv;

    pThreadInfo->createInfo = pThreadCreateInfo;

//    pthread_create ( & pThreadInfo->pthreadHandle, NULL, internalCreateThreadArgs, pThreadCreateInfo );

    return THREAD_SUCCESS;
}

#include <errno.h>
ThreadResult startThread ( Thread ID ) {
    __auto_type pThreadInfo = getThreadHandleInfo( ID );

    void * (* pStartFunc) (void*);
    switch ( ( * (ThreadStructureType* ) pThreadInfo->createInfo ) ) {
        case THREAD_STRUCTURE_TYPE_CREATE_INFO_ARGS:    pStartFunc = internalCreateThreadArgs;      break;
#ifdef __linux__
        case THREAD_STRUCTURE_TYPE_CREATE_INFO_FORMAT:  pStartFunc = internalCreateThreadFormat;    break;
#endif
        case THREAD_STRUCTURE_TYPE_CREATE_INFO_ARG_PTR: pStartFunc = internalCreateThreadArgPtr;    break;
        case THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS: pStartFunc = internalCreateThreadNoArgs;    break;
    }

    int errorID = pthread_create(
            & pThreadInfo->pthreadHandle,
            NULL,
            pStartFunc,
            pThreadInfo->createInfo
    );

    if ( errorID == EAGAIN )
        return THREAD_MAX_CREATED;

    if ( errorID == EINVAL )
        return THREAD_INVALID_ATTRIBUTES;

    if ( errorID == EPERM )
        return THREAD_ATTRIBUTE_PERMISSIONS_MISSING;

    return THREAD_SUCCESS;
}

ThreadResult createThreadArgPtr ( Thread * pID, void (*pFunc) (void*), void * pVoidArg ) {
    if ( pID == NULL || pFunc == NULL )
        return THREAD_ARG_NULL;

    * pID = threadCounter++;

    if ( threadCounter == THREAD_MAX ) {
        * pID = 0;
        return THREAD_MAX_CREATED;
    }

    __auto_type pThreadInfo = createThreadHandleInfo( * pID );
    __auto_type pThreadCreateInfo = (ThreadCreateInfoArgPtr *) malloc ( sizeof ( ThreadCreateInfoArgPtr ) );

    pThreadCreateInfo->sType = THREAD_STRUCTURE_TYPE_CREATE_INFO_ARG_PTR;
    pThreadCreateInfo->pFunc = pFunc;
    pThreadCreateInfo->ID    = * pID;
    pThreadCreateInfo->data  = pVoidArg;

    pThreadInfo->createInfo = pThreadCreateInfo;

//    pthread_create ( & pThreadInfo->pthreadHandle, NULL, internalCreateThreadArgPtr, pThreadCreateInfo );

    return THREAD_SUCCESS;
}

ThreadResult createThread ( Thread * pID, void ( * pFunc ) ( void ) ) {
    if ( pID == NULL || pFunc == NULL )
        return THREAD_ARG_NULL;

    * pID = threadCounter++;

    if ( threadCounter == THREAD_MAX ) {
        * pID = 0;
        return THREAD_MAX_CREATED;
    }

    __auto_type pThreadInfo = createThreadHandleInfo( * pID );
    __auto_type pThreadCreateInfo = (ThreadCreateInfoNoArgs *) malloc ( sizeof ( ThreadCreateInfoNoArgs) );

    pThreadCreateInfo->sType = THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS;
    pThreadCreateInfo->pFunc = pFunc;
    pThreadCreateInfo->ID    = * pID;

    pThreadInfo->createInfo = pThreadCreateInfo;

//    pthread_create ( & pThreadInfo->pthreadHandle, NULL, internalCreateThreadNoArgs, pThreadCreateInfo );

    return THREAD_SUCCESS;
}

typedef struct ThreadArgumentNode {
    Thread ID;
    void * data;
    uint64 size;
    struct ThreadArgumentNode * pNext;
} ThreadArgumentNode;

static ThreadArgumentNode * threadArgumentsHashTable [ THREAD_ARGUMENT_HASH_SIZE ];

ThreadResult setArgument ( Thread ID, void * pArg, uint64 argSize) {
    if ( pArg == NULL || argSize == 0 )
        return THREAD_ARG_NULL;

    uint32 key = (uint32) (ID % THREAD_ARGUMENT_HASH_SIZE);

    __auto_type listHead = threadArgumentsHashTable[ key ];
    __auto_type pNewNode = ( ThreadArgumentNode * ) malloc ( sizeof ( ThreadArgumentNode ) );
    pNewNode->ID    = ID;
    pNewNode->data  = (void *) malloc ( argSize );
    pNewNode->size  = argSize;
    pNewNode->pNext = NULL;
    memcpy ( pNewNode->data, pArg, argSize );

    if ( listHead == NULL ) {
        threadArgumentsHashTable[ key ] = pNewNode;
        return THREAD_SUCCESS;
    }

    while ( listHead->pNext != NULL ) {
        listHead = listHead->pNext;
    }

    listHead->pNext = pNewNode;
    return THREAD_SUCCESS;
}

void printThreadInfo ( const ThreadHandleInfo * pThreadInfo, const char * prefix ) {
    printf( "%s[Thread] [ID : %llu] [Type : %s]\n", prefix, pThreadInfo->ID, (pThreadInfo->ID == THREAD_MAX) ? "Admin Thread" : "Regular Thread" );
    fflush(stdout);
}

void printProcesses () {
//    pthread_mutex_lock( & consoleLock );
    pthread_mutex_lock( & listLock );
    ThreadHandleLinkedList headCopy = threadListHead;

    printf("Active Threads : \n");

    while ( headCopy != NULL ) {
        printThreadInfo( & headCopy->data, "\t" );

        headCopy = headCopy->next;
    }
    pthread_mutex_unlock( & listLock );
//    pthread_mutex_unlock( & consoleLock );
}

void printProcessesNotHidden () {

    pthread_mutex_lock( & listLock );
    ThreadHandleLinkedList headCopy = threadListHead;

    printf("Active Threads : \n");

    while ( headCopy != NULL ) {
        if ( headCopy->data.ID != THREAD_MAX )
            printThreadInfo( & headCopy->data, "\t" );

        headCopy = headCopy->next;
    }
    pthread_mutex_unlock( & listLock );
}

uint8 treadAdminThreadRequest ( const char* request ) {
    if ( strcmp ( request, "stop\n" ) == 0 )
        return 0;

    if ( strstr ( request, "ls " ) == request ) {
        if ( strstr ( request, "-a" ) != NULL )
            printProcesses();
        else if ( strstr ( request , "-A" ) != NULL )
            printProcessesNotHidden();
    }

    if ( strstr ( request, "kill " ) == request ) {
        const char * pThreadIDStr = request + strlen ("kill ");
        while ( ! (( * pThreadIDStr >= '0' ) && ( * pThreadIDStr ) <= '9') )
            pThreadIDStr++;

        killThread( strtol ( pThreadIDStr, NULL, 10 ) );
    }

    if ( strcmp ( request, "lc\n" ) == 0 ) {
        pthread_mutex_lock( & consoleLock );
    }

    if ( strcmp ( request, "uc\n" ) == 0 ) {
        pthread_mutex_unlock( & consoleLock );
    }

    if ( strstr ( request, "lock " ) == request ) {
        const char * pThreadIDStr = request + strlen ("lock ");
        while ( ! (( * pThreadIDStr >= '0' ) && ( * pThreadIDStr ) <= '9') )
            pThreadIDStr++;

        lock( strtol ( pThreadIDStr, NULL, 10 ) );
    }

    if ( strstr ( request, "unlock " ) == request ) {
        const char * pThreadIDStr = request + strlen ("unlock ");
        while ( ! (( * pThreadIDStr >= '0' ) && ( * pThreadIDStr ) <= '9') )
            pThreadIDStr++;

        unlock( strtol ( pThreadIDStr, NULL, 10 ) );
    }

    if ( strcmp ( request, "help\n" ) == 0 ) {
        printAdminHelpInfo();
    }

    return 1;
}

void printAdminHelpInfo () {
//    pthread_mutex_lock( & consoleLock );

    printf("Welcome to Command Line Interface for admin thread!\n");
    printf("Commands : \n");
    printf("\thelp - Prints the help section");
    printf("\tls - Lists all threads active\n");
    printf("\t\t-A all threads\n");
    printf("\t\t-a all threads including hidden ones\n");
    printf("\tlc - locks CLI console so that other threads cannot print on it\n");
    printf("\tuc - unlocks CLI console so that other threads can print on it\n");

    printf("\tlock [ID] - Locks a given thread's default lock\n");
    printf("\tunlock [ID] - Unlocks a given thread's default lock\n");

    printf("\tkill [ID] - Kills thread with specified ID\n");
    printf("\tstop - Stops the program\n");

    fflush(stdout);

//    pthread_mutex_unlock( & consoleLock );
}

void * adminThreadMain ( void * args ) {
    printAdminHelpInfo();

    char * CLIBuffer = (char*) malloc ( sizeof ( char ) * CLI_BUFFER_SIZE );
    while (1) {
        memset ( CLIBuffer, 0, CLI_BUFFER_SIZE );
        read ( 0, CLIBuffer, CLI_BUFFER_SIZE );
        fflush( stdout );

        if ( 0 == treadAdminThreadRequest( CLIBuffer ) )
            break;
    }

    adminThreadInfo.threadRunning = 0;

    free ( CLIBuffer );

    return ( NULL );
}

inline uint8 isAdminThreadRunning () {
    return adminThreadInfo.threadRunning;
}

ThreadResult createCLIAdminThread () {

    adminThreadInfo.threadRunning = 1;

    __auto_type pThreadInfo = createThreadHandleInfo( adminThread );
    __auto_type pThreadCreateInfo = ( ThreadCreateInfoNoArgs * ) malloc ( sizeof ( ThreadCreateInfoNoArgs ) );

    pThreadCreateInfo->sType    = THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS;
    pThreadCreateInfo->pFunc    = NULL;
    pThreadCreateInfo->ID       = adminThread;

    pThreadInfo->createInfo     = pThreadCreateInfo;

    pthread_create( & pThreadInfo->pthreadHandle, NULL, adminThreadMain, pThreadCreateInfo );

    return THREAD_SUCCESS;
}

void * internalCreateThreadNoArgs ( void* pVoidArg ) {
    __auto_type pArg = ( ThreadCreateInfoNoArgs * ) pVoidArg;

    if ( pArg->sType != THREAD_STRUCTURE_TYPE_CREATE_INFO_NO_ARGS )
        return NULL;

    pArg->pFunc ();

    removeThreadInfo( pArg->ID );
    return NULL;
}

void * internalCreateThreadFormat ( void * pVoidArg ) {
    return NULL;
}

void * internalCreateThreadArgs ( void * pVoidArg ) {
    __auto_type pArg = ( ThreadCreateInfoArgs * ) pVoidArg;

    if ( pArg->sType != THREAD_STRUCTURE_TYPE_CREATE_INFO_ARGS )
        return NULL;

    pArg->pFunc ( pArg->argc, pArg->argv );

    removeThreadInfo( pArg->ID );
    return NULL;
}

void * internalCreateThreadArgPtr ( void * pVoidArg ) {
    __auto_type pArg = ( ThreadCreateInfoArgPtr * ) pVoidArg;

    if ( pArg->sType != THREAD_STRUCTURE_TYPE_CREATE_INFO_ARG_PTR )
        return NULL;

    pArg->pFunc ( pArg->data );

    removeThreadInfo( pArg->ID );
    return NULL;
}

ThreadResult initThreadModule () {
    pthread_mutex_init( & listLock, NULL );
    pthread_mutex_init( & consoleLock, NULL );

    return THREAD_SUCCESS;
}

ThreadResult stopThreadModule() {

    clearThreadInfoList();

    pthread_mutex_destroy( & listLock );
    pthread_mutex_destroy( & consoleLock );

    return THREAD_SUCCESS;
}

void threadPutS( const char * string ) {
    pthread_mutex_lock( & consoleLock );

    printf("%s", string);
    fflush( stdout );

    pthread_mutex_unlock( & consoleLock );
}

void threadTryPutS( const char * string ) {
    if ( pthread_mutex_trylock( & consoleLock ) == 0 ) {
        printf("%s", string);
        fflush(stdout);
        pthread_mutex_unlock(&consoleLock);
    }
}
