#pragma once
#include "Module.h"

class ModuleDescriptors : public Module
{
public:
    bool init() override; 
    bool cleanUp() override;

    uint32_t createTexture2DSRV(ID3D12Resource* tex);
    uint32_t createNullTexture2DSRV();

    ID3D12DescriptorHeap* getHeap() const { return heap.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;

private:
    ComPtr<ID3D12DescriptorHeap> heap;
    uint32_t descriptorSize = 0;
    uint32_t count = 0;
    uint32_t current = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{};

    uint32_t maxDescriptors = 2048;
};
