#include "Globals.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleD3D12.h"
#include <DirectXTex.h>
#include "d3dx12.h"

ModuleResources::ModuleResources() {

}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(const void* data, UINT64 sizeInBytes) {
	auto modD3D12 = app->getModule<ModuleD3D12>();
	ID3D12Device* device = modD3D12->getDevice();

	ComPtr<ID3D12Resource> uploadBuffer;

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer))))
        return nullptr;

	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, static_cast<size_t>(sizeInBytes));
	uploadBuffer->Unmap(0, nullptr);

	return uploadBuffer;
}

ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(const void* data, UINT64 sizeInBytes) {
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();
    ID3D12GraphicsCommandList* cmdl = modD3D12->getCommandList();
	ID3D12CommandAllocator* alloc = modD3D12->getCommandAllocator();

    ComPtr<ID3D12Resource> defaultBuffer;
    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

    if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer))))
        return nullptr;

    ComPtr<ID3D12Resource> uploadBuffer = createUploadBuffer(data, sizeInBytes);

    alloc->Reset();
    cmdl->Reset(alloc, nullptr);
    cmdl->CopyResource(defaultBuffer.Get(), uploadBuffer.Get());
    cmdl->Close();

    ID3D12CommandList* lists[] = { cmdl };
    modD3D12->getCommandQueue()->ExecuteCommandLists(1, lists);
    modD3D12->flush();

    return defaultBuffer;
}

ComPtr<ID3D12Resource> ModuleResources::createTextureFromFile(const wchar_t* filePath)
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();
    ID3D12GraphicsCommandList* cmdl = modD3D12->getCommandList();
    ID3D12CommandAllocator* alloc = modD3D12->getCommandAllocator();

    DirectX::ScratchImage image;
    if (FAILED(LoadFromDDSFile(filePath, DDS_FLAGS_NONE, nullptr, image)))
    {
        if (FAILED(LoadFromTGAFile(filePath, nullptr, image)))
        {
            if (FAILED(LoadFromWICFile(filePath, WIC_FLAGS_NONE, nullptr, image)))
                return nullptr;
        }
    }

    DirectX::TexMetadata metaData = image.GetMetadata();

    //mipmap generation
    if (metaData.mipLevels <= 1 && metaData.width > 1 && metaData.height > 1)
    {
        LOG("This image don't have mipmaps originally");
        DirectX::ScratchImage mipChain;

        DirectX::TEX_FILTER_FLAGS filter = DirectX::TEX_FILTER_DEFAULT;
        if (DirectX::IsSRGB(metaData.format))
            filter = (DirectX::TEX_FILTER_FLAGS)(filter | DirectX::TEX_FILTER_SRGB);

        if (SUCCEEDED(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), metaData, filter, 0, mipChain)))
        {
            LOG("Mipmaps generated succesfully");
            image = std::move(mipChain);
            metaData = image.GetMetadata();
        }
    }

    DXGI_FORMAT texFormat = DirectX::MakeSRGB(metaData.format);

    D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, UINT64(metaData.width), UINT(metaData.height), UINT16(metaData.arraySize), UINT16(metaData.mipLevels));
    CD3DX12_HEAP_PROPERTIES texHeap(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> texture;

    if (FAILED(device->CreateCommittedResource(&texHeap, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture))))
        return nullptr;

    const UINT imgCount = image.GetImageCount();
    const UINT64 size = GetRequiredIntermediateSize(texture.Get(), 0, imgCount);

    CD3DX12_RESOURCE_DESC buffDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    CD3DX12_HEAP_PROPERTIES buffHeap(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> intermediate;

    if (FAILED(device->CreateCommittedResource(&buffHeap, D3D12_HEAP_FLAG_NONE, &buffDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediate)))) 
        return nullptr;

    std::vector<D3D12_SUBRESOURCE_DATA> subData;
    subData.reserve(imgCount);

    for (size_t item = 0; item < metaData.arraySize; ++item)
    {
        for (size_t level = 0; level < metaData.mipLevels; ++level)
        {
            const DirectX::Image* subImg = image.GetImage(level, item, 0);
            D3D12_SUBRESOURCE_DATA data = { subImg->pixels, subImg->rowPitch, subImg->slicePitch };

            subData.push_back(data);
        }
    }

    alloc->Reset();
    cmdl->Reset(alloc, nullptr);

    UpdateSubresources(cmdl, texture.Get(), intermediate.Get(), 0, 0, imgCount, subData.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdl->ResourceBarrier(1, &barrier);

    cmdl->Close();

    ID3D12CommandList* lists[] = { cmdl };
    modD3D12->getCommandQueue()->ExecuteCommandLists(1, lists);

    modD3D12->flush();

    return texture;

}

ComPtr<ID3D12Resource> ModuleResources::createRenderTarget(DXGI_FORMAT format, UINT width, UINT height, const float clearColor[4])
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1 );
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = format;
    clear.Color[0] = clearColor[0];
    clear.Color[1] = clearColor[1];
    clear.Color[2] = clearColor[2];
    clear.Color[3] = clearColor[3];

    ComPtr<ID3D12Resource> texture;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, &clear, IID_PPV_ARGS(&texture))))
        return nullptr;

    return texture;
}

ComPtr<ID3D12Resource> ModuleResources::createDepthStencil(DXGI_FORMAT format, UINT width, UINT height, float clearDepth, UINT8 clearStencil)
{
    auto modD3D12 = app->getModule<ModuleD3D12>();
    ID3D12Device* device = modD3D12->getDevice();

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format,width,height, 1, 1);
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = format;
    clear.DepthStencil.Depth = clearDepth;
    clear.DepthStencil.Stencil = clearStencil;

    ComPtr<ID3D12Resource> texture;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&texture))))
        return nullptr;

    return texture;
}