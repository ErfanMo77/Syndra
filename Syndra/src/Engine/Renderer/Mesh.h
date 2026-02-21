#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/VertexArray.h"

namespace Syndra {

	struct Vertex {
		glm::vec3 Position;
		glm::vec2 TexCoords;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
	};

	struct texture {
		unsigned int id;
		std::string type;
		std::string path;
	};

	struct MeshMaterialData
	{
		bool IsPBR = false;
		glm::vec4 BaseColorFactor = glm::vec4(1.0f);
		float MetallicFactor = 1.0f;
		float RoughnessFactor = 1.0f;
		float AOFactor = 1.0f;
		uint32_t AlbedoTextureID = 0;
		uint32_t MetallicTextureID = 0;
		uint32_t NormalTextureID = 0;
		uint32_t RoughnessTextureID = 0;
		uint32_t AOTextureID = 0;
	};

	class Mesh
	{
	public:

		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<texture> textures;
		MeshMaterialData materialData;
		
		Mesh(
			std::vector<Vertex> vertices,
			std::vector<unsigned int> indices,
			std::vector<texture> textures,
			const MeshMaterialData& materialData = MeshMaterialData{});
		~Mesh() = default;

		Ref<VertexArray> GetVertexArray() const  { return m_VertexArray; }
		const MeshMaterialData& GetMaterialData() const { return materialData; }
		const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
		const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }
		bool HasBounds() const { return !vertices.empty(); }
		void BindVertexArray() const { m_VertexArray->Bind(); }

	private:
		Ref<VertexArray> m_VertexArray;
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		glm::vec3 m_BoundsMin = glm::vec3(0.0f);
		glm::vec3 m_BoundsMax = glm::vec3(0.0f);
		void setupMesh();
	};

}
