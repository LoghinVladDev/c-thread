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

ThreadHandleInfo * getThreadHandleInfo (thread_t ID ) {
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

ThreadHandleInfo * createThreadHandleInfo (thread_t ID ) {
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

ThreadResult removeThreadInfo (thread_t ID ) {
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

ThreadResult killThread (thread_t ID ) {
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

ThreadResult stopThread (thread_t ID ) {
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

ThreadResult lock (thread_t ID ) {
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_lock ( & pThreadInfo->pthreadMutex );

    return THREAD_SUCCESS;
}

ThreadResult unlock (thread_t ID ) {
    __auto_type pThreadInfo = getThreadHandleInfo( ID );
    if ( pThreadInfo == NULL )
        return THREAD_SUCCESS;

    pthread_mutex_unlock( & pThreadInfo->pthreadMutex );

    return THREAD_SUCCESS;
}

//ThreadResult createThreadF ( thread_t * pID, void(* pFunc)(const char*) )

ThreadResult createThreadArgs (thread_t * pID, void (* pFunc) (int, char**), int argc, char ** argv ) {
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

    pThreadCreateInfo->argv = ( char ** ) malloc ( sizeof ( char * ) * argc );
    for ( int i = 0; i < argc; i++ ) {
        pThreadCreateInfo->argv[i] = ( char * ) malloc ( sizeof ( char ) * strlen ( argv[i] ) + sizeof ( char ) );
        strcpy ( pThreadCreateInfo->argv[i], argv[i] );
    }

    pThreadInfo->createInfo = pThreadCreateInfo;

//    pthread_create ( & pThreadInfo->pthreadHandle, NULL, internalCreateThreadArgs, pThreadCreateInfo );

    return THREAD_SUCCESS;
}

#include <errno.h>
ThreadResult startThread (thread_t ID ) {
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

ThreadResult createThreadArgPtr (thread_t * pID, void (*pFunc) (void*), void * pVoidArg ) {
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

    return THREAD_SUCCESS;
}

ThreadResult createThread (thread_t * pID, void ( * pFunc ) (void ) ) {
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

    return THREAD_SUCCESS;
}
ThreadResult getArgument (thread_t ID, const char * pTag, void * pArg, uint64 argSize) {
    pthread_mutex_lock( & threadArgumentsLock );

    if ( pArg == NULL || argSize == 0 ) {
        pthread_mutex_unlock( & threadArgumentsLock );
        return THREAD_ARG_NULL;
    }

    THREAD_KEY_TYPE key = (THREAD_KEY_TYPE) (ID % THREAD_ARGUMENT_HASH_SIZE);

    __auto_type listHead = threadArgumentsHashTable[ key ];

    if ( listHead == NULL ) {
        pthread_mutex_unlock(&threadArgumentsLock);
        return THREAD_ARG_NOT_PRESENT;
    }

    if ( listHead->pNext == NULL ) {
        if ( strcmp ( listHead->pTag, pTag ) != 0 ) {
            pthread_mutex_unlock( & threadArgumentsLock );
            return THREAD_ARG_NOT_PRESENT;
        }

        memcpy( pArg, listHead->data, listHead->size );
        free( listHead->data );
        free( listHead->pTag );
        free( listHead );

        threadArgumentsHashTable[ key ] = NULL;

        pthread_mutex_unlock( & threadArgumentsLock );
        return THREAD_SUCCESS;
    }

    while ( listHead->pNext != NULL ) {
        if ( strcmp ( listHead->pNext->pTag, pTag ) == 0 ) {
            memcpy ( pArg, listHead->pNext->data, listHead->pNext->size );
            free( listHead->pNext->data );
            free( listHead->pNext->pTag );

            __auto_type p = listHead->pNext;
            listHead->pNext = listHead->pNext->pNext;
            free ( p );

            pthread_mutex_unlock( & threadArgumentsLock );
            return THREAD_SUCCESS;
        }

        listHead = listHead->pNext;
    }

    pthread_mutex_unlock( & threadArgumentsLock );
    return THREAD_ARG_NOT_PRESENT;
}

ThreadResult setArgument (thread_t ID, const char * pTag, void * pArg, uint64 argSize) {
    pthread_mutex_lock( & threadArgumentsLock );

    if ( pArg == NULL || argSize == 0 ){
        pthread_mutex_unlock( & threadArgumentsLock );
        return THREAD_ARG_NULL;
    }
    THREAD_KEY_TYPE key = (THREAD_KEY_TYPE) (ID % THREAD_ARGUMENT_HASH_SIZE);

    __auto_type listHead = threadArgumentsHashTable[ key ];
    __auto_type pNewNode = ( ThreadArgumentNode * ) malloc ( sizeof ( ThreadArgumentNode ) );
    pNewNode->ID    = ID;
    pNewNode->data  = (void *) malloc ( argSize );
    pNewNode->pTag  = (char *) malloc ( strlen ( pTag ) + 1 );
    pNewNode->size  = argSize;
    pNewNode->pNext = listHead;
    memcpy ( pNewNode->data, pArg, argSize );
    strcpy ( pNewNode->pTag, pTag );

    threadArgumentsHashTable [ key ] = pNewNode;

    pthread_mutex_unlock( & threadArgumentsLock );
    return THREAD_SUCCESS;
}

void * mallocThreadSafe (thread_t ID, uint64 size ) {
    pthread_mutex_lock( & threadMemoryLock );

    if ( size == 0 ) {
        pthread_mutex_unlock( & threadMemoryLock );
        return NULL;
    }

    THREAD_KEY_TYPE key = ( THREAD_KEY_TYPE ) ( ID % THREAD_ARGUMENT_HASH_SIZE );

    __auto_type listHead = threadMemoryHashTable [ key ];
    __auto_type pNewNode = ( ThreadMemoryPointerNode * ) malloc ( sizeof ( ThreadMemoryPointerNode ) );
    pNewNode->ID    = ID;
    pNewNode->p     = ( void * ) malloc ( size );
    pNewNode->pNext = listHead;

    threadMemoryHashTable [ key ] = pNewNode;

    pthread_mutex_unlock( & threadMemoryLock );

    return pNewNode->p;
}

void * callocThreadSafe (thread_t ID, uint64 count, uint64 dataSize ) {
    __auto_type pMem = mallocThreadSafe( ID, count * dataSize );
    memset( pMem, 0, count * dataSize );
    return pMem;
}

void freeThreadSafe (thread_t ID, void * pData ) {
    if ( pData == NULL )
        return;

    pthread_mutex_lock( & threadMemoryLock );

    THREAD_KEY_TYPE key = ( THREAD_KEY_TYPE ) ( ID % THREAD_ARGUMENT_HASH_SIZE );

    __auto_type listHead = threadMemoryHashTable[ key ];

    if ( listHead == NULL ) {
        pthread_mutex_unlock( & threadMemoryLock );
        return;
    }

    if ( listHead->pNext == NULL ) {
        if ( listHead->p == pData ) {
            free ( listHead->p );
            free ( listHead );

            threadMemoryHashTable[ key ] = NULL;
        }

        pthread_mutex_unlock( & threadMemoryLock );
        return;
    }

    while ( listHead->pNext != NULL ) {
        if ( listHead->pNext->p == pData ) {
            free ( listHead->pNext->p );

            __auto_type p = listHead->pNext;
            listHead->pNext = listHead->pNext->pNext;
            free (p);

            pthread_mutex_unlock( & threadMemoryLock );
            return;
        }

        listHead = listHead->pNext;
    }

    pthread_mutex_unlock( & threadMemoryLock );
}

void printThreadInfo ( const ThreadHandleInfo * pThreadInfo, const char * prefix ) {
    printf( "%s[thread_t] [ID : %llu] [Type : %s]\n", prefix, pThreadInfo->ID, (pThreadInfo->ID == THREAD_MAX) ? "Admin thread_t" : "Regular thread_t" );
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

bool treadAdminThreadRequest ( const char* request ) {
    if ( strcmp ( request, "stop\n" ) == 0 )
        return false;

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

    return true;
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

inline bool isAdminThreadRunning () {
    return adminThreadInfo.threadRunning;
}

ThreadResult createCLIAdminThread () {

    adminThreadInfo.threadRunning = true;

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

    for ( int i = 0; i < pArg->argc; i++ ) {
        free ( pArg->argv[i] );
    }

    free ( pArg->argv );
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
    if ( ! threadModuleInitialised )
        threadModuleInitialised = true;

    pthread_mutex_init( & listLock, NULL );
    pthread_mutex_init( & consoleLock, NULL );
    pthread_mutex_init( & threadArgumentsLock, NULL );
    pthread_mutex_init( & threadMemoryLock, NULL );

    return THREAD_SUCCESS;
}

void clearThreadArgumentsHashTable() {
    pthread_mutex_lock( & threadArgumentsLock );
    for ( uint64 key = 0; key <= THREAD_ARGUMENT_HASH_SIZE - 1; key ++ ) {

        __auto_type listHead = threadArgumentsHashTable[ key ];

        while ( listHead != NULL ) {
            free( listHead->data );
            free( listHead->pTag );

            __auto_type p = listHead;
            listHead = listHead->pNext;
            free ( p );
        }

        threadArgumentsHashTable[ key ] = NULL;
    }

    pthread_mutex_unlock( & threadArgumentsLock );
}

void clearThreadMemoryHashTable() {
    pthread_mutex_lock( & threadMemoryLock );

    for ( uint64 key = 0; key <= THREAD_ARGUMENT_HASH_SIZE - 1; key++ ) {
        __auto_type listHead = threadMemoryHashTable [ key ];

        while ( listHead != NULL ) {
            free ( listHead->p );

            __auto_type  p = listHead;
            listHead = listHead->pNext;
            free (p);
        }

        threadMemoryHashTable [ key ] = NULL;
    }

    pthread_mutex_unlock( & threadMemoryLock );
}

ThreadResult stopThreadModule() {

    clearThreadInfoList();
    clearThreadArgumentsHashTable();
    clearThreadMemoryHashTable();

    pthread_mutex_destroy( & listLock );
    pthread_mutex_destroy( & consoleLock );

    return THREAD_SUCCESS;
}

#ifdef __linux

void threadPrintf ( const char * format, ... ){
    pthread_mutex_lock( & consoleLock );

    va_list argumentsPointer;
    static char cliBuffer [ CLI_BUFFER_SIZE ];

    memset ( cliBuffer, 0, CLI_BUFFER_SIZE );

    char * lastMemLoc = cliBuffer;

    typedef enum {
        REG_STR,
        FORMAT_START,
        LONG_FORMAT,
    } State;

    State state;
    va_start(argumentsPointer, format);

    for ( uint32 i = 0, length = strlen(format); i < length; i++ ) {
        if ( state == REG_STR ) {

            if ( format[i] == '%' )
                state = FORMAT_START;
            else {
                * ( lastMemLoc++ ) = format[i];
                * ( lastMemLoc ) = 0;
            }
        } else if ( state == FORMAT_START ) {
            if (format[i] == 'd') {
                sprintf(lastMemLoc, "%d", va_arg(argumentsPointer, int));
                lastMemLoc += strlen(lastMemLoc);
                *(lastMemLoc) = 0;

                state = REG_STR;
            } else if (format[i] == 's') {
                sprintf(lastMemLoc, "%s", va_arg(argumentsPointer, const char *));
                lastMemLoc += strlen(lastMemLoc);
                *(lastMemLoc) = 0;

                state = REG_STR;
            } else if (format[i] == 'u') {
                sprintf(lastMemLoc, "%u", va_arg(argumentsPointer, unsigned int));
                lastMemLoc += strlen(lastMemLoc);
                *(lastMemLoc) = 0;

                state = REG_STR;
            } else if (format[i] == 'l') {
                state = LONG_FORMAT;
            } else {
                *(lastMemLoc++) = format[i];
                *(lastMemLoc) = 0;

                state = REG_STR;
            }
        } else if ( state == LONG_FORMAT ) {
            if (format[i] == 'd') {
                sprintf(lastMemLoc, "%lld", va_arg(argumentsPointer, long long int));
                lastMemLoc += strlen(lastMemLoc);
                *(lastMemLoc) = 0;

                state = REG_STR;
            } else if (format[i] == 'u') {
                sprintf(lastMemLoc, "%llu", va_arg(argumentsPointer, long long unsigned int));
                lastMemLoc += strlen(lastMemLoc);
                *(lastMemLoc) = 0;

                state = REG_STR;
            } else {
                *(lastMemLoc++) = format[i];
                *(lastMemLoc) = 0;

                state = REG_STR;
            }
        }
    }

    printf("%s", cliBuffer);

    fflush(stdout);

    pthread_mutex_unlock( & consoleLock );
}

void threadTryPrintf ( const char * format, ... ) {
    if ( 0 == pthread_mutex_trylock( & consoleLock ) ) {

        va_list argumentsPointer;
        static char cliBuffer[CLI_BUFFER_SIZE];

        memset(cliBuffer, 0, CLI_BUFFER_SIZE);

        char *lastMemLoc = cliBuffer;

        typedef enum {
            REG_STR,
            FORMAT_START,
            LONG_FORMAT,
        } State;

        State state;
        va_start(argumentsPointer, format);

        for (uint32 i = 0, length = strlen(format); i < length; i++) {
            if (state == REG_STR) {

                if (format[i] == '%')
                    state = FORMAT_START;
                else {
                    *(lastMemLoc++) = format[i];
                    *(lastMemLoc) = 0;
                }
            } else if (state == FORMAT_START) {
                if (format[i] == 'd') {
                    sprintf(lastMemLoc, "%d", va_arg(argumentsPointer, int));
                    lastMemLoc += strlen(lastMemLoc);
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                } else if (format[i] == 's') {
                    sprintf(lastMemLoc, "%s", va_arg(argumentsPointer, const char *));
                    lastMemLoc += strlen(lastMemLoc);
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                } else if (format[i] == 'u') {
                    sprintf(lastMemLoc, "%u", va_arg(argumentsPointer, unsigned int));
                    lastMemLoc += strlen(lastMemLoc);
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                } else if (format[i] == 'l') {
                    state = LONG_FORMAT;
                } else {
                    *(lastMemLoc++) = format[i];
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                }
            } else if (state == LONG_FORMAT) {
                if (format[i] == 'd') {
                    sprintf(lastMemLoc, "%lld", va_arg(argumentsPointer, long long int));
                    lastMemLoc += strlen(lastMemLoc);
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                } else if (format[i] == 'u') {
                    sprintf(lastMemLoc, "%llu", va_arg(argumentsPointer, long long unsigned int));
                    lastMemLoc += strlen(lastMemLoc);
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                } else {
                    *(lastMemLoc++) = format[i];
                    *(lastMemLoc) = 0;

                    state = REG_STR;
                }
            }
        }

        printf("%s", cliBuffer);

        fflush(stdout);

        pthread_mutex_unlock(&consoleLock);
    }
}

#endif

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
