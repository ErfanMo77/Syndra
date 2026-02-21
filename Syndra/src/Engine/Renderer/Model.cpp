#include "lpch.h"
#include "Engine/Renderer/Model.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "stb_image.h"

#include <cctype>
#include <limits>
#include <numeric>
#include <optional>
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

	std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
			});
		return value;
	}

	bool IsGltfPath(const std::string& path)
	{
		const auto extension = ToLower(std::filesystem::path(path).extension().string());
		return extension == ".gltf" || extension == ".glb";
	}

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

	glm::vec3 SafeTransformDirection(const glm::mat3& matrix, const glm::vec3& value)
	{
		const glm::vec3 transformed = matrix * value;
		const float lenSq = glm::dot(transformed, transformed);
		if (lenSq <= 1e-8f)
			return glm::vec3(0.0f);

		return glm::normalize(transformed);
	}

	Syndra::MeshMaterialData CreateDefaultGltfMaterial()
	{
		Syndra::MeshMaterialData material{};
		material.IsPBR = true;
		material.BaseColorFactor = glm::vec4(1.0f);
		material.MetallicFactor = 1.0f;
		material.RoughnessFactor = 1.0f;
		material.AOFactor = 1.0f;
		return material;
	}

}

namespace Syndra {

	Model::Model(std::string& path, bool gamma) :gammaCorrection(gamma)
	{
		loadModel(path);
	}

	void Model::loadModel(std::string const& path)
	{
		meshes.clear();
		textures_loaded.clear();
		syndraTextures.clear();
		directory.clear();
		m_Scene = nullptr;

		if (IsGltfPath(path))
		{
			loadGltfModel(path);
			return;
		}

		loadAssimpModel(path);
	}

	void Model::loadGltfModel(std::string const& path)
	{
		const std::filesystem::path modelPath = std::filesystem::path(path).lexically_normal();
		const std::filesystem::path modelDirectory = modelPath.has_parent_path() ? modelPath.parent_path() : std::filesystem::current_path();
		directory = modelDirectory.string();

		fastgltf::Parser parser;
		auto mappedFile = fastgltf::MappedGltfFile::FromPath(modelPath);
		if (!mappedFile)
		{
			SN_CORE_ERROR("Failed to open glTF file '{}': {}", modelPath.string(), fastgltf::getErrorMessage(mappedFile.error()));
			return;
		}

		constexpr auto loadOptions =
			fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::GenerateMeshIndices;

		auto loadedAsset = parser.loadGltf(mappedFile.get(), modelDirectory, loadOptions, fastgltf::Category::OnlyRenderable);
		if (loadedAsset.error() != fastgltf::Error::None)
		{
			SN_CORE_ERROR("Failed to parse glTF file '{}': {}", modelPath.string(), fastgltf::getErrorMessage(loadedAsset.error()));
			return;
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
				syndraTextures.push_back(texture);
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

		auto processPrimitive = [&](const fastgltf::Primitive& primitive, const glm::mat4& nodeTransform) -> std::optional<Mesh> {
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

			MeshMaterialData materialData = CreateDefaultGltfMaterial();
			if (primitive.materialIndex.has_value() && primitive.materialIndex.value() < asset.materials.size())
			{
				const auto& material = asset.materials[primitive.materialIndex.value()];
				const auto baseColor = material.pbrData.baseColorFactor;
				materialData.BaseColorFactor = glm::vec4(
					static_cast<float>(baseColor[0]),
					static_cast<float>(baseColor[1]),
					static_cast<float>(baseColor[2]),
					static_cast<float>(baseColor[3]));
				materialData.MetallicFactor = static_cast<float>(material.pbrData.metallicFactor);
				materialData.RoughnessFactor = static_cast<float>(material.pbrData.roughnessFactor);

				if (material.pbrData.baseColorTexture.has_value())
				{
					if (auto texture = loadTextureFromInfo(material.pbrData.baseColorTexture.value(), true, TextureChannelSelection::RGBA))
						materialData.AlbedoTextureID = texture->GetRendererID();
				}

				if (material.normalTexture.has_value())
				{
					if (auto texture = loadTextureFromInfo(material.normalTexture.value(), false, TextureChannelSelection::RGBA))
						materialData.NormalTextureID = texture->GetRendererID();
				}

				if (material.pbrData.metallicRoughnessTexture.has_value())
				{
					const auto& metallicRoughnessTexture = material.pbrData.metallicRoughnessTexture.value();
					if (auto metallicTexture = loadTextureFromInfo(metallicRoughnessTexture, false, TextureChannelSelection::Blue))
						materialData.MetallicTextureID = metallicTexture->GetRendererID();
					if (auto roughnessTexture = loadTextureFromInfo(metallicRoughnessTexture, false, TextureChannelSelection::Green))
						materialData.RoughnessTextureID = roughnessTexture->GetRendererID();
				}

				if (material.occlusionTexture.has_value())
				{
					materialData.AOFactor = static_cast<float>(material.occlusionTexture->strength);
					if (auto texture = loadTextureFromInfo(material.occlusionTexture.value(), false, TextureChannelSelection::Red))
						materialData.AOTextureID = texture->GetRendererID();
				}
			}

			const glm::mat3 upperLeftTransform = glm::mat3(nodeTransform);
			glm::mat3 normalMatrix(1.0f);
			const float determinant = glm::determinant(upperLeftTransform);
			if (std::abs(determinant) > 1e-8f)
			{
				normalMatrix = glm::transpose(glm::inverse(upperLeftTransform));
			}

			for (auto& vertex : vertices)
			{
				vertex.Position = glm::vec3(nodeTransform * glm::vec4(vertex.Position, 1.0f));
				vertex.Normal = SafeTransformDirection(normalMatrix, vertex.Normal);
				vertex.Tangent = SafeTransformDirection(normalMatrix, vertex.Tangent);
				vertex.Bitangent = SafeTransformDirection(normalMatrix, vertex.Bitangent);
			}

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

		auto processMeshWithTransform = [&](std::size_t meshIndex, const glm::mat4& transform) {
			if (meshIndex >= asset.meshes.size())
				return;

			for (const auto& primitive : asset.meshes[meshIndex].primitives)
			{
				auto mesh = processPrimitive(primitive, transform);
				if (mesh.has_value())
					meshes.push_back(std::move(mesh.value()));
			}
			};

		if (!asset.scenes.empty())
		{
			std::size_t sceneIndex = asset.defaultScene.value_or(0);
			if (sceneIndex >= asset.scenes.size())
				sceneIndex = 0;

			fastgltf::iterateSceneNodes(asset, sceneIndex, fastgltf::math::fmat4x4(), [&](fastgltf::Node& node, const fastgltf::math::fmat4x4& worldTransform) {
				if (!node.meshIndex.has_value())
					return;

				processMeshWithTransform(node.meshIndex.value(), ToGlmMat4(worldTransform));
				});
		}
		else
		{
			const glm::mat4 identity(1.0f);
			for (std::size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex)
				processMeshWithTransform(meshIndex, identity);
		}

		if (meshes.empty())
		{
			SN_CORE_WARN("No renderable meshes were extracted from glTF '{}'.", modelPath.string());
		}
	}

	void Model::loadAssimpModel(std::string const& path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// check for errors
		m_Scene = scene;
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			SN_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
			return;
		}

		const auto pathDirectory = std::filesystem::path(path).parent_path();
		directory = pathDirectory.empty() ? std::filesystem::current_path().string() : pathDirectory.string();
		// process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	void Model::processNode(aiNode* node, const aiScene* scene)
	{
		// process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));

		}
		// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Syndra::Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// data to fill
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<texture> textures;


		// walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = 1 - mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
				// tangent
				if (mesh->mTangents)
				{
					vector.x = mesh->mTangents[i].x;
					vector.y = mesh->mTangents[i].y;
					vector.z = mesh->mTangents[i].z;
					vertex.Tangent = vector;
				}
				// bitangent
				if (mesh->mBitangents)
				{
					vector.x = mesh->mBitangents[i].x;
					vector.y = mesh->mBitangents[i].y;
					vector.z = mesh->mBitangents[i].z;
					vertex.Bitangent = vector;
				}
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// 1. diffuse maps
		std::vector<texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. specular maps
		std::vector<texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		// 3. normal maps
		std::vector<texture> normalMaps = loadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_normal");
		std::vector<texture> normalMaps2 = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		textures.insert(textures.end(), normalMaps2.begin(), normalMaps2.end());
		// 4. height maps
		std::vector<texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		// return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures);
	}

	std::vector<texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
	{
		std::vector<texture> textures;
		const bool isColorTexture = typeName == "texture_diffuse";
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			SN_CORE_TRACE(str.C_Str());
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				texture texture;
				std::string filename = str.C_Str();
				filename = (std::filesystem::path(directory) / filename).lexically_normal().string();
				Ref<Texture2D> syndraTexture;
				if (auto tex = m_Scene->GetEmbeddedTexture(str.C_Str())) {
					if (tex->mHeight == 0)
					{
						int width = 0;
						int height = 0;
						int channels = 0;
						stbi_set_flip_vertically_on_load(1);
						stbi_uc* data = stbi_load_from_memory(
							reinterpret_cast<const stbi_uc*>(tex->pcData),
							static_cast<int>(tex->mWidth),
							&width,
							&height,
							&channels,
							STBI_rgb_alpha);
						if (data)
						{
							syndraTexture = Texture2D::Create(static_cast<uint32_t>(width), static_cast<uint32_t>(height), data, isColorTexture);
							stbi_image_free(data);
						}
					}
					else
					{
						syndraTexture = Texture2D::Create(tex->mWidth, tex->mHeight, reinterpret_cast<unsigned char*>(tex->pcData), isColorTexture);
					}
				}
				else
				{
					syndraTexture = Texture2D::Create(filename, isColorTexture);
				}
				if (syndraTexture) {
					syndraTextures.push_back(syndraTexture);
					texture.id = syndraTexture->GetRendererID();
					texture.type = typeName;
					texture.path = str.C_Str();
					textures.push_back(texture);
					textures_loaded.push_back(texture); // add to loaded textures
				}
			}
		}
		return textures;
	}

}
