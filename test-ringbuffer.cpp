#include <assert.h>
#include <stdio.h>
#include <thread>
#include <future>
#include "RingBuffer.h"

static void testCanWrite()
{
    RingBuffer<char> ring{4};

    for (int i = 0; i < 4; i++) {
        assert(ring.canWrite());
        ring.writeNext();
    }

    assert(!ring.canWrite());
}

static void testCanRead()
{
    RingBuffer<char> ring{4};

    assert(!ring.canRead());

    for (int i = 0; i < 4; i++) {
        ring.writeNext();
    }

    for (int i = 0; i < 4; i++) {
        assert(ring.canRead());
        ring.readNext();
    }

    assert(!ring.canRead());
}

static void testWrapping()
{
    RingBuffer<char> ring{4};

    for (int i = 0; i < 16; i++) {
        char val = 'A' + i;

        ring.writeCurrent() = val;
        ring.writeNext();

        assert(ring.readCurrent() == val);
        ring.readNext();
    }
}

int main(int argc, char **argv)
{
    testCanWrite();
    testCanRead();
    testWrapping();

    printf("ok\n");
    return 0;
}
