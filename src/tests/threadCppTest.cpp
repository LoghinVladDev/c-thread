//
// Created by loghin on 10/15/20.
//

#include <Thread.h>
#include <iostream>
#include <unistd.h>

class MyThread : public Thread {
    void run() override {
        while(true) {
            static uint8 loopCount = 0;
            std::cout << "thread1\n";

            sleep(3);

            if ( loopCount ++ >= 3 )
                break;
        }
    }
};

int main() {
    MyThread thread;

    thread.start();

    while( thread.isRunning() );

    return 0;
}