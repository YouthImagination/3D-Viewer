#include "Renderer/UniformBuffer.h"
#include "Renderer/OpenGLUtils.h"

#include "glad/gl.h"

namespace viewer
{

	UniformBuffer::UniformBuffer(const String& name, uint32_t size, uint32_t binding)
		: m_Name(name)
	{
		GL_CHECK(glCreateBuffers(1, &m_RendererID));
		GL_CHECK(glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW));
		GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID));
	}

	UniformBuffer::UniformBuffer(const String& name, uint32_t size)
		: m_Name(name)
	{
		GL_CHECK(glCreateBuffers(1, &m_RendererID));
		GL_CHECK(glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW));
	}

	UniformBuffer::~UniformBuffer()
	{
		if (m_RendererID)
			GL_CHECK(glDeleteBuffers(1, &m_RendererID));
	}

	void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset /*= 0*/)
	{
		GL_CHECK(glNamedBufferSubData(m_RendererID, offset, size, data));
	}

	bool UniformBuffer::MapAndUpdateData(const void* data, size_t size)
	{
		GL_CHECK(glNamedBufferSubData(m_RendererID, 0, size, data));
		return true;
	}

	int UniformBuffer::GetBinding(const std::string& name, uint programID)
	{
		GLuint blockIndex = GetLocation(name, programID);
		GLint binding;
		glGetActiveUniformBlockiv(m_RendererID, blockIndex, GL_UNIFORM_BLOCK_BINDING, &binding);
		return binding;
	}

	int UniformBuffer::GetLocation(const std::string& name, uint programID)
	{
		GLuint blockIndex = glGetUniformBlockIndex(programID, name.c_str());
		if (blockIndex == GL_INVALID_INDEX) {
			LogE("Failed to find block: {}", name);
		}
		return blockIndex;
	}

}