#include "SyncObject.h"
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

namespace SyncTools {

SyncObject globalWatcher(0);

PipeAgent::PipeAgent(void) {
    int pipeFD[2];
    pipe(pipeFD);
    setDescriptor(pipeFD[0]);
    sender = pipeFD[1];
}

PipeAgent::PipeAgent(const PipeAgent & p)
    : SyncObject(dup(p.descriptor)), sender(dup(p.sender)) {
}

void PipeAgent::assignFrom(const PipeAgent & p) {
    close(sender);
    close(getDescriptor());
    setDescriptor(p.getDescriptor());
    sender = dup(p.sender);
}

PipeAgent & PipeAgent::operator=(const PipeAgent & p) {
    assignFrom(p);
    return *this;
}

PipeAgent::~PipeAgent(void) {
    close(sender);
    close(getDescriptor());
}

void PipeAgent::waitForByte(void) {
    MultiWait waiter(1, this);
    waiter.wait();
}

char PipeAgent::receiveByte(void) {
    char c;
    read(getDescriptor(), &c, 1);
    return c;
}

void PipeAgent::sendByte(char c) {
    write(sender, &c, 1);
}

// SignalEvent implementations
SignalEvent::SignalEvent(const SignalEvent & e) : PipeAgent(e) { }
SignalEvent & SignalEvent::operator=(const SignalEvent & e) {
    assignFrom(e);
    return *this;
}
void SignalEvent::trigger(void) {
    PipeAgent::sendByte('E');
}
void SignalEvent::wait(void) {
    PipeAgent::waitForByte();
}
void SignalEvent::reset(void) {
    PipeAgent::receiveByte();
}

// ThreadSemaphore implementations
ThreadSemaphore::ThreadSemaphore(int initState) : PipeAgent() {
    for (int i = 0; i < initState; i++)
        signal();
}
ThreadSemaphore::ThreadSemaphore(const ThreadSemaphore & e) : PipeAgent(e) { }
ThreadSemaphore & ThreadSemaphore::operator=(const ThreadSemaphore & e) {
    assignFrom(e);
    return *this;
}
void ThreadSemaphore::wait(void) {
    PipeAgent::waitForByte();
    PipeAgent::receiveByte();
}
void ThreadSemaphore::signal(void) {
    PipeAgent::sendByte();
}

// MultiWait implementations
const int MultiWait::FOREVER = -1;
const int MultiWait::POLL = 0;
MultiWait::MultiWait(int count, ... ) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        objects.push_back(va_arg(args, SyncObject*));
    }
    va_end(args);
}

SyncObject * MultiWait::wait(int timeout) {
    timeval tv;
    int maxFD = 0;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    timeval * pTimeout = (timeout == -1) ? nullptr : &tv;
    
    fd_set readSet;
    FD_ZERO(&readSet);
    for (auto obj : objects) {
        int fd = obj->getDescriptor();
        maxFD = std::max(maxFD, fd);
        FD_SET(fd, &readSet);
    }
    int maxFDArg = maxFD + 1;
    int ready = select(maxFDArg, &readSet, nullptr, nullptr, pTimeout);
    if (ready < 0) {
        perror("select");
        throw std::string("Unexpected error in synchronization object");
    }
    if (ready == 0)
        return nullptr;
    for (auto obj : objects) {
        int fd = obj->getDescriptor();
        if (FD_ISSET(fd, &readSet))
            return obj;
    }
    throw std::string("Unknown error in synchronization object");
}

}; // namespace SyncTools
