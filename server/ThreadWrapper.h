#ifndef THREAD_WRAPPER_H
#define THREAD_WRAPPER_H

#include "SignalEvent.h"
#include <iostream>

// Base class for thread wrappers
class ThreadWrapper {
public:
    SignalEvent terminationSignal;
    SignalEvent startSignal;
    int exitTimeout;

    // Inline constructor, destructor, and start() method
    ThreadWrapper(int timeout = 1000) : exitTimeout(timeout) {}
    virtual ~ThreadWrapper() {}
    
    // Method to trigger the start signal
    void start() {
        startSignal.trigger();
    }

    // Pure virtual function for thread main logic
    virtual long ThreadMain(void) = 0;
};

// Function that calls the thread's main function
void ThreadFunction(void* obj);

#endif // THREAD_WRAPPER_H
