//
// Created by loghin on 10/15/20.
//

#ifndef C_THREAD_THREAD_CLASS_H
#define C_THREAD_THREAD_CLASS_H

extern "C" {
#include <c-thread.h>
}

#include <exception>

class Thread {
private:
    thread_t        _ID        {NULL_THREAD}; // NOLINT(bugprone-reserved-identifier)
    volatile bool   _isRunning {false};
    static void threadClassCallerFunction( void * ) noexcept;

protected:
    Thread() noexcept = default;

public:
    class Exception : public std::exception {
    public :
        [[nodiscard]] const char * what() const noexcept override{
            return "Undefined Thread Exception";
        }
    };

    class ModuleInitializationFailure : public Thread::Exception {
        [[nodiscard]] const char * what() const noexcept override {
            return "Thread Module could not be Initialized";
        }
    };

    virtual void run() = 0;

    void start() noexcept(false);

    [[nodiscard]] bool isRunning () const noexcept {
        return this->_isRunning;
    }
};


#endif //C_THREAD_THREAD_CLASS_H
