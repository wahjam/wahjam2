// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <assert.h>
#include <atomic>

template<typename T> class RingBuffer
{
public:
    RingBuffer(size_t nelems_ = 0)
        : ring{nullptr}
    {
        setSize(nelems_);
    }

    ~RingBuffer()
    {
        delete [] ring;
    }

    // Resets ring, not atomic!
    void setSize(size_t nelems_)
    {
        // Need to distinguish full and empty ring
        assert(nelems_ < ~(size_t)0);

        delete [] ring;

        ring = new T[nelems_];
        nelems = nelems_;
        reader = 0;
        writer = 0;
    }

    bool canRead() const
    {
//        qDebug("%s ring %p reader %zu writer %zu", __func__, this, reader.load(), writer.load());
        return reader.load() != writer.load();
    }

    T &readCurrent()
    {
        return ring[reader.load() % nelems];
    }

    const T &readCurrent() const
    {
        return ring[reader.load() % nelems];
    }

    void readNext()
    {
        reader.fetch_add(1);
    }

    bool canWrite() const
    {
//        qDebug("%s ring %p reader %zu writer %zu", __func__, this, reader.load(), writer.load());
        return (writer.load() - reader.load()) < nelems;
    }

    T &writeCurrent()
    {
        return ring[writer.load() % nelems];
    }

    void writeNext()
    {
        writer.fetch_add(1);
    }

private:
    T *ring;
    std::atomic<size_t> reader; // free-running index
    std::atomic<size_t> writer; // free-running index
    size_t nelems;
};
