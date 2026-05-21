#include "Core/Application.h"
#include "Renderer/Renderer.h"
#include "Renderer/OpenGLUtils.h"
#include "Renderer/TextureManager.h"
#include "Core/UI.h"

#include <glad/gl.h>
#include "glfw/glfw3.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


namespace viewer
{
	RenderConfig g_renderConfig{};
	static Renderer* renderer = nullptr;

	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	struct DisplaySettings
	{
		glm::uvec2 viewportSize = { 1280, 720 };
		float AspectRatio = 1280.0f / 720.0f;
	} displaySetting;

	struct MouseStates
	{
		bool firstMouse = true;
		int lastX = displaySetting.viewportSize.x * 0.5;
		int lastY = displaySetting.viewportSize.y * 0.5;
	} mouse;

	static void glfwErrorCallback(int error, const char* description) {
		LogE("Glfw Error {}: {}", error, description);
	}

	static void ProcessInput(GLFWwindow* window) {
		if (!renderer || UI::WantCaptureKeyboard()) {
			return;
		}

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
			return;
		}

		static bool keyPressed_H = false;
		int state = glfwGetKey(window, GLFW_KEY_H);
		if (state == GLFW_PRESS) {
			if (!keyPressed_H) {
				keyPressed_H = true;
				//renderer->TogglePanelState();
			}
		}
		else if (state == GLFW_RELEASE) {
			keyPressed_H = false;
		}
	}

	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
		// make sure the viewport matches the new window dimensions; note that width and
		// height will be significantly larger than specified on retina displays.
		//glViewport(0, 0, width, height);

		if (!renderer) {
			return;
		}
		renderer->UpdateSize(width, height);
	}

	// glfw: whenever the mouse moves, this callback is called
	// -------------------------------------------------------
	static void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
		if (!renderer || !UI::WantCaptureMouse()) {
			return;
		}

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			if (mouse.firstMouse) {
				mouse.lastX = xPos;
				mouse.lastY = yPos;
				mouse.firstMouse = false;
			}

			double xOffset = xPos - mouse.lastX;
			double yOffset = yPos - mouse.lastY;

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
				renderer->UpdateCameraPan((float)xOffset, (float)yOffset);
			}
			else {
				renderer->UpdateCameraRotate((float)xOffset, (float)yOffset);
			}

			mouse.lastX = xPos;
			mouse.lastY = yPos;
		}
		else {
			mouse.firstMouse = true;
		}
	}

	// glfw: whenever the mouse scroll wheel scrolls, this callback is called
	// ----------------------------------------------------------------------
	static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
		if (!renderer || !UI::WantCaptureMouse()) {
			return;
		}

		renderer->UpdateCameraZoom((float)xOffset, (float)yOffset);
	}

	static void DropCallback(GLFWwindow* window, int path_count, const char* paths[])
	{

	}

	Application::Application()
	{
		Log::Init();

		glfwSetErrorCallback(glfwErrorCallback);

		if (!glfwInit())
		{
			LogE("Failed to Init GLFW!");
			glfwTerminate();
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		if (!window)
		{
			window = glfwCreateWindow(1280, 720, "3D-Viewer", nullptr, nullptr);
			if (!window)
			{
				LogE("Failed to create GLFW window!");
				glfwTerminate();
			}
		}

		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
		glfwSetCursorPosCallback(window, MouseCallback);
		glfwSetScrollCallback(window, ScrollCallback);
		glfwSetDropCallback(window, DropCallback);

		if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
			LogE("Failed to initialize GLAD2!");
			glfwTerminate();
		}

		UI::Init(window);

		TextureManager::Init();

		g_renderConfig.Width = displaySetting.viewportSize.x;
		g_renderConfig.Height = displaySetting.viewportSize.y;
		g_renderConfig.camera = CreateScope<Camera>(displaySetting.viewportSize, 60.0f, displaySetting.AspectRatio, 0.1f, 1000.0f);
		g_renderConfig.scene = CreateScope<Scene>();
		g_renderConfig.scene->Models.push_back(CreateRef<Model>(GetAssetDirs() + "Models/vulkanscene_plane.glb"));
		g_renderConfig.scene->Models.push_back(CreateRef<Model>(GetAssetDirs() + "Models/cyborg/cyborg.obj"));
		renderer = new Renderer(g_renderConfig);
	}

	Application::~Application()
	{
		if (renderer)
			delete renderer;
		UI::Shutdown();
		if (window)
		{
			glfwDestroyWindow(window);
			glfwTerminate();
		}
	}

	int Application::Run()
	{

		while (!glfwWindowShouldClose(window))
		{
			ProcessInput(window);
			glfwPollEvents();

			// timer
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			UI::NewFrame();

			renderer->NewFrame();
			renderer->DrawFrame(deltaTime);
			renderer->EndFrame();

			// UI
			uint32 texID = renderer->GetFinalImage()->GetRendererID();
			UI::Begin("Viewport");
			UI::ImageView(texID, displaySetting.viewportSize);
			UI::End();
			UI::EndFrame();

			glfwSwapBuffers(window);
			renderer->WaitIdle();
		}

		return EXIT_SUCCESS;
	}

}