#pragma once

#include "Core/Base.h"
#include "Core/Camera.h"
#include "Renderer/Scene.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Light.h"

namespace viewer
{
	struct RenderConfig
	{
		Scope<Scene> scene = nullptr;
		Scope<Camera> camera = nullptr;
		uint32 Width = 1;
		uint32 Height = 1;
	};

	class Renderer
	{
	public:
		Renderer(RenderConfig& config);
		~Renderer();

		void UpdateConfig(RenderConfig& config);

		void UpdateCameraPan(float x, float y);
		void UpdateCameraRotate(float x, float y);
		void UpdateCameraZoom(float x, float y);
		void UpdateSize(int w, int h);

		void NewFrame();
		void DrawFrame(float deltaTime);
		void EndFrame();
		void WaitIdle();

		void AddLight(const Light& light);

		Ref<Texture2D> GetFinalImage();
	private:
		void InitPipelines();
		void UpdateSceneMatrices();
		void UpdateLightsMatrices();
		void ModelDraw(bool drawLight = true);
		void UpdateUI();
	private:
		Scope<Scene> activeScene;
		Scope<Camera> camera;
		SceneData matrices;
		LightBlock lightBlock;
		Material material;
		bool bLightChange = true;
	private:
		uint32 m_Width;
		uint32 m_Height;

		Ref<UniformBuffer> m_SceneUniformBuffer;
		Ref<UniformBuffer> m_ModelUniformBuffer;
		Ref<UniformBuffer> m_LightsUniformBuffer;
		Ref<UniformBuffer> m_MaterialUniformBuffer;
		Ref<Pipeline> ModelPipeline;
		Ref<RenderPass> ModelRenderPass;

		Ref<Model> lightModel;
		Ref<Shader> lightModelShader;
		Ref<UniformBuffer> lightModelUBO;
		std::vector<const char*> lightItems;
	};
}