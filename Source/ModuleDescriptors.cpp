#include "Globals.h"
#include "ModuleDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"

bool ModuleDescriptors::init()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    count = maxDescriptors;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = maxDescriptors;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap))))
        return false;

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
    current = 0;

    return true;
}

bool ModuleDescriptors::cleanUp()
{
    heap.Reset();
    current = 0;
    count = 0;
    descriptorSize = 0;
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleDescriptors::getCpuHandle(uint32_t index) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE h(cpuStart, index, descriptorSize);
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleDescriptors::getGpuHandle(uint32_t index) const
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE h(gpuStart, index, descriptorSize);
    return h;
}

uint32_t ModuleDescriptors::createTexture2DSRV(ID3D12Resource* tex)
{
    _ASSERTE(current < count);
    uint32_t index = current++;

    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    device->CreateShaderResourceView(tex, nullptr, getCpuHandle(index));
    return index;
}

uint32_t ModuleDescriptors::createNullTexture2DSRV()
{
    _ASSERTE(current < count);
    uint32_t index = current++;

    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(nullptr, &srvDesc, getCpuHandle(index));
    return index;
}
