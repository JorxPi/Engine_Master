#include "Globals.h"
#include "ModuleRingBuffer.h"
#include "Application.h"
#include "ModuleD3D12.h"


bool ModuleRingBuffer::init()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    totalSize = (uint32_t)alignUp((size_t)mbMemory10, (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    allocatedPerFrame.clear();
    allocatedPerFrame.resize(modD3D12->getFrameCount(), 0);

    head = 0;
    tail = 0;
    totalAllocated = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(totalSize);

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer))))
        return false;

    void* ptr = nullptr;
    if (FAILED(buffer->Map(0, nullptr, &ptr)))
        return false;

    mapped = reinterpret_cast<uint8_t*>(ptr);

    return true;
}

void ModuleRingBuffer::preRender()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();

    const uint32_t idx = (uint32_t)modD3D12->getFrameIndex();

    if (allocatedPerFrame[idx] == 0)
        return;

    tail += allocatedPerFrame[idx];
    if (tail >= totalSize)
        tail -= totalSize;

    totalAllocated -= allocatedPerFrame[idx];

    allocatedPerFrame[idx] = 0;
}

bool ModuleRingBuffer::cleanUp()
{
    mapped = nullptr;
    buffer.Reset();

    allocatedPerFrame.clear();

    totalSize = 0;
    head = 0;
    tail = 0;
    totalAllocated = 0;

    return true;
}

RingAllocation ModuleRingBuffer::allocBuffer(uint32_t sizeBytes)
{
    RingAllocation ringAlloc{};

    uint32_t size = (uint32_t)alignUp(sizeBytes, (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (size > totalSize)
        return ringAlloc;

    auto modD3D12 = app->getModule<ModuleD3D12>();

    const uint32_t frameIdx = (uint32_t)modD3D12->getFrameIndex();

    // Case A
    if (tail > head)
    {
        const uint32_t freeFromStart = tail - head;
        if (size <= freeFromStart)
        {
            return commitRingAlloc(head, size, frameIdx);
        }
        return ringAlloc; 
    }

    // Case B
    if (tail < head)
    {
        const uint32_t freeToEnd = totalSize - head;

        if (size <= freeToEnd)
        {
            return commitRingAlloc(head, size, frameIdx);
        }
        else
        {
            allocatedPerFrame[frameIdx] += freeToEnd;
            totalAllocated += freeToEnd;
            head = 0;

            if (tail > head)
            {
                const uint32_t freeFromStart = tail - head;
                if (size <= freeFromStart)
                {
                    return commitRingAlloc(head, size, frameIdx);
                }
            }
            return ringAlloc;
        }
    }

    // Case C
    if (tail == head)
    {
        if (totalAllocated == 0)
        {
            return commitRingAlloc(head, size, frameIdx);
        }
        else
        {
            return ringAlloc;
        }
    }

    return ringAlloc;
}

RingAllocation ModuleRingBuffer::commitRingAlloc(uint32_t allocOffset, uint32_t size, uint32_t frameIdx)
{
    RingAllocation r{};
    r.cpu = mapped + allocOffset;
    r.gpu = buffer->GetGPUVirtualAddress() + allocOffset;
    r.size = size;

    head = allocOffset + size;
    if (head >= totalSize) head -= totalSize;

    totalAllocated += size;
    allocatedPerFrame[frameIdx] += size;
    return r;
}