#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"

ModuleD3D12::ModuleD3D12(HWND windowHandle)
    : hWnd(windowHandle) 
{
}

bool ModuleD3D12::init() {
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
    OutputDebugString(L"[D3D12] Debug layer enabled\n");
#endif

#if defined(_DEBUG)
    CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
#else
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));

//Command queue
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
queueDesc.NodeMask = 0;

device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

//Swap chain
DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
swapChainDesc.BufferCount = FrameCount;
swapChainDesc.Width = 0; // 0 is the same as writing the variable of window size
swapChainDesc.Height = 0; // Same here
swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

swapChainDesc.Stereo = FALSE;
swapChainDesc.SampleDesc = { 1, 0 };
swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

swapChainDesc.Scaling = DXGI_SCALING_NONE;
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

ComPtr<IDXGISwapChain1> tempSwapChain;
factory->CreateSwapChainForHwnd(commandQueue.Get(), (HWND)hWnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

factory->MakeWindowAssociation((HWND)hWnd, DXGI_MWA_NO_ALT_ENTER);

tempSwapChain.As(&swapChain);

frameIndex = swapChain->GetCurrentBackBufferIndex();

D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
rtvHeapDesc.NumDescriptors = FrameCount;
rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

for (UINT n = 0; n < FrameCount; n++)
{
    swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
    device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, rtvDescriptorSize);
}

device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
fenceCounter = 0;
fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

for (UINT i = 0; i < FrameCount; ++i) fenceValues[i] = 0;

//Command allocator
device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

//Command list
device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
commandList->Close();

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    device.As(&infoQueue);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

    return true;
}

void ModuleD3D12::preRender() {
    if (pendingResize) {
        resize();
    }

    commandAllocator->Reset();

    commandList->Reset(commandAllocator.Get(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrierToRT);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

    FLOAT clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void ModuleD3D12::render() {
    CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrierToPresent);

    commandList->Close();
}

void ModuleD3D12::postRender() {
    ID3D12CommandList* listsToExecute[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(listsToExecute), listsToExecute);

    if (allowTearing)
        swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    else
        swapChain->Present(1, 0);

    const UINT64 currentFence = ++fenceCounter;
    commandQueue->Signal(fence.Get(), currentFence);

    if (fence->GetCompletedValue() < currentFence)
    {
        fence->SetEventOnCompletion(currentFence, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

bool ModuleD3D12::cleanUp() {
    if (commandQueue && fence) {
        const UINT64 fenceToWait = ++fenceCounter;
        commandQueue->Signal(fence.Get(), fenceToWait);
        if (fence->GetCompletedValue() < fenceToWait) {
            fence->SetEventOnCompletion(fenceToWait, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }

    swapChain.Reset();
    commandQueue.Reset();
    commandAllocator.Reset();
    commandList.Reset();
    rtvHeap.Reset();

    for (UINT i = 0; i < FrameCount; ++i) {
        renderTargets[i].Reset();
    }

    fence.Reset();
    device.Reset();
    adapter.Reset();
    factory.Reset();

    return true;
}

void ModuleD3D12::requestResize(UINT width, UINT height) {
    pendingResize = true;
    newWidth = width;
    newHeight = height;
}

void ModuleD3D12::resize()
{
    if (!swapChain)
        return;

    // Wait until GPU finishes current work
    const UINT64 fenceToWait = ++fenceCounter;
    commandQueue->Signal(fence.Get(), fenceToWait);
    if (fence->GetCompletedValue() < fenceToWait) {
        fence->SetEventOnCompletion(fenceToWait, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // Release old resources
    for (UINT i = 0; i < FrameCount; ++i)
        renderTargets[i].Reset();
    commandAllocator.Reset();
    commandList.Reset();

    // Resize the swap chain
    DXGI_SWAP_CHAIN_DESC desc = {};
    swapChain->GetDesc(&desc);
    swapChain->ResizeBuffers(FrameCount, newWidth, newHeight, desc.BufferDesc.Format, desc.Flags);

    // Recreate RTVs
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT n = 0; n < FrameCount; ++n) {
        swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
        device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();

    pendingResize = false;
}

