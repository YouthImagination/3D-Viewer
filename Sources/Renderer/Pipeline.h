#pragma once

#include "Core/Base.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderPass.h"

namespace viewer
{
	enum class Topology
	{
		TRIANGLE_LIST = 1,
		LINE_LIST,
		POINT_LIST
	};

	enum class CullMode
	{
		NONE,
		BACK,
		FRONT,
		FRONT_AND_BACK
	};

	enum class FillMode
	{
		POINT,
		SOLID,
		WIREFRAME
	};

	enum class BlendFactor
	{
		NONE,
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA
	};

	enum class BlendOp
	{
		NONE,
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX
	};

	enum class StencilOp
	{
		NONE,
		KEEP,
		ZERO,
		REPLACE,
		INCREMENT_AND_CLAMP,
		DECREMENT_AND_CLAMP,
		INVERT
	};

	enum class DepthCompareOperator
	{
		NONE,
		LESS,
		LESS_OR_EQUAL,
		GREATER,
		GREATER_OR_EQUAL
	};

	struct PipelineCreateInfo
	{
		String Name;
		Ref<RenderPass> RenderPass;
		Ref<Shader> Shader;
		Topology Topology = Topology::TRIANGLE_LIST;
		CullMode CullMode = CullMode::BACK;
		FillMode FillMode = FillMode::SOLID;
		bool DepthTest = true;
		bool DepthWrite = true;
		float LineWidth = 1.0f;
		DepthCompareOperator DepthCompareOp = DepthCompareOperator::LESS_OR_EQUAL;
		bool BlendEnable = false;
	};

	class Pipeline
	{
	public:
		static Ref<Pipeline> Create(const PipelineCreateInfo& info);
		Pipeline(const PipelineCreateInfo& info);
		~Pipeline();

		void Bind();

		void SetLineWidth(float width);

		void SetInput(const String& name, const Ref<Texture>& resource);
		void SetInput(const String& name, const Ref<UniformBuffer>& resource);
		Ref<Texture> GetInput(const String& name);

		void Bake();

		Ref<Shader>& GetShader() { return m_Info.Shader; }
		const PipelineCreateInfo& GetInfo() const { return m_Info; }

	protected:
		PipelineCreateInfo m_Info;
		std::set<String> m_TextureInputs;
		std::unordered_map<String, Ref<UniformBuffer>> m_UniformBufferInputs;
	};
}