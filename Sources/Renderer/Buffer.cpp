#include "Renderer/Buffer.h"

#include <glad/gl.h>

namespace viewer
{

	VertexBuffer::VertexBuffer(float* vertices, uint32_t size)
		: m_Renderer(0)
	{
		glCreateBuffers(1, &m_Renderer);
		glBindBuffer(GL_ARRAY_BUFFER, m_Renderer);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	VertexBuffer::VertexBuffer(uint32_t size)
		: m_Renderer(0)
	{
		glCreateBuffers(1, &m_Renderer);
		glBindBuffer(GL_ARRAY_BUFFER, m_Renderer);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices)
		: m_Renderer(0)
	{
		glCreateBuffers(1, &m_Renderer);
		glBindBuffer(GL_ARRAY_BUFFER, m_Renderer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
	}

	VertexBuffer::~VertexBuffer()
	{
		if (m_Renderer)
			glDeleteBuffers(1, &m_Renderer);
	}

	void VertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Renderer);
	}

	void VertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void VertexBuffer::SetData(const void* data, uint32_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Renderer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	IndexBuffer::IndexBuffer(uint32* indices, uint32 count)
		: m_Count(count)
	{
		glCreateBuffers(1, &m_Renderer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Renderer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &m_Renderer);
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Renderer);
	}

	void IndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	uint32_t IndexBuffer::GetCount() const
	{
		return m_Count;
	}

}