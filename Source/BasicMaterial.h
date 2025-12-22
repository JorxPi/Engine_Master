#pragma once

#include <string>
#include <wrl/client.h>
#include <SimpleMath.h>

#include "tiny_gltf.h"

using Microsoft::WRL::ComPtr;

struct ID3D12Resource;

class BasicMaterial
{
public:
    BasicMaterial() = default;

    void load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);

    const DirectX::SimpleMath::Vector4& getColour() const { return colour; }
    ID3D12Resource* getColourTex() const { return colourTex.Get(); }
    bool hasColourTex() const { return colourTex != nullptr; }

private:
    DirectX::SimpleMath::Vector4 colour = { 1,1,1,1 };
    ComPtr<ID3D12Resource> colourTex;
};

