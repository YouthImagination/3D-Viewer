#include "Core/UI.h"

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "IconsMaterialSymbols.h"
#include "material_symbols_rounded_regular.h"
#define MATERIAL_SYMBOLS_DATA g_materialSymbolsRounded_compressed_data
#define MATERIAL_SYMBOLS_SIZE g_materialSymbolsRounded_compressed_size

namespace viewer
{

	void UI::Init(GLFWwindow* window)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Alpha = 0.8f;

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);
		ImGui_ImplOpenGL3_Init("#version 460 core");

		SetDarkThemeColors();
		SetIconMaterial();
	}

	void UI::Shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void UI::NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void UI::EndFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void UI::Update()
	{
		static bool DockSpaceOpen = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoWindowMenuButton;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen) {
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &DockSpaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinX = style.WindowMinSize.x;
		style.WindowMinSize.x = 350.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinX;

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				// Disabling fullscreen would allow the window to be moved to the front of other windows,
				// which we can't undo at the moment without finer window depth/z control.
				if (ImGui::MenuItem(ICON_MS_FILTER_NONE " New", "Ctrl+N"))
				{

				}

				if (ImGui::MenuItem(ICON_MS_FILE_OPEN " Open...", "Ctrl+O"))
				{

				}

				if (ImGui::MenuItem(ICON_MS_FILE_SAVE " Save As...", "Ctrl+Shift+S"))
				{

				}

				if (ImGui::MenuItem(ICON_MS_IMPORT_EXPORT " Import...", "Ctrl+I"))
				{

				}

				if (ImGui::MenuItem(ICON_MS_POWER_SETTINGS_NEW " Exit"))
				{

				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		/*ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };

		uint32 texID = r->GetFinalImage()->GetRendererID();
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(texID)), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();*/
		ImGui::End();
	}

	bool UI::WantCaptureMouse()
	{
		ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureMouse;
	}

	bool UI::WantCaptureKeyboard()
	{
		ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureKeyboard;
	}

	void UI::ImageView(uint32 texID, glm::uvec2& size, glm::vec2& uv1, glm::vec2& uv2)
	{
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(texID)), ImVec2{ (float)size.x, (float)size.y }, ImVec2{ uv1.x, uv1.y }, ImVec2{ uv2.x, uv2.y });
	}

	void UI::Begin(const char* name)
	{
		ImGui::Begin(name);
	}

	void UI::End()
	{
		ImGui::End();
	}

	bool UI::ComboBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
	{
		return ImGui::Combo(label, current_item, items, items_count, height_in_items);
	}

	bool UI::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format)
	{
		return ImGui::SliderFloat3(label, v, v_min, v_max, format);
	}

	bool UI::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format /*= "%.3f"*/)
	{
		return ImGui::DragFloat3(label, v, v_speed, v_min, v_max, format);
	}

	void UI::Text(const char* fmt)
	{
		ImGui::Text(fmt);
	}

	void UI::SameLine()
	{
		return ImGui::SameLine();
	}

	void UI::SetDarkThemeColors(bool useLinearColor /*= false*/)
	{
		typedef ImVec4(*srgbFunction)(float, float, float, float);
		srgbFunction passthrough = [](float r, float g, float b, float a) -> ImVec4 { return ImVec4(r, g, b, a); };
		srgbFunction toLinear = [](float r, float g, float b, float a) -> ImVec4 {
			auto toLinearScalar = [](float u) -> float {
				return u <= 0.04045 ? 25 * u / 323.f : powf((200 * u + 11) / 211.f, 2.4f);
				};
			return ImVec4(toLinearScalar(r), toLinearScalar(g), toLinearScalar(b), a);
			};
		srgbFunction srgb = useLinearColor ? toLinear : passthrough;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.WindowBorderSize = 0.0f;
		style.ColorButtonPosition = ImGuiDir_Right;
		style.FrameRounding = 2.0f;
		style.FrameBorderSize = 1.0f;
		style.GrabRounding = 4.0f;
		style.IndentSpacing = 12.0f;
		style.Colors[ImGuiCol_WindowBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
		style.Colors[ImGuiCol_MenuBarBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarBg] = srgb(0.2f, 0.2f, 0.2f, 1.0f);
		style.Colors[ImGuiCol_PopupBg] = srgb(0.135f, 0.135f, 0.135f, 1.0f);
		style.Colors[ImGuiCol_Border] = srgb(0.4f, 0.4f, 0.4f, 0.5f);
		style.Colors[ImGuiCol_FrameBg] = srgb(0.05f, 0.05f, 0.05f, 0.5f);

		// Normal
		ImVec4                normal_color = srgb(0.465f, 0.465f, 0.525f, 1.0f);
		std::vector<ImGuiCol> to_change_nrm;
		to_change_nrm.push_back(ImGuiCol_Header);
		to_change_nrm.push_back(ImGuiCol_SliderGrab);
		to_change_nrm.push_back(ImGuiCol_Button);
		to_change_nrm.push_back(ImGuiCol_CheckMark);
		to_change_nrm.push_back(ImGuiCol_ResizeGrip);
		to_change_nrm.push_back(ImGuiCol_TextSelectedBg);
		to_change_nrm.push_back(ImGuiCol_Separator);
		to_change_nrm.push_back(ImGuiCol_FrameBgActive);
		for (auto c : to_change_nrm)
		{
			style.Colors[c] = normal_color;
		}

		// Active
		ImVec4                active_color = srgb(0.365f, 0.365f, 0.425f, 1.0f);
		std::vector<ImGuiCol> to_change_act;
		to_change_act.push_back(ImGuiCol_HeaderActive);
		to_change_act.push_back(ImGuiCol_SliderGrabActive);
		to_change_act.push_back(ImGuiCol_ButtonActive);
		to_change_act.push_back(ImGuiCol_ResizeGripActive);
		to_change_act.push_back(ImGuiCol_SeparatorActive);
		for (auto c : to_change_act)
		{
			style.Colors[c] = active_color;
		}

		// Hovered
		ImVec4                hovered_color = srgb(0.565f, 0.565f, 0.625f, 1.0f);
		std::vector<ImGuiCol> to_change_hover;
		to_change_hover.push_back(ImGuiCol_HeaderHovered);
		to_change_hover.push_back(ImGuiCol_ButtonHovered);
		to_change_hover.push_back(ImGuiCol_FrameBgHovered);
		to_change_hover.push_back(ImGuiCol_ResizeGripHovered);
		to_change_hover.push_back(ImGuiCol_SeparatorHovered);
		for (auto c : to_change_hover)
		{
			style.Colors[c] = hovered_color;
		}


		style.Colors[ImGuiCol_TitleBgActive] = srgb(0.465f, 0.465f, 0.465f, 1.0f);
		style.Colors[ImGuiCol_TitleBg] = srgb(0.125f, 0.125f, 0.125f, 1.0f);
		style.Colors[ImGuiCol_Tab] = srgb(0.05f, 0.05f, 0.05f, 0.5f);
		style.Colors[ImGuiCol_TabHovered] = srgb(0.465f, 0.495f, 0.525f, 1.0f);
		style.Colors[ImGuiCol_TabActive] = srgb(0.282f, 0.290f, 0.302f, 1.0f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = srgb(0.465f, 0.465f, 0.465f, 0.350f);

		//Colors_ext[ImGuiColExt_Warning] = srgb (1.0f, 0.43f, 0.35f, 1.0f);

		ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
	}

	ImFont* g_defaultFont = nullptr;
	ImFont* g_iconicFont = nullptr;
	ImFont* g_monospaceFont = nullptr;

	static ImFontConfig GetDefaultConfig()
	{
		ImFontConfig config{};
		config.OversampleH = 3;
		config.OversampleV = 3;
		return config;
	}

	// Helper function to append a font with embedded Material Symbols icons
	// Icon fonts: https://fonts.google.com/icons?icon.set=Material+Symbols
	static ImFont* AppendFontWithMaterialSymbols(const void* fontData, int fontDataSize, float fontSize)
	{
		// Configure Material Symbols icon font for merging
		ImFontConfig iconConfig = GetDefaultConfig();
		iconConfig.MergeMode = true;
		iconConfig.PixelSnapH = true;

		// Material Symbols specific configuration
		float iconFontSize = 1.28571429f * fontSize;  // Material Symbols work best at 9/7x the base font size
		iconConfig.GlyphOffset.x = iconFontSize * 0.01f;
		iconConfig.GlyphOffset.y = iconFontSize * 0.2f;
		iconConfig.DstFont = g_defaultFont;

		// Define the Material Symbols character range
		static const ImWchar materialSymbolsRange[] = { ICON_MIN_MS, ICON_MAX_MS, 0 };
		iconConfig.GlyphRanges = materialSymbolsRange;

		// Load embedded Material Symbols
		ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData, fontDataSize, iconFontSize, &iconConfig);

		return font;
	}

	bool UI::SetIconMaterial()
	{
		ImFontConfig fontConfig = GetDefaultConfig();
		auto font_path = ASSET_DIR + std::string("/Fonts/opensans/OpenSans-Bold.ttf");
		if (g_defaultFont == nullptr)
		{
			g_defaultFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(font_path.c_str(), 18.0f, &fontConfig);
			AppendFontWithMaterialSymbols(MATERIAL_SYMBOLS_DATA, MATERIAL_SYMBOLS_SIZE, 18.0f);
		}
		ImGuiIO& io = ImGui::GetIO();
		io.FontDefault = g_defaultFont;
		io.Fonts->Build();
		return io.FontDefault != nullptr;
	}

}