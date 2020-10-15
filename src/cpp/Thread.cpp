//
// Created by loghin on 10/15/20.
//

#include "Thread.h"

void Thread::start() noexcept(false){
    if ( initThreadModule() != ThreadResult::THREAD_SUCCESS )
        throw Thread::ModuleInitializationFailure();

    createThreadArgPtr( & this->_ID, threadClassCallerFunction, (void*) this);

    this->_isRunning = true;
    startThread( this->_ID );
}

void Thread::threadClassCallerFunction(void * pObj) noexcept {
    ( ( Thread * ) ( pObj ) )->run();
    ( ( Thread * ) ( pObj ) )->_isRunning = false;
}