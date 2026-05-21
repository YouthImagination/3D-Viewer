#include "Renderer/Pipeline.h"
#include "Renderer/TextureManager.h"
#include "Renderer/OpenGLUtils.h"

namespace viewer
{

	Ref<Pipeline> Pipeline::Create(const PipelineCreateInfo& info)
	{
		return CreateRef<Pipeline>(info);
	}

	Pipeline::Pipeline(const PipelineCreateInfo& info)
	{
		m_Info = info;
	}

	Pipeline::~Pipeline()
	{

	}

	void Pipeline::Bind()
	{
		GL_CHECK(glDepthMask(m_Info.DepthWrite ? GL_TRUE : GL_FALSE));
		auto depthOp = GetDepthCompareOp(m_Info.DepthCompareOp);
		GL_CHECK(glDepthFunc(depthOp));
		if (m_Info.DepthTest)
			GL_CHECK(glEnable(GL_DEPTH_TEST));
		else
			GL_CHECK(glDisable(GL_DEPTH_TEST));
		if (m_Info.BlendEnable)
		{
			GL_CHECK(glEnable(GL_BLEND));
		}
		else
		{
			GL_CHECK(glDisable(GL_BLEND));
		}
		GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		GLenum cullMode = GetCullMode(m_Info.CullMode);
		GL_CHECK(glCullFace(cullMode));
		GL_CHECK(glFrontFace(GL_CCW));
		GL_CHECK(glLineWidth(m_Info.LineWidth));

		GLenum mode = GetFillMode(m_Info.FillMode);
		GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, mode));

		if (m_Info.Shader)
			m_Info.Shader->Bind();
	}

	void Pipeline::SetLineWidth(float width)
	{
		m_Info.LineWidth = width;
	}

	void Pipeline::SetInput(const String& name, const Ref<Texture>& resource)
	{
		if (m_TextureInputs.find(name) != m_TextureInputs.end())
		{
			LogW("Texture input with name {0} already exists in pipeline {1}!", name, m_Info.Name);
			return;
		}
		m_TextureInputs.insert(name);
	}

	void Pipeline::SetInput(const String& name, const Ref<UniformBuffer>& resource)
	{
		if (m_UniformBufferInputs.find(name) != m_UniformBufferInputs.end())
		{
			LogW("Uniform input with name {0} already exists in pipeline {1}!", name, m_Info.Name);
			return;
		}
		m_UniformBufferInputs[name] = resource;
	}

	Ref<Texture> Pipeline::GetInput(const String& name)
	{
		if (m_TextureInputs.find(name) != m_TextureInputs.end())
		{
			return TextureManager::GetTexture(name);
		}
		return nullptr;
	}

	void Pipeline::Bake()
	{
		// prepare shader setting here
		// , such as binding texture units
	}

}