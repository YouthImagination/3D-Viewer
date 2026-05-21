#pragma once

#include "Renderer/Texture.h"

#include <glm/glm.hpp>

namespace viewer
{
	enum class RenderTargetLoadOp
	{
		DONT_CARE,
		CLEAR = 2,
		LOAD = 1
	};

	struct RenderTarget
	{
		RenderTarget() = default;
		RenderTarget(const String& name, uint32 width, uint32 height, TextureFormat format, TextureFilter filter = TextureFilter::LINEAR)
		{
			uint32 usage = TextureUsage::ATTACHMENT | TextureUsage::SAMPLED;

			TextureCreateInfo texInfo;
			texInfo.Name = name;
			texInfo.Format = format;
			texInfo.Usage = (TextureUsage)usage;
			texInfo.Width = width;
			texInfo.Height = height;
			texInfo.Layers = 1;
			texInfo.GenerateMipMap = false;
			texInfo.Sampler.Filter = filter;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			Texture = CreateRef<Texture2D>(texInfo);
			LoadOp = RenderTargetLoadOp::CLEAR;
		}

		RenderTarget(const String& name, const TextureCreateInfo& texInfo)
		{
			uint32 usage = TextureUsage::ATTACHMENT | TextureUsage::SAMPLED;

			Texture = CreateRef<Texture2D>(texInfo);
			LoadOp = RenderTargetLoadOp::CLEAR;
		}

		RenderTarget(const Ref<Texture2D>& texture, RenderTargetLoadOp loadOp = RenderTargetLoadOp::LOAD)
		{
			Texture = texture;
			LoadOp = loadOp;
		}

		Ref<Texture2D> Texture;
		RenderTargetLoadOp LoadOp = RenderTargetLoadOp::CLEAR;
		glm::vec4 ClearColor = { 0.f, 0.f, 0.f, 1.0f };
		float DepthClearColor = 0.f;
		uint32 StencilClearColor = 1.f;
	};

	class RenderPass;

	struct RenderPassCreateInfo
	{
		String Name;
		Ref<RenderPass> InputPass;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		glm::vec4 DebugColor = glm::vec4(0.f, 0.f, 0.f, 0.f);
	};

	class RenderPass
	{
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& info);
		RenderPass(const RenderPassCreateInfo& info);
		~RenderPass();

		void Begin();
		void End();

		void Resize(uint32 width, uint32 height);
		void Bake();

		void SetOutput(const RenderTarget& target);

		Ref<Texture2D> GetOutput(const String& name) const;
		Ref<Texture2D> GetOutput(uint32 index) const;
		Ref<Texture2D> GetDepthOutput() const;

		const auto& GetAllOutputs() const { return m_Outputs; }

		uint32 GetColorTargetsCount() const;
		bool HasDepthRenderTarget() const;

		const RenderPassCreateInfo& GetInfo() const { return m_Info; }

	protected:
		RenderPassCreateInfo m_Info;
		std::vector<RenderTarget> m_Outputs;

		uint32 m_FrameBufferID;
		uint32 m_RenderBufferID;
	};
}