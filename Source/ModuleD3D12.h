#pragma once

#include "Module.h"

class ModuleD3D12 : public Module 
{
public:
	ModuleD3D12(HWND hWnd);

	bool init() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;
	
	ID3D12Device2* getDevice() const { return device.Get(); }
	HWND getWindowHandle() const { return hWnd; }
	ID3D12GraphicsCommandList* getCommandList() const { return commandList.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex,rtvDescriptorSize); }

private:
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
	UINT64 fenceValues[FrameCount] = {};
	UINT64 fenceCounter = 0;               
	HANDLE fenceEvent = nullptr;

	bool allowTearing = true;

};