#pragma once

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

inline bool loadAccessorData(uint8_t* data, size_t size, size_t stride, size_t elemCount, const tinygltf::Model& model, int accessorIndex) {
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const size_t compSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    const size_t numComps = tinygltf::GetNumComponentsInType(accessor.type);
    const size_t expectedElemSize = compSize * numComps;

    if (expectedElemSize != size)
        return false;

    if (elemCount != accessor.count)
        return false;

    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const uint8_t* bufferData = reinterpret_cast<const uint8_t*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
    size_t bufferStride = (view.byteStride == 0) ? expectedElemSize : view.byteStride;

    for (size_t i = 0; i < elemCount; ++i)
    {
        std::memcpy(data, bufferData, size);
        data += stride;
        bufferData += bufferStride;
    }

    return true;
}

inline bool loadAccessorData(uint8_t* data, size_t size, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int>& attributes, const char* accessorName) {
    const auto& it = attributes.find(accessorName);
    if (it != attributes.end())
    {
        return loadAccessorData(data, size, stride, elemCount, model, it->second);
    }

    return false;
}