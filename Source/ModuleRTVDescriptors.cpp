#include "Globals.h"
#include "ModuleRTVDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "d3dx12.h"

bool ModuleRTVDescriptors::init()
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = maxDescriptors;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap))))
        return false;

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();

    allocator.init(maxDescriptors);
    return true;
}

bool ModuleRTVDescriptors::cleanUp()
{
    heap.Reset();
    descriptorSize = 0;
    allocator.reset();
    return true;
}

uint32_t ModuleRTVDescriptors::createRTV(ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
    uint32_t index = allocator.alloc();

    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    device->CreateRenderTargetView(res, desc, getCpuHandle(index));
    return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleRTVDescriptors::getCpuHandle(uint32_t index) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, (int)index, (int)descriptorSize);
}
