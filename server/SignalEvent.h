#ifndef SIGNAL_EVENT_H
#define SIGNAL_EVENT_H

#include <mutex>
#include <condition_variable>

class SignalEvent {
public:
    std::mutex mtx;
    std::condition_variable cv;
    bool signaled = false;

    // Trigger the event and notify waiting threads.
    void trigger() {
        std::lock_guard<std::mutex> lock(mtx);
        signaled = true;
        cv.notify_all();
    }

    // Wait until the event is triggered.
    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return signaled; });
        // Reset after waiting so it can be reused.
        signaled = false;
    }
};

#endif // SIGNAL_EVENT_H
