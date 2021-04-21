#include "lpch.h"
#include "Engine/Renderer/Mesh.h"

namespace Syndra {

	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		setupMesh();
	}

	void Mesh::setupMesh()
	{
		// create buffers/arrays
		m_VertexArray = VertexArray::Create();
		auto vertexBuffer = VertexBuffer::Create((float*)(&vertices[0]), sizeof(vertices));
		auto indexBuffer = IndexBuffer::Create(&indices[0], indices.size() / sizeof(uint32_t));

		m_VertexArray->Bind();

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float3,"a_normal"},
			{ShaderDataType::Float3,"a_uv"},
			{ShaderDataType::Float3,"a_tangent"},
			{ShaderDataType::Float3,"a_bitangent"}
		};

		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);
		vertexBuffer->Unbind();
	}
}