#ifndef SYNC_OBJECT_H
#define SYNC_OBJECT_H

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

namespace SyncTools {

typedef int TerminationCode;

class SyncObject {
protected:
    int descriptor;
public:
    SyncObject(int d = 0) : descriptor(d) { }
    SyncObject(const SyncObject & other) : descriptor(dup(other.descriptor)) { }
    virtual ~SyncObject(void) { }
    operator int(void) const { return descriptor; }
    void setDescriptor(int d) { descriptor = d; }
    int getDescriptor(void) const { return descriptor; }
};

extern SyncObject globalWatcher;

class PipeAgent : public SyncObject {
private:
    int sender;
protected:
    PipeAgent(void);
    PipeAgent(const PipeAgent &);
    PipeAgent & operator=(const PipeAgent &);
    void assignFrom(const PipeAgent &);
    ~PipeAgent(void);
    void waitForByte(void);
    void sendByte(char c = 'P');
    char receiveByte(void);
};

class SignalEvent : public PipeAgent {
public:
    SignalEvent(void) { }
    ~SignalEvent(void) { }
    SignalEvent(const SignalEvent &);
    SignalEvent & operator=(const SignalEvent &);
    void trigger(void);
    void wait(void);
    void reset(void);
};

class ThreadSemaphore : public PipeAgent {
public:
    ThreadSemaphore(int initState = 0);
    ThreadSemaphore(const ThreadSemaphore &);
    ~ThreadSemaphore(void) { }
    ThreadSemaphore & operator=(const ThreadSemaphore &);
    void wait(void);
    void signal(void);
};

class MultiWait {
public:
    static const int FOREVER; // == -1
    static const int POLL;    // == 0
private:
    std::vector<SyncObject*> objects;
public:
    MultiWait(int count, ...);
    SyncObject * wait(int timeout = -1);
};

}; // namespace SyncTools

#endif // SYNC_OBJECT_H
