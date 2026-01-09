#pragma once

class FreeListAllocator {
public:
    void init(uint32_t capacity, uint32_t firstUsable = 1)
    {
        assert(capacity > firstUsable);

        this->capacity = capacity;
        this->first = firstUsable;

        next.resize(capacity);

        for (uint32_t i = first; i < capacity - 1; ++i)
            next[i] = i + 1;

        next[capacity - 1] = UINT32_MAX;
        head = first;
    }

    uint32_t alloc()
    {
        assert(head != UINT32_MAX && "Out of handles in FreeListAllocator!");
        const uint32_t idx = head;
        head = next[idx];
        return idx;
    }

    void free(uint32_t index)
    {
        if (index < first || index >= capacity)
            return;

        next[index] = head;
        head = index;
    }

    void reset()
    {
        next.clear();
        head = UINT32_MAX;
        capacity = 0;
        first = 1;
    }

private:
    std::vector<uint32_t> next;
    uint32_t head = UINT32_MAX;
    uint32_t capacity = 0;
    uint32_t first = 1;
};
