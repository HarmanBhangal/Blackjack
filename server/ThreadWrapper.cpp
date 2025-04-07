#include "ThreadWrapper.h"
#include <unistd.h>
#include <iostream>

ThreadWrapper::ThreadWrapper(int timeout)
    : exitTimeout(timeout), workerThread(ThreadFunction, this) {
}

void ThreadWrapper::start() {
    startSignal.trigger();
}

ThreadWrapper::~ThreadWrapper() {
    MultiWait endWait(1, &terminationSignal);
    if (endWait.wait(exitTimeout)) {
        workerThread.join();
    } else {
        std::cout << "Thread: Failed to terminate properly" << std::endl;
    }
}

void ThreadFunction(void * param) {
    ThreadWrapper * threadObj = static_cast<ThreadWrapper*>(param);
    threadObj->startSignal.wait();
    try {
        threadObj->ThreadMain();
    } catch (SyncTools::TerminationCode) {
        ;
    }
    threadObj->terminationSignal.trigger();
}
