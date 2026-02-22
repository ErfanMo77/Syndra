#include "lpch.h"

#include "Engine/Renderer/GltfDecoder.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "stb_image.h"

#include <filesystem>
#include <limits>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace {

	enum class TextureChannelSelection : uint8_t
	{
		RGBA = 0,
		Red,
		Green,
		Blue,
		Alpha
	};

	struct TextureCacheKey
	{
		std::size_t ImageIndex = 0;
		TextureChannelSelection Channel = TextureChannelSelection::RGBA;
		bool SRGB = false;

		bool operator==(const TextureCacheKey& other) const
		{
			return ImageIndex == other.ImageIndex &&
				Channel == other.Channel &&
				SRGB == other.SRGB;
		}
	};

	struct TextureCacheKeyHasher
	{
		std::size_t operator()(const TextureCacheKey& key) const
		{
			std::size_t hash = std::hash<std::size_t>{}(key.ImageIndex);
			hash ^= (static_cast<std::size_t>(key.Channel) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
			hash ^= (static_cast<std::size_t>(key.SRGB) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
			return hash;
		}
	};

	struct DecodedImage
	{
		int Width = 0;
		int Height = 0;
		std::vector<unsigned char> Pixels;
	};

	glm::mat4 ToGlmMat4(const fastgltf::math::fmat4x4& matrix)
	{
		glm::mat4 result(1.0f);
		for (int column = 0; column < 4; ++column)
		{
			for (int row = 0; row < 4; ++row)
			{
				result[column][row] = matrix[column][row];
			}
		}
		return result;
	}

	Syndra::MeshMaterialData CreateDefaultMaterialData()
	{
		Syndra::MeshMaterialData material{};
		material.IsPBR = true;
		material.BaseColorFactor = glm::vec4(1.0f);
		material.MetallicFactor = 1.0f;
		material.RoughnessFactor = 1.0f;
		material.AOFactor = 1.0f;
		return material;
	}

	uint64_t MakePrimitiveKey(size_t meshIndex, size_t primitiveIndex)
	{
		return (static_cast<uint64_t>(meshIndex) << 32) | static_cast<uint64_t>(primitiveIndex);
	}

}

namespace Syndra {

	bool DecodeGltf(const std::string& path, DecodedGltfScene& outScene)
	{
		outScene = {};
		outScene.SourcePath = path;

		const std::filesystem::path modelPath = std::filesystem::path(path).lexically_normal();
		const std::filesystem::path modelDirectory = modelPath.has_parent_path() ? modelPath.parent_path() : std::filesystem::current_path();

		fastgltf::Parser parser;
		auto mappedFile = fastgltf::MappedGltfFile::FromPath(modelPath);
		if (!mappedFile)
		{
			SN_CORE_ERROR("Failed to open glTF file '{}': {}", modelPath.string(), fastgltf::getErrorMessage(mappedFile.error()));
			return false;
		}

		constexpr auto loadOptions =
			fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::GenerateMeshIndices;

		auto loadedAsset = parser.loadGltf(mappedFile.get(), modelDirectory, loadOptions, fastgltf::Category::OnlyRenderable);
		if (loadedAsset.error() != fastgltf::Error::None)
		{
			SN_CORE_ERROR("Failed to parse glTF file '{}': {}", modelPath.string(), fastgltf::getErrorMessage(loadedAsset.error()));
			return false;
		}

		fastgltf::Asset asset = std::move(loadedAsset.get());
		std::unordered_map<TextureCacheKey, Ref<Texture2D>, TextureCacheKeyHasher> textureCache;
		std::unordered_map<std::size_t, DecodedImage> decodedImageCache;
		std::unordered_set<std::size_t> failedDecodedImages;

		auto decodeImageBytes = [&](std::size_t imageIndex) -> const DecodedImage* {
			if (imageIndex >= asset.images.size())
				return nullptr;

			if (const auto cacheIt = decodedImageCache.find(imageIndex); cacheIt != decodedImageCache.end())
				return &cacheIt->second;

			if (failedDecodedImages.find(imageIndex) != failedDecodedImages.end())
				return nullptr;

			const auto loadPixelsFromMemory = [](const stbi_uc* bytes, std::size_t length, DecodedImage& output) -> bool {
				if (bytes == nullptr || length == 0 || length > static_cast<std::size_t>(std::numeric_limits<int>::max()))
					return false;

				int width = 0;
				int height = 0;
				int channels = 0;
				stbi_set_flip_vertically_on_load(1);
				stbi_uc* pixels = stbi_load_from_memory(bytes, static_cast<int>(length), &width, &height, &channels, STBI_rgb_alpha);
				if (pixels == nullptr || width <= 0 || height <= 0)
					return false;

				output.Width = width;
				output.Height = height;
				output.Pixels.assign(pixels, pixels + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
				stbi_image_free(pixels);
				return true;
			};

			DecodedImage decodedImage{};
			const auto& image = asset.images[imageIndex];
			const bool success = std::visit(fastgltf::visitor{
				[&](const fastgltf::sources::URI& filePath) -> bool {
					if (!filePath.uri.isLocalPath())
					{
						SN_CORE_WARN("Unsupported non-local glTF image URI for '{}'.", modelPath.string());
						return false;
					}

					std::filesystem::path resolvedPath = filePath.uri.fspath();
					if (resolvedPath.is_relative())
						resolvedPath = modelDirectory / resolvedPath;
					resolvedPath = resolvedPath.lexically_normal();

					int width = 0;
					int height = 0;
					int channels = 0;
					stbi_set_flip_vertically_on_load(1);
					stbi_uc* pixels = stbi_load(resolvedPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
					if (pixels == nullptr || width <= 0 || height <= 0)
						return false;

					decodedImage.Width = width;
					decodedImage.Height = height;
					decodedImage.Pixels.assign(pixels, pixels + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
					stbi_image_free(pixels);
					return true;
				},
				[&](const fastgltf::sources::Array& array) -> bool {
					return loadPixelsFromMemory(reinterpret_cast<const stbi_uc*>(array.bytes.data()), array.bytes.size(), decodedImage);
				},
				[&](const fastgltf::sources::Vector& vector) -> bool {
					return loadPixelsFromMemory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), vector.bytes.size(), decodedImage);
				},
				[&](const fastgltf::sources::ByteView& byteView) -> bool {
					return loadPixelsFromMemory(reinterpret_cast<const stbi_uc*>(byteView.bytes.data()), byteView.bytes.size(), decodedImage);
				},
				[&](const fastgltf::sources::BufferView& bufferViewSource) -> bool {
					fastgltf::DefaultBufferDataAdapter adapter;
					const auto bufferBytes = adapter(asset, bufferViewSource.bufferViewIndex);
					return loadPixelsFromMemory(reinterpret_cast<const stbi_uc*>(bufferBytes.data()), bufferBytes.size(), decodedImage);
				},
				[&](const fastgltf::sources::Fallback&) -> bool {
					SN_CORE_WARN("glTF image fallback source is unsupported for '{}'.", modelPath.string());
					return false;
				},
				[&](const auto&) -> bool {
					SN_CORE_WARN("Unsupported glTF image source variant for '{}'.", modelPath.string());
					return false;
				}
			}, image.data);

			if (!success || decodedImage.Pixels.empty())
			{
				failedDecodedImages.insert(imageIndex);
				return nullptr;
			}

			const auto [it, _] = decodedImageCache.emplace(imageIndex, std::move(decodedImage));
			return &it->second;
		};

		auto resolveImageIndexFromTextureInfo = [&](const fastgltf::TextureInfo& textureInfo) -> std::optional<std::size_t> {
			if (textureInfo.textureIndex >= asset.textures.size())
				return std::nullopt;

			const auto& texture = asset.textures[textureInfo.textureIndex];
			if (texture.imageIndex.has_value())
				return texture.imageIndex.value();
			if (texture.webpImageIndex.has_value())
				return texture.webpImageIndex.value();
			if (texture.basisuImageIndex.has_value())
				return texture.basisuImageIndex.value();
			if (texture.ddsImageIndex.has_value())
				return texture.ddsImageIndex.value();
			return std::nullopt;
		};

		auto loadTextureFromImageIndex = [&](std::size_t imageIndex, bool sRGB, TextureChannelSelection channel) -> Ref<Texture2D> {
			TextureCacheKey cacheKey{ imageIndex, channel, sRGB };
			if (const auto cacheIt = textureCache.find(cacheKey); cacheIt != textureCache.end())
				return cacheIt->second;

			const DecodedImage* decodedImage = decodeImageBytes(imageIndex);
			if (decodedImage == nullptr)
				return nullptr;

			Ref<Texture2D> texture;
			if (channel == TextureChannelSelection::RGBA)
			{
				texture = Texture2D::Create(
					static_cast<uint32_t>(decodedImage->Width),
					static_cast<uint32_t>(decodedImage->Height),
					decodedImage->Pixels.data(),
					sRGB);
			}
			else
			{
				const std::size_t pixelCount = static_cast<std::size_t>(decodedImage->Width) * static_cast<std::size_t>(decodedImage->Height);
				std::vector<unsigned char> channelPixels(pixelCount * 4, 255);
				const uint32_t channelIndex =
					channel == TextureChannelSelection::Red ? 0 :
					channel == TextureChannelSelection::Green ? 1 :
					channel == TextureChannelSelection::Blue ? 2 : 3;

				for (std::size_t i = 0; i < pixelCount; ++i)
				{
					const unsigned char value = decodedImage->Pixels[i * 4 + channelIndex];
					channelPixels[i * 4 + 0] = value;
					channelPixels[i * 4 + 1] = value;
					channelPixels[i * 4 + 2] = value;
					channelPixels[i * 4 + 3] = 255;
				}

				texture = Texture2D::Create(
					static_cast<uint32_t>(decodedImage->Width),
					static_cast<uint32_t>(decodedImage->Height),
					channelPixels.data(),
					false);
			}

			if (texture)
			{
				outScene.Textures.push_back(texture);
				textureCache.emplace(cacheKey, texture);
			}

			return texture;
		};

		auto loadTextureFromInfo = [&](const fastgltf::TextureInfo& textureInfo, bool sRGB, TextureChannelSelection channel) -> Ref<Texture2D> {
			const auto imageIndex = resolveImageIndexFromTextureInfo(textureInfo);
			if (!imageIndex.has_value())
				return nullptr;

			return loadTextureFromImageIndex(imageIndex.value(), sRGB, channel);
		};

		outScene.Materials.reserve(asset.materials.size());
		for (size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex)
		{
			const auto& material = asset.materials[materialIndex];
			DecodedGltfMaterial decodedMaterial{};
			decodedMaterial.Name = material.name.empty() ? ("Material_" + std::to_string(materialIndex)) : std::string(material.name);
			decodedMaterial.MaterialData = CreateDefaultMaterialData();

			const auto baseColor = material.pbrData.baseColorFactor;
			decodedMaterial.MaterialData.BaseColorFactor = glm::vec4(
				static_cast<float>(baseColor[0]),
				static_cast<float>(baseColor[1]),
				static_cast<float>(baseColor[2]),
				static_cast<float>(baseColor[3]));
			decodedMaterial.MaterialData.MetallicFactor = static_cast<float>(material.pbrData.metallicFactor);
			decodedMaterial.MaterialData.RoughnessFactor = static_cast<float>(material.pbrData.roughnessFactor);

			if (material.pbrData.baseColorTexture.has_value())
			{
				if (auto texture = loadTextureFromInfo(material.pbrData.baseColorTexture.value(), true, TextureChannelSelection::RGBA))
					decodedMaterial.MaterialData.AlbedoTextureID = texture->GetRendererID();
			}

			if (material.normalTexture.has_value())
			{
				if (auto texture = loadTextureFromInfo(material.normalTexture.value(), false, TextureChannelSelection::RGBA))
					decodedMaterial.MaterialData.NormalTextureID = texture->GetRendererID();
			}

			if (material.pbrData.metallicRoughnessTexture.has_value())
			{
				const auto& metallicRoughnessTexture = material.pbrData.metallicRoughnessTexture.value();
				if (auto metallicTexture = loadTextureFromInfo(metallicRoughnessTexture, false, TextureChannelSelection::Blue))
					decodedMaterial.MaterialData.MetallicTextureID = metallicTexture->GetRendererID();
				if (auto roughnessTexture = loadTextureFromInfo(metallicRoughnessTexture, false, TextureChannelSelection::Green))
					decodedMaterial.MaterialData.RoughnessTextureID = roughnessTexture->GetRendererID();
			}

			if (material.occlusionTexture.has_value())
			{
				decodedMaterial.MaterialData.AOFactor = static_cast<float>(material.occlusionTexture->strength);
				if (auto texture = loadTextureFromInfo(material.occlusionTexture.value(), false, TextureChannelSelection::Red))
					decodedMaterial.MaterialData.AOTextureID = texture->GetRendererID();
			}

			outScene.Materials.push_back(decodedMaterial);
		}

		auto processPrimitive = [&](const fastgltf::Primitive& primitive) -> std::optional<Mesh> {
			if (primitive.type != fastgltf::PrimitiveType::Triangles)
			{
				SN_CORE_WARN("Skipping non-triangle glTF primitive in '{}'.", modelPath.string());
				return std::nullopt;
			}

			const auto positionAttribute = primitive.findAttribute("POSITION");
			if (positionAttribute == primitive.attributes.end())
			{
				SN_CORE_WARN("Skipped a glTF primitive without POSITION in '{}'.", modelPath.string());
				return std::nullopt;
			}

			const auto& positionAccessor = asset.accessors[positionAttribute->accessorIndex];
			if (positionAccessor.count == 0)
				return std::nullopt;

			std::vector<Vertex> vertices(positionAccessor.count);
			for (auto& vertex : vertices)
			{
				vertex.Position = glm::vec3(0.0f);
				vertex.Normal = glm::vec3(0.0f);
				vertex.TexCoords = glm::vec2(0.0f);
				vertex.Tangent = glm::vec3(0.0f);
				vertex.Bitangent = glm::vec3(0.0f);
			}

			fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, positionAccessor, [&](const glm::vec3& position, std::size_t index) {
				vertices[index].Position = position;
			});

			const auto normalAttribute = primitive.findAttribute("NORMAL");
			if (normalAttribute != primitive.attributes.end())
			{
				const auto& normalAccessor = asset.accessors[normalAttribute->accessorIndex];
				fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, normalAccessor, [&](const glm::vec3& normal, std::size_t index) {
					vertices[index].Normal = normal;
				});
			}

			const auto uvAttribute = primitive.findAttribute("TEXCOORD_0");
			if (uvAttribute != primitive.attributes.end())
			{
				const auto& uvAccessor = asset.accessors[uvAttribute->accessorIndex];
				fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, uvAccessor, [&](const glm::vec2& uv, std::size_t index) {
					vertices[index].TexCoords = { uv.x, 1.0f - uv.y };
				});
			}

			std::vector<float> tangentSigns(vertices.size(), 1.0f);
			const auto tangentAttribute = primitive.findAttribute("TANGENT");
			if (tangentAttribute != primitive.attributes.end())
			{
				const auto& tangentAccessor = asset.accessors[tangentAttribute->accessorIndex];
				fastgltf::iterateAccessorWithIndex<glm::vec4>(asset, tangentAccessor, [&](const glm::vec4& tangent, std::size_t index) {
					vertices[index].Tangent = glm::vec3(tangent);
					tangentSigns[index] = tangent.w;
				});
			}

			if (tangentAttribute != primitive.attributes.end() && normalAttribute != primitive.attributes.end())
			{
				for (std::size_t i = 0; i < vertices.size(); ++i)
				{
					const glm::vec3 bitangent = glm::cross(vertices[i].Normal, vertices[i].Tangent) * tangentSigns[i];
					const float lenSq = glm::dot(bitangent, bitangent);
					if (lenSq > 1e-8f)
						vertices[i].Bitangent = glm::normalize(bitangent);
				}
			}

			std::vector<unsigned int> indices;
			if (primitive.indicesAccessor.has_value())
			{
				const auto& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
				std::vector<uint32_t> indexData(indexAccessor.count);
				fastgltf::copyFromAccessor<uint32_t>(asset, indexAccessor, indexData.data());
				indices.assign(indexData.begin(), indexData.end());
			}
			else
			{
				indices.resize(vertices.size());
				std::iota(indices.begin(), indices.end(), 0u);
			}

			MeshMaterialData materialData = CreateDefaultMaterialData();
			if (primitive.materialIndex.has_value() && primitive.materialIndex.value() < outScene.Materials.size())
				materialData = outScene.Materials[primitive.materialIndex.value()].MaterialData;

			std::vector<texture> meshTextures;
			if (materialData.AlbedoTextureID != 0)
				meshTextures.push_back({ materialData.AlbedoTextureID, "texture_diffuse", "" });
			if (materialData.MetallicTextureID != 0)
				meshTextures.push_back({ materialData.MetallicTextureID, "texture_metallic", "" });
			if (materialData.NormalTextureID != 0)
				meshTextures.push_back({ materialData.NormalTextureID, "texture_normal", "" });
			if (materialData.RoughnessTextureID != 0)
				meshTextures.push_back({ materialData.RoughnessTextureID, "texture_roughness", "" });
			if (materialData.AOTextureID != 0)
				meshTextures.push_back({ materialData.AOTextureID, "texture_ao", "" });

			return Mesh(vertices, indices, meshTextures, materialData);
		};

		std::unordered_map<uint64_t, size_t> primitiveIndexLookup;
		for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex)
		{
			const auto& mesh = asset.meshes[meshIndex];
			for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
			{
				auto decodedMesh = processPrimitive(mesh.primitives[primitiveIndex]);
				if (!decodedMesh.has_value())
					continue;

				DecodedGltfPrimitive decodedPrimitive{};
				decodedPrimitive.Name = mesh.name.empty()
					? ("Mesh_" + std::to_string(meshIndex) + "_Prim" + std::to_string(primitiveIndex))
					: (std::string(mesh.name) + "_Prim" + std::to_string(primitiveIndex));
				decodedPrimitive.Mesh = std::move(decodedMesh.value());
				decodedPrimitive.MaterialIndex = mesh.primitives[primitiveIndex].materialIndex.has_value()
					? static_cast<int32_t>(mesh.primitives[primitiveIndex].materialIndex.value())
					: -1;
				decodedPrimitive.SourceMeshIndex = meshIndex;
				decodedPrimitive.SourcePrimitiveIndex = primitiveIndex;

				const size_t decodedIndex = outScene.Primitives.size();
				outScene.Primitives.push_back(std::move(decodedPrimitive));
				primitiveIndexLookup[MakePrimitiveKey(meshIndex, primitiveIndex)] = decodedIndex;
			}
		}

		outScene.Nodes.resize(asset.nodes.size());
		for (size_t nodeIndex = 0; nodeIndex < asset.nodes.size(); ++nodeIndex)
		{
			const auto& node = asset.nodes[nodeIndex];
			auto& decodedNode = outScene.Nodes[nodeIndex];
			decodedNode.Name = node.name.empty() ? ("Node_" + std::to_string(nodeIndex)) : std::string(node.name);
			decodedNode.LocalTransform = ToGlmMat4(fastgltf::getTransformMatrix(node));
			decodedNode.Children.assign(node.children.begin(), node.children.end());

			if (node.meshIndex.has_value() && node.meshIndex.value() < asset.meshes.size())
			{
				const auto& mesh = asset.meshes[node.meshIndex.value()];
				for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
				{
					const auto key = MakePrimitiveKey(node.meshIndex.value(), primitiveIndex);
					const auto primitiveIt = primitiveIndexLookup.find(key);
					if (primitiveIt != primitiveIndexLookup.end())
						decodedNode.PrimitiveIndices.push_back(primitiveIt->second);
				}
			}
		}

		if (!asset.scenes.empty())
		{
			size_t sceneIndex = asset.defaultScene.value_or(0);
			if (sceneIndex >= asset.scenes.size())
				sceneIndex = 0;

			for (size_t rootNodeIndex : asset.scenes[sceneIndex].nodeIndices)
			{
				if (rootNodeIndex < outScene.Nodes.size())
					outScene.RootNodes.push_back(rootNodeIndex);
			}
		}
		else
		{
			std::vector<bool> isChild(outScene.Nodes.size(), false);
			for (const auto& node : outScene.Nodes)
			{
				for (size_t childIndex : node.Children)
				{
					if (childIndex < isChild.size())
						isChild[childIndex] = true;
				}
			}

			for (size_t nodeIndex = 0; nodeIndex < isChild.size(); ++nodeIndex)
			{
				if (!isChild[nodeIndex])
					outScene.RootNodes.push_back(nodeIndex);
			}
		}

		if (outScene.Primitives.empty())
		{
			SN_CORE_WARN("No renderable meshes were extracted from glTF '{}'.", modelPath.string());
		}

		return true;
	}

}
