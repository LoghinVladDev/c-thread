#include <stdio.h>
#include <c-thread.h>
#include <unistd.h>

thread_t thread1;
thread_t thread2;
thread_t thread3;

#include <string.h>
#include <stdlib.h>
_Noreturn void threadFunction (void * arg) {

    char buffer [256];
    int counter = 0;

    uint32 strLength;

    getArgument( thread1, "stringLength", & strLength, sizeof( uint32 ) );
    char * message = ( char * ) mallocThreadSafe ( thread1,  strLength + 1 );
    getArgument( thread1, "string", message, strLength + 1 );

    char * msg = (char* ) arg;



    while ( 1 ) {
        lock(thread1);

        sleep(3);

        threadPrintf( "thread_t 1 : %s -> %d ... %s\n", msg, counter++, message );

        unlock(thread1);
    }

}

_Noreturn void thread2f (int argc, char ** argv) {

    sleep(1);
    char buffer [256];
    int counter = 0;


    while ( 1 ) {
        lock(thread2);

        sleep(3);
        sprintf( buffer, "thread_t 2 : %s -> %d\n", argv[0], counter++ );

        threadPutS( buffer );

        unlock(thread2);
    }
}

_Noreturn void thread3f () {

    sleep(1);
    char buffer [256];
    int counter = 0;


    while ( 1 ) {
        lock(thread3);

        sleep(6);
        sprintf( buffer, "thread_t 3 : %d\n", counter++ );

        threadTryPutS( buffer );

        unlock(thread3);
    }
}


int main() {

    initThreadModule();

    createCLIAdminThread();

    char * string = "mesaj1";

    char * thread2Args [] = { "mesaj2" };

    createThreadArgPtr ( & thread1, threadFunction, (void*) string );
    createThreadArgs ( & thread2, thread2f, 1, thread2Args );
//    createThread ( & thread3, thread3f );

    char * str = (char*) malloc ( 256 );
    strcpy( str, "ei na mea" );

    uint32 var = strlen(str);

    setArgument( thread1, "string", str, strlen(str) + 1 );
    setArgument( thread1, "stringLength", & var, sizeof(uint32) );

    startThread( thread1 );

//    uint64 var2 = 46ULL;
//    sint8 var3 = 8;
//    setArgument( thread2, "int", & var2, sizeof(uint64) );
//    setArgument( thread2, "int2", & var3, sizeof(sint8) );

//    startThread( thread2 );

    free ( str );

//    startThread( thread2 );
//    startThread( thread3 );

    while ( isAdminThreadRunning() );

    stopThreadModule();

}