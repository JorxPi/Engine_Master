#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"

ModuleD3D12::ModuleD3D12(HWND windowHandle)
    : hWnd(windowHandle) 
{
}

bool ModuleD3D12::init() {
    timer.start();

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    if (SUCCEEDED(hr) && debugInterface)
    {
        debugInterface->EnableDebugLayer();
        LOG("D3D12 debug layer enabled");
    }
    else
    {
        LOG("WARNING: D3D12GetDebugInterface failed (hr=0x%08X). Debug layer not available.", (unsigned)hr);
    }
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
swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

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

createRtvHandle();

//DepthStencilBuffer

D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
dsvHeapDesc.NumDescriptors = 1;
dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

UINT width, height;
{
    DXGI_SWAP_CHAIN_DESC scDesc;
    swapChain->GetDesc(&scDesc);
    width = scDesc.BufferDesc.Width;
    height = scDesc.BufferDesc.Height;
}

// Create depth texture
createDepthBuffer(width, height);

device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
fenceCounter = 0;
fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

//Command allocator
device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

//Command list
device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
commandList->Close();

LOG("Device initialized successfully.");

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    device.As(&infoQueue);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

timer.stop();
LOG("Time to initialize module D3D12: %lld", timer.read());

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

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

    FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void ModuleD3D12::render() {
    CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrierToPresent);

    commandList->Close();
}

void ModuleD3D12::postRender() {
    ID3D12CommandList* listsToExecute[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(listsToExecute), listsToExecute);

    swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

    flush();

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

bool ModuleD3D12::cleanUp() {
    if (commandQueue && fence) {
        flush();
    }

    if (fenceEvent) {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }

    swapChain.Reset();
    commandQueue.Reset();
    depthStencilBuffer.Reset();
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

    flush();

    for (UINT i = 0; i < FrameCount; ++i)
        renderTargets[i].Reset();
    depthStencilBuffer.Reset();
    commandAllocator.Reset();
    commandList.Reset();

    DXGI_SWAP_CHAIN_DESC desc = {};
    swapChain->GetDesc(&desc);
    swapChain->ResizeBuffers(FrameCount, newWidth, newHeight, desc.BufferDesc.Format, desc.Flags);

    createRtvHandle();

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    createDepthBuffer(newWidth, newHeight);

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();

    pendingResize = false;
}

void ModuleD3D12::flush() {
    const UINT64 fenceToWait = ++fenceCounter;
    commandQueue->Signal(fence.Get(), fenceToWait);
    if (fence->GetCompletedValue() < fenceToWait) {
        fence->SetEventOnCompletion(fenceToWait, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

void ModuleD3D12::createDepthBuffer(UINT width, UINT height)
{
    depthStencilBuffer.Reset();

    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthStencilBuffer)
    );

    device->CreateDepthStencilView(
        depthStencilBuffer.Get(),
        nullptr,
        dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );
}

void ModuleD3D12::createRtvHandle()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT n = 0; n < FrameCount; n++)
    {
        swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
        device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }
}

