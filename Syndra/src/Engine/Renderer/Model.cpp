#include "lpch.h"
#include "Engine/Renderer/Model.h"
#include "Engine/Renderer/GltfDecoder.h"

#include "stb_image.h"

#include <cctype>
#include <filesystem>

namespace {

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

	glm::vec3 SafeTransformDirection(const glm::mat3& matrix, const glm::vec3& value)
	{
		const glm::vec3 transformed = matrix * value;
		const float lenSq = glm::dot(transformed, transformed);
		if (lenSq <= 1e-8f)
			return glm::vec3(0.0f);

		return glm::normalize(transformed);
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

		DecodedGltfScene decodedScene;
		if (!DecodeGltf(path, decodedScene))
			return;

		syndraTextures = decodedScene.Textures;

		auto appendPrimitiveWithTransform = [&](const DecodedGltfPrimitive& primitive, const glm::mat4& worldTransform)
		{
			std::vector<Vertex> transformedVertices = primitive.Mesh.vertices;
			const glm::mat3 upperLeftTransform = glm::mat3(worldTransform);
			glm::mat3 normalMatrix(1.0f);
			const float determinant = glm::determinant(upperLeftTransform);
			if (std::abs(determinant) > 1e-8f)
				normalMatrix = glm::transpose(glm::inverse(upperLeftTransform));

			for (auto& vertex : transformedVertices)
			{
				vertex.Position = glm::vec3(worldTransform * glm::vec4(vertex.Position, 1.0f));
				vertex.Normal = SafeTransformDirection(normalMatrix, vertex.Normal);
				vertex.Tangent = SafeTransformDirection(normalMatrix, vertex.Tangent);
				vertex.Bitangent = SafeTransformDirection(normalMatrix, vertex.Bitangent);
			}

			meshes.emplace_back(
				std::move(transformedVertices),
				primitive.Mesh.indices,
				primitive.Mesh.textures,
				primitive.Mesh.materialData);
		};

		if (decodedScene.Nodes.empty())
		{
			for (const auto& primitive : decodedScene.Primitives)
				appendPrimitiveWithTransform(primitive, glm::mat4(1.0f));
		}
		else
		{
			auto traverseNode = [&](auto&& self, size_t nodeIndex, const glm::mat4& parentTransform) -> void
			{
				if (nodeIndex >= decodedScene.Nodes.size())
					return;

				const auto& node = decodedScene.Nodes[nodeIndex];
				const glm::mat4 worldTransform = parentTransform * node.LocalTransform;

				for (size_t primitiveIndex : node.PrimitiveIndices)
				{
					if (primitiveIndex >= decodedScene.Primitives.size())
						continue;

					appendPrimitiveWithTransform(decodedScene.Primitives[primitiveIndex], worldTransform);
				}

				for (size_t childIndex : node.Children)
					self(self, childIndex, worldTransform);
			};

			if (decodedScene.RootNodes.empty())
			{
				for (size_t nodeIndex = 0; nodeIndex < decodedScene.Nodes.size(); ++nodeIndex)
					traverseNode(traverseNode, nodeIndex, glm::mat4(1.0f));
			}
			else
			{
				for (size_t rootIndex : decodedScene.RootNodes)
					traverseNode(traverseNode, rootIndex, glm::mat4(1.0f));
			}
		}

		if (meshes.empty())
			SN_CORE_WARN("No renderable meshes were extracted from glTF '{}'.", modelPath.string());
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
