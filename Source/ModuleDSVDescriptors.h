#pragma once
#include "Module.h"
#include "FreeListAllocator.h"

class ModuleDSVDescriptors : public Module
{
public:
    bool init() override;
    bool cleanUp() override;

    uint32_t createDSV(ID3D12Resource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr);

    void freeIndex(uint32_t idx) { allocator.free(idx); }
    uint32_t allocIndex() { return allocator.alloc(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;

private:
    FreeListAllocator allocator;
    ComPtr<ID3D12DescriptorHeap> heap;
    uint32_t descriptorSize = 0;

    uint32_t maxDescriptors = 256;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{};
};