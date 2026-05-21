#include "Renderer/RenderPass.h"
#include "Renderer/OpenGLUtils.h"

#include <glad/gl.h>

namespace viewer
{

	Ref<viewer::RenderPass> RenderPass::Create(const RenderPassCreateInfo& info)
	{
		return CreateRef<RenderPass>(info);
	}

	RenderPass::RenderPass(const RenderPassCreateInfo& info)
		: m_Info(info), m_FrameBufferID(0), m_RenderBufferID(0)
	{
		GL_CHECK(glCreateFramebuffers(1, &m_FrameBufferID));
	}

	RenderPass::~RenderPass()
	{
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		GL_CHECK(glDeleteFramebuffers(1, &m_FrameBufferID));
		GL_CHECK(glDeleteRenderbuffers(1, &m_RenderBufferID));
		m_FrameBufferID = 0;
		m_RenderBufferID = 0;
	}

	void RenderPass::Begin()
	{
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));
		GL_CHECK(glViewport(0, 0, m_Info.Width, m_Info.Height));

		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		GL_CHECK(glClearColor(m_Info.DebugColor.r, m_Info.DebugColor.g, m_Info.DebugColor.b, m_Info.DebugColor.a));
	}

	void RenderPass::End()
	{
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	void RenderPass::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;
		GL_CHECK(glDeleteRenderbuffers(1, &m_RenderBufferID));
		m_RenderBufferID = 0;
		Bake();
	}

	void RenderPass::Bake()
	{
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

		int colorAttachmentCount = 0;
		std::vector<GLenum> attachments;
		bool depthAttached = false;
		for (int i = 0; i < m_Outputs.size(); i++) {
			auto format = m_Outputs[i].Texture->GetFormat();
			if (Texture::IsDepthFormat(format)) {
				if (!depthAttached) {
					if (!m_RenderBufferID)
						glCreateRenderbuffers(1, &m_RenderBufferID);
					GL_CHECK(glNamedRenderbufferStorage(m_RenderBufferID,
						GetInternalTextureFormat(format),
						m_Info.Width, m_Info.Height));
					GL_CHECK(glNamedFramebufferRenderbuffer(m_FrameBufferID,
						GL_DEPTH_STENCIL_ATTACHMENT,
						GL_RENDERBUFFER, m_RenderBufferID));
					depthAttached = true;
				}
			}
			else {
				GLenum attachment = GL_COLOR_ATTACHMENT0 + colorAttachmentCount;
				GL_CHECK(glNamedFramebufferTexture(m_FrameBufferID, attachment,
					m_Outputs[i].Texture->GetRendererID(), 0));
				attachments.push_back(attachment);
				colorAttachmentCount++;
			}
		}

		if (colorAttachmentCount > 0) {
			GL_CHECK(glNamedFramebufferDrawBuffers(m_FrameBufferID, colorAttachmentCount, attachments.data()));
		}
		else {
			GL_CHECK(glNamedFramebufferDrawBuffers(m_FrameBufferID, 0, nullptr));
		}

		GLenum status = glCheckNamedFramebufferStatus(m_FrameBufferID, GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			LogE("Framebuffer incomplete: {}\n", status);
			LogA(false, "Framebuffer incomplete");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderPass::SetOutput(const RenderTarget& target)
	{
		m_Outputs.push_back(target);
	}

	Ref<viewer::Texture2D> RenderPass::GetOutput(const String& name) const
	{
		auto it = std::find_if(m_Outputs.begin(), m_Outputs.end(),
			[&](RenderTarget rt) { return rt.Texture->GetName() == name; });

		if (it != m_Outputs.end())
		{
			int index = std::distance(m_Outputs.begin(), it);
			return m_Outputs[index].Texture;
		}
		return nullptr;
	}

	Ref<viewer::Texture2D> RenderPass::GetOutput(uint32 index) const
	{
		if (index < 0 && index >= m_Outputs.size())
			return nullptr;
		return m_Outputs[index].Texture;
	}

	Ref<viewer::Texture2D> RenderPass::GetDepthOutput() const
	{
		auto it = std::find_if(m_Outputs.begin(), m_Outputs.end(),
			[&](RenderTarget rt) { return Texture::IsDepthFormat(rt.Texture->GetFormat()); });
		if (it != m_Outputs.end())
		{
			int index = std::distance(m_Outputs.begin(), it);
			return m_Outputs[index].Texture;
		}
		return nullptr;
	}

	uint32 RenderPass::GetColorTargetsCount() const
	{
		uint32 count = 0;
		for (const auto& target : m_Outputs)
		{
			if (Texture::IsColorFormat(target.Texture->GetFormat()))
			{
				count++;
			}
		}

		return count;
	}

	bool RenderPass::HasDepthRenderTarget() const
	{
		for (const auto& target : m_Outputs)
		{
			if (Texture::IsDepthFormat(target.Texture->GetFormat()))
			{
				return true;
			}
		}

		return false;
	}

}