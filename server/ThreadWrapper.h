#ifndef THREAD_WRAPPER_H
#define THREAD_WRAPPER_H

#include <thread>
#include "SyncObject.h"

void ThreadFunction(void * param);

class ThreadWrapper {
    friend void ThreadFunction(void * param);
private:
    std::thread workerThread;
    int exitTimeout;
protected:
    SignalEvent terminationSignal;
    SignalEvent startSignal;
    void start();
private:
    ThreadWrapper(const ThreadWrapper &) = delete;
    ThreadWrapper & operator=(const ThreadWrapper &) = delete;
public:
    ThreadWrapper(int timeout = 1000);
    virtual ~ThreadWrapper();
    virtual long ThreadMain(void) = 0;
};

#endif // THREAD_WRAPPER_H
