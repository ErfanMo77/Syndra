#include "lpch.h"
#include "Engine/Renderer/Mesh.h"

namespace Syndra {

	Mesh::Mesh(
		std::vector<Vertex> vertices,
		std::vector<unsigned int> indices,
		std::vector<texture> textures,
		const MeshMaterialData& materialData)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->materialData = materialData;

		if (!this->vertices.empty())
		{
			m_BoundsMin = this->vertices[0].Position;
			m_BoundsMax = this->vertices[0].Position;
			for (const auto& vertex : this->vertices)
			{
				m_BoundsMin = glm::min(m_BoundsMin, vertex.Position);
				m_BoundsMax = glm::max(m_BoundsMax, vertex.Position);
			}
		}

		setupMesh();
	}

	void Mesh::setupMesh()
	{
		// create buffers/arrays
		m_VertexArray = VertexArray::Create();
		m_VertexBuffer = VertexBuffer::Create((float*)(&vertices[0]), vertices.size()*sizeof(Vertex));
		m_IndexBuffer = IndexBuffer::Create(&indices[0], indices.size());

		m_VertexArray->Bind();

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
			{ShaderDataType::Float3,"a_normal"},
			{ShaderDataType::Float3,"a_tangent"},
			{ShaderDataType::Float3,"a_bitangent"}
		};

		m_VertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);
		//vertexBuffer->Unbind();
	}
}
