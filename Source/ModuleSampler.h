#pragma once

#include "Module.h"

class ModuleSampler : public Module
{
public:
    bool init() override;
    bool cleanUp() override;

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(UINT index) const;

    UINT getCount() const { return samplerCount; }
    ID3D12DescriptorHeap* getHeap() const { return samplerHeap.Get(); }

private:
    void createDefaultSamplers(ID3D12Device* device);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerHeap;
    UINT samplerDescriptorSize = 0;
    static constexpr UINT samplerCount = 4;
};