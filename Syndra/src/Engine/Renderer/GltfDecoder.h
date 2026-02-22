#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Syndra {

	struct DecodedGltfMaterial
	{
		std::string Name;
		MeshMaterialData MaterialData;
	};

	struct DecodedGltfPrimitive
	{
		std::string Name;
		Mesh Mesh;
		int32_t MaterialIndex = -1;
		size_t SourceMeshIndex = 0;
		size_t SourcePrimitiveIndex = 0;
	};

	struct DecodedGltfNode
	{
		std::string Name;
		glm::mat4 LocalTransform = glm::mat4(1.0f);
		std::vector<size_t> PrimitiveIndices;
		std::vector<size_t> Children;
	};

	struct DecodedGltfScene
	{
		std::string SourcePath;
		std::vector<DecodedGltfMaterial> Materials;
		std::vector<DecodedGltfPrimitive> Primitives;
		std::vector<DecodedGltfNode> Nodes;
		std::vector<size_t> RootNodes;
		std::vector<Ref<Texture2D>> Textures;
	};

	bool DecodeGltf(const std::string& path, DecodedGltfScene& outScene);

}

