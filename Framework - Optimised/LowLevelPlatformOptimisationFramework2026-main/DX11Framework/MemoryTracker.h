#pragma once

#include <cstddef>

#ifdef _DEBUG
#define MEMORY_TRACKING 1
#endif

#ifdef MEMORY_TRACKING

namespace MemoryTracker
{
    long long BytesAllocatedThisThread();
    size_t AllocationCountThisThread();

    void TestLeak();
    void TestOverflow();
    void TestUnderflow();

    void CreateReport();
}

#endif