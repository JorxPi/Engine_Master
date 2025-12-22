#pragma once
#include "Module.h"

struct RingAllocation
{
    void* cpu = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
    uint32_t size = 0;
};

class ModuleRingBuffer : public Module
{
public:
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    RingAllocation allocBuffer(uint32_t sizeBytes);
    RingAllocation commitRingAlloc(uint32_t allocOffset, uint32_t size, uint32_t frameIdx);

private:
    ComPtr<ID3D12Resource> buffer;
    uint32_t mbMemory10 = 10 * 1024 * 1024;
    uint8_t* mapped = nullptr;

    uint32_t totalSize = 0;
    uint32_t head = 0;
    uint32_t tail = 0;
    uint32_t totalAllocated = 0;

    // one counter per backbuffer
    std::vector<uint32_t> allocatedPerFrame;

};