// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <stdio.h>
#include <thread>
#include <future>
#include <functional>
#include "rcu.h"

static void testPointer()
{
    RCUContext rcu;
    RCUPointer<unsigned> pointer{&rcu, new unsigned{0xcafebabe}};

    std::promise<void> barrier1;
    std::future<void> barrier1_future = barrier1.get_future();
    std::promise<void> barrier2;
    std::future<void> barrier2_future = barrier2.get_future();

    std::thread readerThread{
        [&rcu, &pointer, &barrier1, &barrier2_future]() {
            rcu.readLock();
            assert(*pointer.load() == 0xcafebabe);
            rcu.readUnlock();

            barrier1.set_value();
            barrier2_future.wait();

            rcu.readLock();
            assert(*pointer.load() == 0xfeedface);
            rcu.readUnlock();
        }
    };

    rcu.reclaim();

    barrier1_future.wait();

    pointer.store(new unsigned{0xfeedface});
    rcu.reclaim();

    barrier2.set_value();

    readerThread.join();

    rcu.reclaim();

    pointer.store(nullptr);
}

static void testDeleter()
{
    bool deleted = false;

    {
        RCUContext rcu;
        RCUPointer<int, std::function<void (int *)>> pointer{&rcu, new int,
            [&deleted](int *i) {
                delete i;
                deleted = true;
            }
        };

        pointer.store(nullptr);
    }

    assert(deleted);
}

int main(int argc, char **argv)
{
    testPointer();
    testDeleter();

    printf("ok\n");
    return 0;
}
