#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <new>
#include <Windows.h>

#include "MemoryTracker.h"

#ifdef MEMORY_TRACKING

namespace
{
    struct AllocationHeader;

    struct ThreadRecord
    {
        std::mutex lock;
        AllocationHeader* head = nullptr;
        long long bytes = 0;
        size_t count = 0;
    };

    struct AllocationHeader
    {
        AllocationHeader* prev;
        AllocationHeader* next;
        ThreadRecord* owner;
        void* raw;
        size_t size;
        size_t alignment;
    };

    static_assert(sizeof(AllocationHeader) % 16 == 0, "header must keep 16-byte alignment");

    thread_local ThreadRecord* t_record = nullptr;

    ThreadRecord* GetRecord()
    {
        if (!t_record)
        {
            void* memory = std::malloc(sizeof(ThreadRecord));
            if (!memory)
                std::abort();
            t_record = new (memory) ThreadRecord();
        }
        return t_record;
    }

    void* TrackedAlloc(size_t size, size_t alignment)
    {
        if (alignment < 16)
            alignment = 16;

        void* raw = std::malloc(size + sizeof(AllocationHeader) + alignment);
        if (!raw)
            return nullptr;

        const uintptr_t start = reinterpret_cast<uintptr_t>(raw) + sizeof(AllocationHeader);
        const uintptr_t user = (start + alignment - 1) & ~static_cast<uintptr_t>(alignment - 1);

        auto* header = reinterpret_cast<AllocationHeader*>(user - sizeof(AllocationHeader));
        ThreadRecord* record = GetRecord();

        header->raw = raw;
        header->size = size;
        header->alignment = alignment;
        header->owner = record;

        {
            std::lock_guard<std::mutex> guard(record->lock);
            header->prev = nullptr;
            header->next = record->head;
            if (record->head)
                record->head->prev = header;
            record->head = header;
            record->bytes += static_cast<long long>(size);
            ++record->count;
        }

        return reinterpret_cast<void*>(user);
    }

    void TrackedFree(void* pointer)
    {
        if (!pointer)
            return;

        auto* header = reinterpret_cast<AllocationHeader*>(static_cast<char*>(pointer) - sizeof(AllocationHeader));
        ThreadRecord* record = header->owner;
        void* raw = header->raw;

        {
            std::lock_guard<std::mutex> guard(record->lock);
            if (header->prev)
                header->prev->next = header->next;
            else
                record->head = header->next;
            if (header->next)
                header->next->prev = header->prev;

            record->bytes -= static_cast<long long>(header->size);
            --record->count;
        }

        std::free(raw);
    }
}

long long MemoryTracker::BytesAllocatedThisThread()
{
    ThreadRecord* record = GetRecord();
    std::lock_guard<std::mutex> guard(record->lock);
    return record->bytes;
}

size_t MemoryTracker::AllocationCountThisThread()
{
    ThreadRecord* record = GetRecord();
    std::lock_guard<std::mutex> guard(record->lock);
    return record->count;
}

void MemoryTracker::CreateReport()
{
    ThreadRecord* record = GetRecord();
    std::lock_guard<std::mutex> guard(record->lock);

    char line[160];

    std::snprintf(line, sizeof(line), "=== Memory report (calling thread): %lld bytes in %zu allocations ===\n",
        record->bytes, record->count);
    OutputDebugStringA(line);

    for (AllocationHeader* h = record->head; h; h = h->next)
    {
        std::snprintf(line, sizeof(line), "  addr=%p size=%zu align=%zu\n",
            static_cast<void*>(reinterpret_cast<char*>(h) + sizeof(AllocationHeader)), h->size, h->alignment);
        OutputDebugStringA(line);
    }

    OutputDebugStringA("=== End of report ===\n");
}

void* operator new(size_t size)
{
    void* pointer = TrackedAlloc(size, 16);
    if (!pointer)
        throw std::bad_alloc();
    return pointer;
}

void operator delete(void* pointer) noexcept
{
    TrackedFree(pointer);
}

// Aligned allocation
void* operator new(size_t size, std::align_val_t alignment)
{
    void* pointer = TrackedAlloc(size, static_cast<size_t>(alignment));
    if (!pointer)
        throw std::bad_alloc();
    return pointer;
}

void* operator new[](size_t size, std::align_val_t alignment)
{
    void* pointer = TrackedAlloc(size, static_cast<size_t>(alignment));
    if (!pointer)
        throw std::bad_alloc();
    return pointer;
}

void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return TrackedAlloc(size, static_cast<size_t>(alignment));
}

void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    return TrackedAlloc(size, static_cast<size_t>(alignment));
}

//Aligned deallocation
void operator delete(void* pointer, std::align_val_t) noexcept { TrackedFree(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { TrackedFree(pointer); }
void operator delete(void* pointer, size_t, std::align_val_t) noexcept { TrackedFree(pointer); }
void operator delete[](void* pointer, size_t, std::align_val_t) noexcept { TrackedFree(pointer); }
void operator delete(void* pointer, std::align_val_t, const std::nothrow_t&) noexcept { TrackedFree(pointer); }
void operator delete[](void* pointer, std::align_val_t, const std::nothrow_t&) noexcept { TrackedFree(pointer); }

void MemoryTracker::TestLeak()
{
    int* leaked = new int[16];
    (void)leaked;
}

void MemoryTracker::TestOverflow()
{
    char* buffer = new char[16];
    buffer[16] = 'X';
    delete[] buffer;
}

void MemoryTracker::TestUnderflow()
{
    char* buffer = new char[16];
    buffer[-1] = 'X';
    delete[] buffer;
}
#endif