#pragma once

#include <stdint.h>
#include <atomic>
#include <queue>
#include <memory>
#include <utility>
#include <functional>

/*
 * RCUContext is a simplified Read-Copy-Update (RCU) implementation.  It only
 * supports a single reader and a single writer.  The writer also performs
 * reclamation.
 *
 * Readers do not take locks, making RCU relevant for real-time programming
 * tasks where code is not allowed to block.
 *
 * Reader example:
 *
 *   while (true) {
 *       rcu.readLock();
 *       Object *shared = sharedPointer.load();
 *       ...safe to access shared until...
 *       rcu.readUnlock();
 *   }
 *
 * Writer example:
 *
 *   // Make readers see a new object
 *   sharedPointer.store(new Object{ ... });
 *   ...
 *   // Periodically delete old objects no longer accessible after reader
 *   // called rcu.readUnlock()
 *   rcu.reclaim();
 */
class RCUContext
{
public:
    RCUContext();
    ~RCUContext();

    void readLock();
    void readUnlock();

    void addReclaimItem(std::function<void ()> f);
    void reclaim();

private:
    // Incremented by reader, fetched by updater and reclaimer
    std::atomic<uint64_t> gracePeriod;

    // Appended to by updater, emptied by reclaimer
    std::queue<std::pair<uint64_t, std::function<void ()>>> reclaimItems;
};

class RCUReadLocker
{
public:
    RCUReadLocker(RCUContext *rcu_)
        : rcu{rcu_}
    {
        rcu->readLock();
    }

    ~RCUReadLocker()
    {
        rcu->readUnlock();
    }

private:
    RCUContext *rcu;
};

template<
    typename T,
    class Deleter = std::default_delete<T>
> class RCUPointer
{
public:
    explicit RCUPointer(RCUContext *rcu_, T *initial, const Deleter deleter_)
        : rcu{rcu_}, pointer{initial}, deleter{deleter_}
    {
    }

    explicit RCUPointer(RCUContext *rcu_, T *initial)
        : RCUPointer(rcu_, initial, Deleter{})
    {
    }

    /*
     * Copy constructor and assignment operator needed because std::atomic
     * cannot be implicitly copied or assigned.  It's safe to do so for
     * RCUPointer.
     */
    RCUPointer(const RCUPointer<T, Deleter> &other)
        : RCUPointer(other.rcu, other.pointer.load(), other.deleter)
    {
    }

    RCUPointer<T, Deleter> &operator=(const RCUPointer<T, Deleter> &other)
    {
        rcu = other.rcu;
        pointer.store(other.pointer.load());
        deleter = other.deleter;
        return *this;
    }

    T *load() const
    {
        return pointer.load();
    }

    void store(T *newPointer)
    {
        T *oldPointer = pointer.exchange(newPointer);

        /*
         * Need to copy deleter into lambda since this object may not exist
         * when the lambda is invoked.  Could use C++14 lambda capture
         * expressions but stick to C++11 for now using a local variable
         * capture.
         */
        Deleter deleter_ = deleter;
        rcu->addReclaimItem([oldPointer, deleter_]() {
            deleter_(oldPointer);
        });
    }

private:
    RCUContext *rcu;
    std::atomic<T*> pointer;
    Deleter deleter;
};
