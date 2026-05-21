#include "Renderer/Renderer.h"
#include "Renderer/OpenGLUtils.h"
#include "Renderer/TextureManager.h"

#include "Core/UI.h"

#include <glm/gtc/matrix_transform.hpp>

namespace viewer
{

	Renderer::Renderer(RenderConfig& config /*= {}*/)
	{
		UpdateConfig(config);
		InitPipelines();
		lightModel = CreateRef<Model>(GetAssetDirs() + "Models/sphere.obj");
		lightModelShader = CreateRef<Shader>(GetShaderDirs() + "light.glsl");
		lightModelUBO = CreateRef<UniformBuffer>("u_Color", sizeof(glm::vec4), 5);

		lightBlock.numLights = 4;

		// directional
		lightItems.push_back("Directional Light 1");
		lightBlock.lights[0].type = int(LightType::Directional);
		lightBlock.lights[0].direction = glm::vec4(-0.2f, -1.0f, -0.3f, 1.0f);
		lightBlock.lights[0].ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
		lightBlock.lights[0].diffuse = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
		lightBlock.lights[0].specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		// point 1
		lightItems.push_back("Point Light 1");
		lightBlock.lights[1].type = int(LightType::Point);
		lightBlock.lights[1].position = glm::vec4(0.7f, 0.2f, 2.0f, 1.0);
		lightBlock.lights[1].ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0);
		lightBlock.lights[1].diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0);
		lightBlock.lights[1].specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0);
		lightBlock.lights[1].linear = 0.09f;
		lightBlock.lights[1].quadratic = 0.032f;
		// point 2
		lightItems.push_back("Point Light 2");
		lightBlock.lights[2].type = int(LightType::Point);
		lightBlock.lights[2].position = glm::vec4(2.3f, -3.3f, -4.0f, 1.0);
		lightBlock.lights[2].ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0);
		lightBlock.lights[2].diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0);
		lightBlock.lights[2].specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0);
		lightBlock.lights[2].linear = 0.09f;
		lightBlock.lights[2].quadratic = 0.032f;
		// spot
		lightItems.push_back("Spot Light 1");
		lightBlock.lights[3].type = int(LightType::Spot);
		lightBlock.lights[3].position = glm::vec4(camera->GetPosition(), 1.0f);
		lightBlock.lights[3].direction = glm::vec4(camera->GetForwardDirection(), 1.0f);
		lightBlock.lights[3].ambient = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		lightBlock.lights[3].diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightBlock.lights[3].specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightBlock.lights[3].linear = 0.09f;
		lightBlock.lights[3].quadratic = 0.032f;
		lightBlock.lights[3].outerCutOff = glm::cos(glm::radians(15.0f));
	}

	Renderer::~Renderer()
	{
		
	}

	void Renderer::UpdateConfig(RenderConfig& config)
	{
		activeScene = std::move(config.scene);
		camera = std::move(config.camera);
		m_Width = config.Width;
		m_Height = config.Height;
	}

	void Renderer::UpdateCameraPan(float x, float y)
	{
		camera->MousePan({ x, y });
	}

	void Renderer::UpdateCameraRotate(float x, float y)
	{
		camera->MouseRotate({ x, y });
	}

	void Renderer::UpdateCameraZoom(float x, float y)
	{
		camera->MouseZoom(y);
	}

	void Renderer::UpdateSize(int w, int h)
	{
		camera->SetViewportSize(w, h);
	}

	void Renderer::NewFrame()
	{
	}

	void Renderer::DrawFrame(float deltaTime)
	{
		// UpdateUI
		UpdateUI();
		// update Uniform
		UpdateSceneMatrices();
		UpdateLightsMatrices();

		// model render pass
		ModelPipeline->Bind();
		ModelRenderPass->Begin();
		ModelDraw();
		ModelRenderPass->End();
	}

	void Renderer::EndFrame()
	{
	}

	void Renderer::WaitIdle()
	{
		glFinish();
	}

	void Renderer::AddLight(const Light& light)
	{

	}

	Ref<Texture2D> Renderer::GetFinalImage()
	{
		return ModelRenderPass->GetOutput(0);
	}

	void Renderer::UpdateSceneMatrices()
	{
		matrices.View = camera->GetViewMatrix();
		matrices.Proj = camera->GetProjectionMatrix();
		matrices.ViewPos = glm::vec4(camera->GetPosition(), 1.0f);
		m_SceneUniformBuffer->MapAndUpdateData(&matrices, sizeof(matrices));
	}

	void Renderer::UpdateLightsMatrices()
	{
		if (bLightChange)
		{
			m_LightsUniformBuffer->MapAndUpdateData(&lightBlock, sizeof(LightBlock));
			bLightChange = false;
		}
		m_MaterialUniformBuffer->MapAndUpdateData(&material, sizeof(material));
	}

	void Renderer::ModelDraw(bool drawLight)
	{
		for (auto& model : activeScene->Models)
		{
			m_ModelUniformBuffer->MapAndUpdateData(&model->PositionMatrix(), sizeof(glm::mat4));
			model->Draw();
		}
		if (drawLight)
		{
			lightModelShader->Bind();
			for (auto i = 0; i < lightItems.size(); i++)
			{
				Light l = lightBlock.lights[i];
				if (l.type == int(LightType::Directional))
					continue;
				glm::vec3 p = glm::vec3(l.position);
				glm::mat4 m = glm::translate(glm::mat4(1.0f), p) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
				m_ModelUniformBuffer->MapAndUpdateData(&m, sizeof(glm::mat4));
				lightModelUBO->MapAndUpdateData(&l.diffuse, sizeof(glm::vec4));
				lightModel->Draw();
			}
		}
	}

	int currentLightIndex = 0;

	void Renderer::UpdateUI()
	{
		UI::Update();
		UI::Begin("Light");
		if (UI::ComboBox("Index", &currentLightIndex, lightItems.data(), lightItems.size(), lightItems.size()))
		{
		}
		UI::Text("Type: ");
		UI::SameLine();
		UI::Text(lightItems[currentLightIndex]);
		glm::vec3 pos = glm::vec3(lightBlock.lights[currentLightIndex].position);
		if (UI::DragFloat3("Position", &pos.x, 0.1f, -50.0f, 50.0f))
		{
			lightBlock.lights[currentLightIndex].position = glm::vec4(pos, 1.0f);
			bLightChange = true;
		};
		glm::vec3 diffColor = glm::vec3(lightBlock.lights[currentLightIndex].diffuse);
		if (UI::DragFloat3("Diffuse", &diffColor.x, 0.05f, 0.0f, 1.0f))
		{
			lightBlock.lights[currentLightIndex].diffuse = glm::vec4(diffColor, 1.0f);
			bLightChange = true;
		};
		UI::End();
	}

	void Renderer::InitPipelines()
	{
		RenderPassCreateInfo renderPassCI{};
		renderPassCI.Width = m_Width;
		renderPassCI.Height = m_Height;
		renderPassCI.DebugColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
		ModelRenderPass = RenderPass::Create(renderPassCI);

		RenderTarget target = RenderTarget{ "SceneOutput", m_Width, m_Height, TextureFormat::RGBA8 };
		ModelRenderPass->SetOutput(target);
		target = RenderTarget{ "Depth", m_Width, m_Height, TextureFormat::DEPTH24STENCIL8 };
		ModelRenderPass->SetOutput(target);
		ModelRenderPass->Bake();

		PipelineCreateInfo compCI{};
		compCI.Name = "ModelPipeline";
		compCI.Shader = CreateRef<Shader>(GetShaderDirs() + "model.glsl");
		m_SceneUniformBuffer = CreateRef<UniformBuffer>("u_Matrices", sizeof(matrices), 0);
		m_ModelUniformBuffer = CreateRef<UniformBuffer>("u_Model", sizeof(glm::mat4), 1);
		m_LightsUniformBuffer = CreateRef<UniformBuffer>("u_Lights", sizeof(LightBlock), 2);
		m_MaterialUniformBuffer = CreateRef<UniformBuffer>("u_Material", sizeof(Material), 3);
		ModelPipeline = Pipeline::Create(compCI);
		ModelPipeline->SetInput("TextureDiffuse", TextureManager::GetTexture("TextureDiffuse"));
		ModelPipeline->SetInput("u_Matrices", m_SceneUniformBuffer);
		ModelPipeline->Bake();
	}

}