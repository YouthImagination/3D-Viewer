#pragma once

#include "camera.h"
#include "model.h"
#include "logger.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>
#include <mutex>
#include <functional>

struct FontInfo {
    std::string filename;
    std::string path;
    std::string displayName;
};

class GUI {
public:
    GUI() = default;
    ~GUI() = default;

    // ImGui lifecycle
    void init(GLFWwindow* window, float dpiScale);
    void shutdown();
    void newFrame();
    void draw(float deltaTime, Model& model, ArcballCamera& camera,
              GLFWwindow* window, float dpiScale,
              const std::function<void()>& resetCameraFn);
    void render();

    // Settings accessors (used by Renderer/ViewerApp)
    float lightAzimuth() const { return lightAzimuth_; }
    float lightElevation() const { return lightElevation_; }
    float lightIntensity() const { return lightIntensity_; }
    float ambientIntensity() const { return ambientIntensity_; }
    glm::vec3 lightColor() const { return lightColor_; }
    glm::vec3 getLightDirection() const;
    bool wireframeMode() const { return wireframeMode_; }
    bool showBoundingBox() const { return showBoundingBox_; }
    float boundingBoxAlpha() const { return boundingBoxAlpha_; }
    bool isPlaying() const { return isPlaying_; }
    float playbackSpeed() const { return playbackSpeed_; }
    int activeAnimationIndex() const { return activeAnimationIndex_; }
    float sidebarWidth() const { return sidebarWidth_; }
    float bottomBarHeight() const { return bottomBarHeight_; }
    float menuBarHeight() const { return menuBarHeight_; }
    bool orthoProjection() const { return orthoProjection_; }
    bool useMipmap() const { return useMipmap_; }

    // Mutators
    void setAnimationPlaying(bool playing) { isPlaying_ = playing; }
    void setActiveAnimationIndex(int idx) { activeAnimationIndex_ = idx; }
    void setAnimationTimeTicks(float t) { animationTimeTicks_ = t; }
    float animationTimeTicks() const { return animationTimeTicks_; }
    void setUseMipmap(bool v) { useMipmap_ = v; }

    // Download/model management
    bool consumeDownloadedModel(std::vector<char>& outData);
    bool checkPendingRandomLoad();
    void startDownload(const std::string& url);
    void startFetchIndex();
    void processRandomLoad();
    bool isDownloadRunning() const;
    std::string getDownloadStatus();

    bool isSelectFileModel() const;
    void clearSelectedFileModelStatus();
    std::string getSelectedModelFile();

    // Index state
    bool isIndexReady() const;
    bool isIndexFetching() const;
    std::string getIndexStatus();
    std::vector<std::pair<std::string, std::string>> getWebModels();

    // Random load
    void setPendingRandomLoad(bool v) { pendingRandomLoad_ = v; }
    bool pendingRandomLoad() const { return pendingRandomLoad_; }
    int selectedModelIndex() const { return selectedModelIndex_; }

    // Font management
    void changeActiveFont(const std::string& fontPath);
    void saveSettings();
    ImFont* activeFontPtr() const { return activeFont_; }

    // Screenshot
    bool triggerScreenshot() const { return triggerScreenshot_; }
    void setTriggerScreenshot(bool v) { triggerScreenshot_ = v; }
    bool screenshotPending() const { return screenshotPending_; }
    void setScreenshotPending(bool v) { screenshotPending_ = v; }
    bool saveScreenShot() const { return saveScreenShot_; }
    void setSaveScreenShot(bool v) { saveScreenShot_ = v; }
    void updateScreenShotImage(uint32_t id, int w, int h) {
        texRef_ = ImTextureRef(ImTextureID(id));
        screenshotWidth_ = w;
        screenshotHeight_ = h;
    }
    std::string_view savedScreenShotName() { return savedName_; }

    // Shader hot-reload
    bool shaderReloadRequested() const { return shaderReloadRequested_; }
    void setShaderReloadRequested(bool v) { shaderReloadRequested_ = v; }

private:
    // Sub-draw methods
    void drawMenuBar(GLFWwindow* window, const std::function<void()>& resetCameraFn);
    void drawSidebar(float deltaTime, Model& model, ArcballCamera& camera,
                     GLFWwindow* window, const std::function<void()>& resetCameraFn);
    void drawBottomPlayback(Model& model);
    void drawSettingsWindow();
    void drawLogWindow();

    // Custom ImGui widgets
    bool drawIconTab(const char* id_str, int index, int& activeIndex, ImVec2 size);
    bool drawLightRotationDial(const char* label, float& azimuth);

    // Font helpers
    void scanAvailableFonts();

    // --- UI state ---
    int activeTab_ = 0;
    float dpiScale_ = 1.0f;
    float sidebarWidth_ = 360.0f;
    float bottomBarHeight_ = 65.0f;
    float menuBarHeight_ = 25.0f;

    float lightAzimuth_ = 45.0f;
    float lightElevation_ = 45.0f;
    float lightIntensity_ = 1.2f;
    float ambientIntensity_ = 0.20f;
    glm::vec3 lightColor_ = glm::vec3(1.0f);

    bool isPlaying_ = true;
    float playbackSpeed_ = 1.0f;
    int activeAnimationIndex_ = 1;
    float animationTimeTicks_ = 0.0f;

    bool showSettingsWindow_ = false;
    bool showLogWindow_ = false;
    bool showFileDialog_ = false;
    bool fileModelSelected_ = false;
    std::string selectedModelFile_;

    bool orthoProjection_ = false;
    bool wireframeMode_ = false;
    bool useMipmap_ = true;
    bool triangleVisibility_ = true;
    bool hasModel_ = true;
    bool showBoundingBox_ = false;
    float boundingBoxAlpha_ = 1.0f;

    // Font state
    std::vector<FontInfo> availableFonts_;
    std::string currentFontPath_ = "";
    ImFont* activeFont_ = nullptr;
    ImFont* fontDefault_ = nullptr;
    char fontFilter_[128] = "";

    // URL input
    char urlInputBuf_[512] = "https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/Models";

    // Download state
    std::vector<char> webModelData_;
    mutable std::mutex downloadMutex_;
    bool downloadReady_ = false;
    bool downloadRunning_ = false;
    std::string downloadStatus_ = "Idle";

    // Index state
    std::vector<std::pair<std::string, std::string>> webModels_;
    mutable std::mutex indexMutex_;
    bool indexReady_ = false;
    bool indexFetching_ = false;
    std::string indexStatus_ = "Click 'Fetch' to retrieve online models";
    int selectedModelIndex_ = 0;
    bool pendingRandomLoad_ = false;

    // Screenshot
    bool triggerScreenshot_ = false;
    bool screenshotPending_ = false;
    bool saveScreenShot_ = false;
	ImTextureRef texRef_{};
    std::string savedName_;
    char screenshotNameBuf_[256] = "screenshot.png";
    int screenshotWidth_ = 0;
    int screenshotHeight_ = 0;

    // Shader hot-reload
    bool shaderReloadRequested_ = false;

    // Log
    // Note: uses global appLog from logger.h, not a member
};
