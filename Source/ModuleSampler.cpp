#include "Globals.h"
#include "Application.h"
#include "ModuleSampler.h"
#include "ModuleD3D12.h"

bool ModuleSampler::init()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = samplerCount;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&samplerHeap));

    samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    createDefaultSamplers(device);

    return true;
}

bool ModuleSampler::cleanUp()
{
    samplerHeap.Reset();
    samplerDescriptorSize = 0;
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleSampler::getCpuHandle(UINT index) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(samplerHeap->GetCPUDescriptorHandleForHeapStart(), index,
        samplerDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleSampler::getGpuHandle(UINT index) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(samplerHeap->GetGPUDescriptorHandleForHeapStart(), index,
        samplerDescriptorSize);
}

void ModuleSampler::createDefaultSamplers(ID3D12Device* device)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = samplerHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SAMPLER_DESC samplers[samplerCount] = {};

    // Linear Wrap
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplers[0].BorderColor[0] = 0.0f;
    samplers[0].BorderColor[1] = 0.0f;
    samplers[0].BorderColor[2] = 0.0f;
    samplers[0].BorderColor[3] = 0.0f;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;

    // Linear Clamp
    samplers[1] = samplers[0];
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    // Point Wrap
    samplers[2] = samplers[0];
    samplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

    // Point Clamp
    samplers[3] = samplers[2];
    samplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    for (UINT i = 0; i < samplerCount; ++i)
    {
        device->CreateSampler(&samplers[i], cpuHandle);
        cpuHandle.ptr += samplerDescriptorSize;
    }
}