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

	class Mesh
	{
	public:

		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<texture> textures;
		
		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<texture> textures);

		Ref<VertexArray> GetVertexArray() { return m_VertexArray; }

	private:
		Ref<VertexArray> m_VertexArray;
		void setupMesh();
	};

}