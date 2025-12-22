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