#ifndef POSIX_SEMAPHORE_H
#define POSIX_SEMAPHORE_H

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>

class PosixSemaphore {
private:
    sem_t * semPtr;
    bool isOwner;
    std::string semName;
public:
    PosixSemaphore(const std::string & name, int initState = 0, bool ownerFlag = false)
      : isOwner(ownerFlag), semName("/" + name)
    {
        if (ownerFlag) {
            sem_unlink(semName.c_str());
            semPtr = sem_open(semName.c_str(), O_CREAT, 0666, initState);
        } else {
            semPtr = sem_open(semName.c_str(), 0);
        }
        if (semPtr == SEM_FAILED)
            throw std::string("Failed to create semaphore");
    }
    virtual ~PosixSemaphore() {
        sem_close(semPtr);
        if (isOwner)
            sem_unlink(semName.c_str());
    }
    void wait() { sem_wait(semPtr); }
    void signal() { sem_post(semPtr); }
};

#endif // POSIX_SEMAPHORE_H
