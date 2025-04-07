#include "ThreadWrapper.h"

void ThreadFunction(void* obj) {
    ThreadWrapper* threadObj = static_cast<ThreadWrapper*>(obj);
    // Wait until the thread is signaled to start
    threadObj->startSignal.wait();
    try {
        threadObj->ThreadMain();
    } catch (...) {
        // Optionally handle exceptions here
    }
    // Signal that the thread has terminated
    threadObj->terminationSignal.trigger();
}
