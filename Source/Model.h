#pragma once

#include "Material.h"
#include "Mesh.h"

namespace tinygltf { class Model; }

class Model {
public:
	Model();

	void loadModel(const char* assetFileName);

	void setModelMatrix(const Matrix& m) { modelMatrix = m; }
	const Matrix& getModelMatrix() const { return modelMatrix; }

	const std::vector<Material>& getMaterials() const { return materials; }
	const std::vector<Mesh>& getMeshes() const { return meshes; }

	size_t getMaterialCount() const { return materials.size(); }
	size_t getMeshCount() const { return meshes.size(); }

private:
	void loadMaterials(const tinygltf::Model& model, const char* basePath);
	void loadMeshes(const tinygltf::Model& model);

	std::vector<Material> materials;
	std::vector<Mesh> meshes;

	Matrix modelMatrix = Matrix::Identity;

};
