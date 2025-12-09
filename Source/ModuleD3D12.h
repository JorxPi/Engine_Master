#pragma once

#include "Module.h"
#include "Timer.h"

class ModuleD3D12 : public Module 
{
public:
	ModuleD3D12(HWND hWnd);

	bool init() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;
	void requestResize(UINT width, UINT height);
	void flush();

	//GETTERS
	
	ID3D12Device2* getDevice() const { return device.Get(); }
	HWND getWindowHandle() const { return hWnd; }
	ID3D12GraphicsCommandList* getCommandList() const { return commandList.Get(); }
	ID3D12CommandAllocator* getCommandAllocator() const { return commandAllocator.Get(); }
	ID3D12CommandQueue* getCommandQueue() const { return commandQueue.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex,rtvDescriptorSize); }
	D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const{ return dsvHeap->GetCPUDescriptorHandleForHeapStart(); }
	ID3D12Resource* getCurrentRenderTarget() const { return renderTargets[frameIndex].Get(); }

private:
	void resize();
	void createDepthBuffer(UINT width, UINT height);
	void createRtvHandle();

	HWND hWnd;

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory; 
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter; 
	Microsoft::WRL::ComPtr<ID3D12Device5> device;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	static const UINT FrameCount = 2;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[FrameCount];

	UINT rtvDescriptorSize = 0;
	UINT frameIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 fenceCounter = 0;               
	HANDLE fenceEvent = nullptr;

	bool pendingResize = false;
	UINT newWidth = 0;
	UINT newHeight = 0;

	Timer timer;

	ComPtr<ID3D12Resource> depthStencilBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

};