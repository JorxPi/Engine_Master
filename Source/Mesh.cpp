#include "Globals.h"
#include "Mesh.h"
#include "Application.h"
#include "ModuleResources.h"
#include "gltf_utils.h"
#include "tiny_gltf.h"

void Mesh::load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
	name = mesh.name;

	const auto& itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end())
        return;

	numVertices = uint32_t(model.accessors[itPos->second].count);
    std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(numVertices);
    uint8_t* vertexData = reinterpret_cast<uint8_t*>(vertices.get());

	loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
	loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
    loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");


    auto resources = app->getModule<ModuleResources>();
    vertexBuffer = resources->createDefaultBuffer(vertices.get(), numVertices * sizeof(Vertex));

    vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(Vertex);
    vbv.SizeInBytes = numVertices * sizeof(Vertex);

    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];

        if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT || indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
        {
            indexElementSize = (uint32_t)tinygltf::GetComponentSizeInBytes(indAcc.componentType);
            numIndices = (uint32_t)indAcc.count;

            std::unique_ptr<uint8_t[]> indices = std::make_unique<uint8_t[]>(numIndices * indexElementSize);

            loadAccessorData(indices.get(), indexElementSize, indexElementSize, numIndices, model, primitive.indices);

            indexBuffer = resources->createDefaultBuffer(indices.get(), numIndices * indexElementSize);

            static const DXGI_FORMAT formats[3] = {
                DXGI_FORMAT_R8_UINT,
                DXGI_FORMAT_R16_UINT,
                DXGI_FORMAT_R32_UINT
            };

            ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
            ibv.Format = formats[indexElementSize >> 1];
            ibv.SizeInBytes = numIndices * indexElementSize;
        }
    }

    materialIndex = primitive.material;
}

void Mesh::createFromRaw(const Vertex* vertices, uint32_t vertexCount,const uint16_t* indices, uint32_t indexCount,int32_t materialIdx)
{
    name = "RawMesh";
    numVertices = vertexCount;
    numIndices = indexCount;
    materialIndex = materialIdx;

    auto resources = app->getModule<ModuleResources>();

    vertexBuffer = resources->createDefaultBuffer(vertices, numVertices * sizeof(Vertex));
    vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(Vertex);
    vbv.SizeInBytes = numVertices * sizeof(Vertex);

    if (numIndices > 0)
    {
        indexElementSize = 2;
        indexBuffer = resources->createDefaultBuffer(indices, numIndices * sizeof(uint16_t));

        ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibv.Format = DXGI_FORMAT_R16_UINT;
        ibv.SizeInBytes = numIndices * sizeof(uint16_t);
    }
    else
    {
        indexElementSize = 0;
        indexBuffer.Reset();
        ibv = {};
    }
}

void Mesh::createPlane(float halfSize, int32_t materialIdx)
{
    Vertex v[4]{};
    v[0].position = Vector3(-halfSize, 0.0f, -halfSize);
    v[1].position = Vector3(halfSize, 0.0f, -halfSize);
    v[2].position = Vector3(halfSize, 0.0f, halfSize);
    v[3].position = Vector3(-halfSize, 0.0f, halfSize);

    v[0].texCoord0 = Vector2(0.0f, 0.0f);
    v[1].texCoord0 = Vector2(1.0f, 0.0f);
    v[2].texCoord0 = Vector2(1.0f, 1.0f);
    v[3].texCoord0 = Vector2(0.0f, 1.0f);

    v[0].normal = v[1].normal = v[2].normal = v[3].normal = Vector3::Up;

    const uint16_t idx[6] = { 0, 2, 1, 0, 3, 2 };

    name = "Plane";
    createFromRaw(v, 4, idx, 6, materialIdx);
}