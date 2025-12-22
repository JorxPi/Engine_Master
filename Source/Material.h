#pragma once

#include <string>
#include <wrl/client.h>
#include <SimpleMath.h>

namespace tinygltf {
    class Model;
    struct PbrMetallicRoughness;
}

using Microsoft::WRL::ComPtr;

struct ID3D12Resource;

struct MaterialData
{
    DirectX::SimpleMath::Vector4 baseColour;
    BOOL hasColourTexture;
};

class Material
{
public:
    Material();

    void load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);

    const DirectX::SimpleMath::Vector4& getColour() const { return colour; }
    ID3D12Resource* getColourTex() const { return colourTex.Get(); }
    bool hasColourTex() const { return colourTex != nullptr; }

    D3D12_GPU_VIRTUAL_ADDRESS getMaterialBufferAddress() const { return materialBuffer ? materialBuffer->GetGPUVirtualAddress() : 0; }
    uint32_t getColourSrvIndex() const { return colourSrvIndex; }

private:
    DirectX::SimpleMath::Vector4 colour = { 1,1,1,1 };
    ComPtr<ID3D12Resource> colourTex;

    MaterialData materialData{};
    ComPtr<ID3D12Resource> materialBuffer;
    uint32_t colourSrvIndex = 0;
};

