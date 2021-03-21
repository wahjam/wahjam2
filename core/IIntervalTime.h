// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "audio/AudioStream.h" // for SampleTime

/*
 * IIntervalTime provides methods for sample-accurate jam session interval time
 * keeping. This interface allows test cases to provide a mock object that
 * controls timing instead of using a real jam session.
 */
class IIntervalTime
{
public:
    virtual ~IIntervalTime() {}

    // Returns the sample time when the next interval starts
    virtual SampleTime nextIntervalTime() const = 0;

    // Returns the number of samples remaining in this interval given an
    // absolute time
    virtual SampleTime remainingIntervalTime(SampleTime pos) const = 0;
};
