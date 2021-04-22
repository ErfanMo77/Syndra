#pragma once
#include "Engine/Renderer/Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Syndra {

	class Model
	{
	public:
		std::vector<texture> textures_loaded;
		std::vector<Mesh>    meshes;
		std::string directory;
		bool gammaCorrection;

		Model(std::string const& path, bool gamma = false);
		void Draw(Shader& shader);

	private:
		void loadModel(std::string const& path);
		void processNode(aiNode* node, const aiScene* scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene);
		std::vector<texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	};

}