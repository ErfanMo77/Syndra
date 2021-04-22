#include "lpch.h"
#include "Engine/Renderer/Mesh.h"

namespace Syndra {

	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<texture> textures)
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
		auto vertexBuffer = VertexBuffer::Create((float*)(&vertices[0]), vertices.size()*sizeof(Vertex));
		auto indexBuffer = IndexBuffer::Create(&indices[0], indices.size());

		m_VertexArray->Bind();

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_pos"},
			{ShaderDataType::Float2,"a_uv"},
			{ShaderDataType::Float3,"a_normal"},
			{ShaderDataType::Float3,"a_tangent"},
			{ShaderDataType::Float3,"a_bitangent"}
		};

		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);
		vertexBuffer->Unbind();
	}
}