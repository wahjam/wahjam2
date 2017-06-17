#include "rcu.h"

RCUContext::RCUContext()
    : gracePeriod{0}
{
}

RCUContext::~RCUContext()
{
    // Just in case reclaim items were added after last readUnlock()
    gracePeriod.fetch_add(1);

    reclaim();
}

void RCUContext::readLock()
{
}

void RCUContext::readUnlock()
{
    gracePeriod.fetch_add(1);
}

void RCUContext::addReclaimItem(std::function<void ()> f)
{
    uint64_t after = gracePeriod.load();
    reclaimItems.push(std::make_pair(after, f));
}

void RCUContext::reclaim()
{
    uint64_t now = gracePeriod.load();

    while (!reclaimItems.empty()) {
        const auto &item{reclaimItems.front()};

        // Only reclaim items whose grace period has expired
        if (item.first == now) {
            return;
        }

        item.second();
        reclaimItems.pop();
    }
}
