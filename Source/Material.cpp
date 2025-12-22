#include "Globals.h"
#include "Material.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

Material::Material()
{
}

static std::string joinPath(const char* basePath, const std::string& uri) {
    if (!basePath) return uri;
    std::string full = basePath;
    if (!full.empty() && full.back() != '/' && full.back() != '\\')
        full.push_back('/');
    full += uri;
    return full;
}

void Material::load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath) {
    colour = DirectX::SimpleMath::Vector4(
        float(material.baseColorFactor[0]),
        float(material.baseColorFactor[1]),
        float(material.baseColorFactor[2]),
        float(material.baseColorFactor[3])
    );

    colourTex.Reset();

    if (material.baseColorTexture.index >= 0 && material.baseColorTexture.index < (int)model.textures.size())
    {
        const tinygltf::Texture& tex = model.textures[material.baseColorTexture.index];

        if (tex.source >= 0 && tex.source < (int)model.images.size())
        {
            const tinygltf::Image& img = model.images[tex.source];

            if (!img.uri.empty())
            {
                std::string fullPath = joinPath(basePath, img.uri);
                std::wstring wPath = std::wstring(fullPath.begin(), fullPath.end());

                auto res = app->getModule<ModuleResources>();
                colourTex = res->createTextureFromFile(wPath.c_str());
            }
        }
    }

    materialData.baseColour = colour;
    materialData.hasColourTexture = (colourTex != nullptr) ? TRUE : FALSE;

    materialBuffer.Reset();

    const size_t cbSize = alignUp(sizeof(MaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    std::vector<uint8_t> tmp(cbSize, 0);
    std::memcpy(tmp.data(), &materialData, sizeof(MaterialData));

    auto modRes = app->getModule<ModuleResources>();
    materialBuffer = modRes->createDefaultBuffer(tmp.data(), (UINT64)cbSize);

    auto modDesc = app->getModule<ModuleDescriptors>();
    if (colourTex)
        colourSrvIndex = modDesc->createTexture2DSRV(colourTex.Get());
    else
        colourSrvIndex = modDesc->createNullTexture2DSRV();
}
