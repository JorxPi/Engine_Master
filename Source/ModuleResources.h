#pragma once

#include "Module.h"

using Microsoft::WRL::ComPtr;

class ModuleResources : public Module
{
public:
	ModuleResources();

	ComPtr<ID3D12Resource> createUploadBuffer(const void* data, UINT64 sizeInBytes);

	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, UINT64 sizeInBytes);

	ComPtr<ID3D12Resource> createTextureFromFile(const wchar_t* filePath);

	ComPtr<ID3D12Resource> createRenderTarget(DXGI_FORMAT format, UINT width, UINT height, const float clearColor[4]);

	ComPtr<ID3D12Resource> createDepthStencil(DXGI_FORMAT format, UINT width, UINT height, float clearDepth = 1.0f, UINT8 clearStencil = 0);

private:
};