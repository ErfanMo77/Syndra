#pragma once
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Texture.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Syndra {

	class Model
	{
	public:
		std::vector<texture> textures_loaded;
		std::vector<Ref<Texture2D>> syndraTextures;
		std::vector<Mesh>  meshes;
		std::string directory;
		bool gammaCorrection;
		Model() = default;
		~Model() = default;
		Model(std::string& path, bool gamma = false);

	private:
		void loadModel(std::string const& path);
		void processNode(aiNode* node, const aiScene* scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene);
		std::vector<texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	};

}