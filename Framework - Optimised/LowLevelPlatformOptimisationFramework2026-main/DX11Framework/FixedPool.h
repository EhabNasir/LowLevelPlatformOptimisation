#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <new>

template<typename T, size_t SlotsPerChunk, size_t MaxChunks = 16>
class FixedPool
{
    static constexpr size_t slotAlign = alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
    static constexpr size_t slotSize = (sizeof(T) + slotAlign - 1) / slotAlign * slotAlign;

    static_assert(slotSize >= sizeof(void*), "slot must hold the free-list pointer");
    static_assert(slotAlign <= alignof(std::max_align_t), "over-aligned types need aligned malloc");

public:
    FixedPool() = default;

    ~FixedPool()
    {
        for (size_t i = 0; i < _chunkCount; ++i)
            std::free(_chunks[i]);
    }

    FixedPool(const FixedPool&) = delete;
    FixedPool& operator=(const FixedPool&) = delete;

    void* Allocate()
    {
        if (!_freeHead && !Grow())
            return nullptr;

        Slot* slot = _freeHead;
        _freeHead = slot->next;
        ++_inUse;
        return slot;
    }

    void Free(void* pointer)
    {
        Slot* slot = static_cast<Slot*>(pointer);
        slot->next = _freeHead;
        _freeHead = slot;
        --_inUse;
    }

    bool Contains(const void* pointer) const
    {
        const char* p = static_cast<const char*>(pointer);
        for (size_t i = 0; i < _chunkCount; ++i)
            if (p >= _chunks[i] && p < _chunks[i] + slotSize * SlotsPerChunk)
                return true;
        return false;
    }

    size_t InUse() const { return _inUse; }
    size_t ChunkCount() const { return _chunkCount; }
    size_t Capacity() const { return _chunkCount * SlotsPerChunk; }

private:
    struct Slot { Slot* next; };

    bool Grow()
    {
        if (_chunkCount == MaxChunks)
            return false;

        char* chunk = static_cast<char*>(std::malloc(slotSize * SlotsPerChunk));
        if (!chunk)
            return false;

        _chunks[_chunkCount++] = chunk;

        for (size_t i = SlotsPerChunk; i-- > 0;)
        {
            Slot* slot = reinterpret_cast<Slot*>(chunk + i * slotSize);
            slot->next = _freeHead;
            _freeHead = slot;
        }
        return true;
    }

    char* _chunks[MaxChunks] = {};
    size_t _chunkCount = 0;
    Slot* _freeHead = nullptr;
    size_t _inUse = 0;
};

template<typename T, size_t SlotsPerChunk = 128>
struct Pooled
{
    static void* operator new(size_t size)
    {
        assert(size == sizeof(T));
        if (void* p = Pool().Allocate())
            return p;
        ++Fallbacks();
        return ::operator new(size);
    }

    static void operator delete(void* pointer, size_t size)
    {
        if (!pointer) return;
        if (Pool().Contains(pointer)) Pool().Free(pointer);
        else ::operator delete(pointer, size);
    }

    static size_t FallbackCount() { return Fallbacks(); }
    static size_t PoolChunks() { return Pool().ChunkCount(); }

private:
    static FixedPool<T, SlotsPerChunk>& Pool() { thread_local FixedPool<T, SlotsPerChunk> pool; return pool; }
    static size_t& Fallbacks() { thread_local size_t count = 0; return count; }
};