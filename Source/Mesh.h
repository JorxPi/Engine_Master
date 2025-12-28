#pragma once
#include <SimpleMath.h>

using Microsoft::WRL::ComPtr;

namespace tinygltf { class Model; struct Mesh; struct Primitive; }

class Mesh
{
public:
    struct Vertex
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector2 texCoord0;
        DirectX::SimpleMath::Vector3 normal;
    };

    void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    const D3D12_VERTEX_BUFFER_VIEW& getVBV() const { return vbv; }
    const D3D12_INDEX_BUFFER_VIEW& getIBV() const { return ibv; }
    bool hasIndices() const { return numIndices > 0; }
    uint32_t getNumIndices() const { return numIndices; }
    uint32_t getNumVertices() const { return numVertices; }
    int32_t  getMaterialIndex() const { return materialIndex; }

private:
    std::string name;

    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t indexElementSize = 0;
    int32_t  materialIndex = -1;

    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbv{};

    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibv{};
};