#include "Renderer/VertexArray.h"

#include <glad/gl.h>

namespace viewer
{

	VertexArray::VertexArray()
		: m_RendererID(0)
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	VertexArray::~VertexArray()
	{
		if (m_RendererID)
			glDeleteVertexArrays(1, &m_RendererID);
	}

	void VertexArray::Bind() const
	{
		glBindVertexArray(m_RendererID);
	}

	void VertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexbuffer, const std::vector<VertexComponent>& comps)
	{
		glBindVertexArray(m_RendererID);
		vertexbuffer->Bind();
		// vertex layout
		int offset = 0;
		int count = 0;
		int format = 0;
		for (const auto& vc : comps)
		{
			switch (vc) {
			case VertexComponent::Position:
				count = 3;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Position);
				break;
			case VertexComponent::Color:
				count = 4;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Color);
				break;
			case VertexComponent::UV:
				count = 2;
				format = GL_FLOAT;
				offset = offsetof(Vertex, UV);
				break;
			case VertexComponent::UV1:
				count = 2;
				format = GL_FLOAT;
				offset = offsetof(Vertex, UV1);
				break;
			case VertexComponent::Normal:
				count = 3;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Normal);
				break;
			case VertexComponent::Tangent:
				count = 3;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Tangent);
				break;
			case VertexComponent::Bitangent:
				count = 3;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Bitangent);
				break;
			case VertexComponent::Joint0:
				count = 4;
				format = GL_INT;
				offset = offsetof(Vertex, Joint0);
				break;
			case VertexComponent::Weight0:
				count = 4;
				format = GL_FLOAT;
				offset = offsetof(Vertex, Weight0);
				break;
			default:
				LogA(false, "Unknown VertexComponent!");
			};
			glEnableVertexAttribArray(m_VertexBufferIndex);
			glVertexAttribPointer(m_VertexBufferIndex,
				count,
				format,
				GL_FALSE,
				sizeof(Vertex),
				(const void*)offset);
			m_VertexBufferIndex++;
		}
		m_VertexBuffer.push_back(vertexbuffer);
	}

	void VertexArray::AddIndexBuffer(const std::shared_ptr<IndexBuffer>& indexbuffer)
	{
		glBindVertexArray(m_RendererID);
		indexbuffer->Bind();

		m_IndexBuffer = indexbuffer;
	}

}