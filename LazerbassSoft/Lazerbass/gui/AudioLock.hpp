#pragma once

namespace gui {

struct AudioLock {
    void Lock();
    void Unlock();
    void SetRtosSemHandle(void* rtosSemHandle) { rtosSemHandle_ = rtosSemHandle; }
    
    void* rtosSemHandle_;
};

struct AudioLockGuide {
    AudioLockGuide(AudioLock& lock) : lock_(lock) {
        lock.Lock();
    }

    ~AudioLockGuide() {
        lock_.Unlock();
    }

    AudioLock& lock_;
};

}