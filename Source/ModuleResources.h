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

private:
};