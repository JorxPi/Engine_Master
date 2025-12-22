#include "Globals.h"
#include "Model.h"
#include "Application.h"

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_IMAGE
#define TINYGLTF_NO_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION

#include "tiny_gltf.h"

Model::Model()
{
}

static std::string getBasePath(const char* filePath)
{
    std::string p = filePath ? filePath : "";
    size_t slash = p.find_last_of("/\\");
    if (slash == std::string::npos) return "";
    return p.substr(0, slash);
}

void Model::loadModel(const char* assetFileName)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;

    std::string error;
    std::string warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, assetFileName);

    if (!warning.empty())
        LOG("tinygltf warning: %s", warning.c_str());

    if (!loadOk)
    {
        LOG("Error loading %s: %s", assetFileName, error.c_str());
        return;
    }

    std::string basePath = getBasePath(assetFileName);

    loadMaterials(model, basePath.c_str());
    loadMeshes(model);

 
}

void Model::loadMaterials(const tinygltf::Model& model, const char* basePath)
{
    materials.clear();
    // Preallocate enough space for all materials
    materials.reserve(model.materials.size());

    for (const tinygltf::Material& m : model.materials)
    {
        Material mat;
        mat.load(model, m.pbrMetallicRoughness, basePath);
        materials.push_back(std::move(mat));
    }

    LOG("Loaded materials: %zu", materials.size());
}

void Model::loadMeshes(const tinygltf::Model& model)
{
    meshes.clear();

    size_t totalPrimitives = 0;
    for (const tinygltf::Mesh& mesh : model.meshes)
        totalPrimitives += mesh.primitives.size();

    meshes.reserve(totalPrimitives);

    for (const tinygltf::Mesh& mesh : model.meshes)
    {
        for (const tinygltf::Primitive& prim : mesh.primitives)
        {
            meshes.emplace_back();
            meshes.back().load(model, mesh, prim);
        }
    }

    LOG("Loaded meshes (primitives): %zu", meshes.size());
}