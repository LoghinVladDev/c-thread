#include <stdio.h>
#include <c-thread.h>
#include <unistd.h>

Thread thread1;
Thread thread2;
Thread thread3;

_Noreturn void threadFunction (void * arg) {

    char buffer [256];
    int counter = 0;

    char * msg = (char* ) arg;

    while ( 1 ) {
        lock(thread1);

        sleep(3);
        sprintf( buffer, "Thread 1 : %s -> %d\n", msg, counter++ );

        threadTryPutS( buffer );

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
        sprintf( buffer, "Thread 2 : %s -> %d\n", argv[0], counter++ );

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
        sprintf( buffer, "Thread 3 : %d\n", counter++ );

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
//    createThreadArgs ( & thread2, thread2f, 1, thread2Args );
//    createThread ( & thread3, thread3f );

    startThread( thread1 );
//    startThread( thread2 );
//    startThread( thread3 );

    while ( isAdminThreadRunning() );

    stopThreadModule();

}