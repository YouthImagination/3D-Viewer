#pragma once

#include "Core/Base.h"
#include "glm/glm.hpp"

struct GLFWwindow;

namespace viewer
{
	class UI
	{
	public:
		static void Init(GLFWwindow* window);
		static void Shutdown();
		static void NewFrame();
		static void EndFrame();
		static void Update();
		static bool WantCaptureMouse();
		static bool WantCaptureKeyboard();
		static void ImageView(uint32 id, glm::uvec2& size, glm::vec2& uv1 = glm::vec2(0, 1), glm::vec2& uv2 = glm::vec2(1, 0));
		static void Begin(const char* name);
		static void End();
		static bool ComboBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
		static bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f");
		static bool DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format = "%.3f");
		static void Text(const char* fmt);
		static void SameLine();
	private:
		static void SetDarkThemeColors(bool useLinearColor = false);
		static bool SetIconMaterial();
	};
}