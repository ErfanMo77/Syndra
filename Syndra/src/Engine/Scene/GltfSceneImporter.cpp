#include "lpch.h"

#include "Engine/Scene/GltfSceneImporter.h"

#include "Engine/Renderer/GltfDecoder.h"
#include "Engine/Renderer/Material.h"
#include "Engine/Renderer/SceneRenderer.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Utils/Math.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {

	bool IsGltfPath(const std::string& path)
	{
		std::string extension = std::filesystem::path(path).extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return extension == ".gltf" || extension == ".glb";
	}

	std::string GetScenePath(const std::string& path)
	{
		std::string scenePath = std::filesystem::path(path).lexically_normal().string();
		const std::string cwd = std::filesystem::current_path().lexically_normal().string();
		if (scenePath.rfind(cwd, 0) == 0)
			scenePath = scenePath.substr(cwd.size());
		return scenePath;
	}

}

namespace Syndra {

	bool GltfSceneImporter::ImportIntoEntity(Scene& scene, Entity root, const std::string& path)
	{
		if (!root)
			return false;
		if (!IsGltfPath(path))
			return false;

		DecodedGltfScene decodedScene;
		if (!DecodeGltf(path, decodedScene))
			return false;

		Ref<Shader> materialShader = SceneRenderer::GetDefaultMaterialShader();
		if (!materialShader)
			materialShader = SceneRenderer::ResolveShader("GeometryPass");
		if (!materialShader)
			materialShader = SceneRenderer::ResolveShader("main");
		if (!materialShader)
		{
			SN_CORE_WARN("glTF import aborted because no compatible material shader is available.");
			return false;
		}

		std::unordered_map<uint32_t, Ref<Texture2D>> texturesByRendererId;
		for (const auto& texture : decodedScene.Textures)
		{
			if (texture)
				texturesByRendererId[texture->GetRendererID()] = texture;
		}

		auto registerMaterialFromData = [&](const MeshMaterialData& materialData, const std::string& name) -> uint64_t
		{
			auto material = Material::Create(materialShader);
			if (!material)
				return 0;

			material->Set("tiling", 1.0f);
			material->Set("push.material.color", materialData.BaseColorFactor);
			material->Set("push.material.MetallicFactor", materialData.MetallicFactor);
			material->Set("push.material.RoughnessFactor", materialData.RoughnessFactor);
			material->Set("push.material.AO", materialData.AOFactor);

			material->Set("HasAlbedoMap", materialData.AlbedoTextureID != 0 ? 1 : 0);
			material->Set("HasMetallicMap", materialData.MetallicTextureID != 0 ? 1 : 0);
			material->Set("HasNormalMap", materialData.NormalTextureID != 0 ? 1 : 0);
			material->Set("HasRoughnessMap", materialData.RoughnessTextureID != 0 ? 1 : 0);
			material->Set("HasAOMap", materialData.AOTextureID != 0 ? 1 : 0);

			auto& materialTextures = material->GetTextures();
			auto assignTexture = [&](uint32_t rendererId, uint32_t binding)
			{
				if (rendererId == 0)
					return;
				const auto it = texturesByRendererId.find(rendererId);
				if (it != texturesByRendererId.end())
					materialTextures[binding] = it->second;
			};

			assignTexture(materialData.AlbedoTextureID, 0);
			assignTexture(materialData.MetallicTextureID, 1);
			assignTexture(materialData.NormalTextureID, 2);
			assignTexture(materialData.RoughnessTextureID, 3);
			assignTexture(materialData.AOTextureID, 4);

			return scene.RegisterMaterial(material, name);
		};

		std::vector<uint64_t> materialIds(decodedScene.Materials.size(), 0);
		for (size_t materialIndex = 0; materialIndex < decodedScene.Materials.size(); ++materialIndex)
		{
			const auto& decodedMaterial = decodedScene.Materials[materialIndex];
			materialIds[materialIndex] = registerMaterialFromData(decodedMaterial.MaterialData, decodedMaterial.Name);
		}

		MeshMaterialData defaultMaterialData{};
		defaultMaterialData.IsPBR = true;
		defaultMaterialData.BaseColorFactor = glm::vec4(1.0f);
		defaultMaterialData.MetallicFactor = 1.0f;
		defaultMaterialData.RoughnessFactor = 1.0f;
		defaultMaterialData.AOFactor = 1.0f;
		const uint64_t defaultMaterialId = registerMaterialFromData(defaultMaterialData, "glTF Default Material");

		if (root.HasComponent<RelationshipComponent>())
		{
			const auto children = root.GetComponent<RelationshipComponent>().Children;
			for (entt::entity child : children)
			{
				if (child == entt::null)
					continue;
				scene.DestroyEntity(Entity{ child });
			}
		}

		if (root.HasComponent<MeshComponent>())
			root.RemoveComponent<MeshComponent>();
		if (root.HasComponent<MaterialComponent>())
			root.RemoveComponent<MaterialComponent>();

		const std::string scenePath = GetScenePath(path);

		auto getPrimitiveMaterialId = [&](const DecodedGltfPrimitive& primitive) -> uint64_t
		{
			if (primitive.MaterialIndex >= 0 && static_cast<size_t>(primitive.MaterialIndex) < materialIds.size())
			{
				const uint64_t materialId = materialIds[primitive.MaterialIndex];
				if (materialId != 0)
					return materialId;
			}

			return defaultMaterialId;
		};

		auto attachPrimitiveToEntity = [&](Entity targetEntity, const DecodedGltfPrimitive& primitive, size_t decodedPrimitiveIndex)
		{
			if (targetEntity.HasComponent<MeshComponent>())
				targetEntity.RemoveComponent<MeshComponent>();

			auto& meshComponent = targetEntity.AddComponent<MeshComponent>();
			meshComponent.path = scenePath;
			meshComponent.MeshIndex = static_cast<int32_t>(decodedPrimitiveIndex);
			meshComponent.MeshName = primitive.Name;
			meshComponent.model = Model{};
			meshComponent.model.meshes.push_back(primitive.Mesh);
			meshComponent.model.syndraTextures = decodedScene.Textures;

			const uint64_t materialId = getPrimitiveMaterialId(primitive);
			if (materialId != 0)
				scene.AssignMaterial(targetEntity, materialId);
		};

		auto createPrimitiveChild = [&](Entity parentEntity, const DecodedGltfPrimitive& primitive, size_t decodedPrimitiveIndex)
		{
			auto primitiveEntityRef = scene.CreateEntity(primitive.Name.empty() ? "Primitive" : primitive.Name);
			Entity primitiveEntity = *primitiveEntityRef;
			scene.SetParent(primitiveEntity, parentEntity);
			attachPrimitiveToEntity(primitiveEntity, primitive, decodedPrimitiveIndex);
		};

		auto createNodeRecursive = [&](auto&& self, size_t nodeIndex, Entity parentEntity) -> void
		{
			if (nodeIndex >= decodedScene.Nodes.size())
				return;

			const auto& node = decodedScene.Nodes[nodeIndex];
			auto nodeEntityRef = scene.CreateEntity(node.Name.empty() ? ("Node_" + std::to_string(nodeIndex)) : node.Name);
			Entity nodeEntity = *nodeEntityRef;
			scene.SetParent(nodeEntity, parentEntity);

			auto& transform = nodeEntity.GetComponent<TransformComponent>();
			glm::vec3 translation(0.0f);
			glm::vec3 rotation(0.0f);
			glm::vec3 scale(1.0f);
			if (Math::DecomposeTransform(node.LocalTransform, translation, rotation, scale))
			{
				transform.Translation = translation;
				transform.Rotation = rotation;
				transform.Scale = scale;
			}

			if (node.PrimitiveIndices.size() == 1)
			{
				const size_t primitiveIndex = node.PrimitiveIndices[0];
				if (primitiveIndex < decodedScene.Primitives.size())
					attachPrimitiveToEntity(nodeEntity, decodedScene.Primitives[primitiveIndex], primitiveIndex);
			}
			else if (!node.PrimitiveIndices.empty())
			{
				for (size_t primitiveIndex : node.PrimitiveIndices)
				{
					if (primitiveIndex >= decodedScene.Primitives.size())
						continue;
					createPrimitiveChild(nodeEntity, decodedScene.Primitives[primitiveIndex], primitiveIndex);
				}
			}

			for (size_t childIndex : node.Children)
				self(self, childIndex, nodeEntity);
		};

		if (decodedScene.Nodes.empty())
		{
			for (size_t primitiveIndex = 0; primitiveIndex < decodedScene.Primitives.size(); ++primitiveIndex)
				createPrimitiveChild(root, decodedScene.Primitives[primitiveIndex], primitiveIndex);
		}
		else if (decodedScene.RootNodes.empty())
		{
			for (size_t nodeIndex = 0; nodeIndex < decodedScene.Nodes.size(); ++nodeIndex)
				createNodeRecursive(createNodeRecursive, nodeIndex, root);
		}
		else
		{
			for (size_t rootNodeIndex : decodedScene.RootNodes)
				createNodeRecursive(createNodeRecursive, rootNodeIndex, root);
		}

		scene.PruneUnusedMaterials();
		return true;
	}

}
