#include "viewer_app.h"
#include "logger.h"
#include "backend.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

// ============================================================
// Constructor / Destructor
// ============================================================

ViewerApp::ViewerApp(uint32_t w, uint32_t h)
    : Application(w, h) {
    currentAPIStr_ = BackendInstance->isOpenGL() ? "gl" : "vk";
}

// ============================================================
// onAttach - Initialize resources
// ============================================================

void ViewerApp::onAttach() {
    // Compute DPI scale
    float xScale = 1.0f, yScale = 1.0f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    setDpiScale(xScale);

    // Initialize GUI (ImGui context, theme, fonts, backends)
    gui_.init(windowHandle_, xScale);

    // Load default model
    if (!model_.loadFromFile("Bee.glb")) {
        LOG_ERROR << "Failed to load Bee.glb" << std::endl;
    }

    modelCenter_ = model_.getCenter();
    modelScale_ = model_.getScale();
    modelSize_ = model_.getModelSize();
    globalModelExtent_ = model_.getExtent();
    globalModelScale_ = model_.getScale();

    // Create renderer and add main render pass
    auto mainPass = std::make_unique<MainRenderPass>();
    renderer_.addPass(std::move(mainPass));
    renderer_.init(width_, height_);

    // Create screenshot PBO
    screenshotPBO_.create();

    // Reset camera to frame the model
    resetCameraToModel();

    LOG_INFO << "ViewerApp attached successfully" << std::endl;
}

// ============================================================
// onDetach - Cleanup resources
// ============================================================

void ViewerApp::onDetach() {
    static bool detached = false;
    if (detached) return;
    detached = true;

    model_.destroy();
    screenshotPBO_.destroy();
    renderer_.destroy();
    gui_.shutdown();
}

// ============================================================
// onUpdate - Per-frame logic (rendering, animation, downloads)
// ============================================================

void ViewerApp::onUpdate(float deltaTime) {
    // Handle pending random model loads
    if (gui_.checkPendingRandomLoad()) {
        gui_.processRandomLoad();
    }

    // Handle downloaded model
    handleModelDownloads();

    // handle selected model
    handleModelSelected();

    // Update animation
    updateAnimation(deltaTime);

    // Sync ortho projection from GUI to camera
    camera.orthoProjection = gui_.orthoProjection();

    // Populate render context and render
    populateRenderContext();
    renderer_.render(ctx_);

    // Shader hot-reload
    if (gui_.shaderReloadRequested()) {
        gui_.setShaderReloadRequested(false);
        renderer_.reloadShaders();
    }
    // F5 key shortcut for shader reload
    if (glfwGetKey(windowHandle_, GLFW_KEY_F5) == GLFW_PRESS) {
        static double lastReloadTime = 0.0;
        double now = glfwGetTime();
        if (now - lastReloadTime > 0.5) {  // Debounce 0.5s
            lastReloadTime = now;
            renderer_.reloadShaders();
        }
    }

    // Screenshot PBO handling
    if (gui_.screenshotPending()) {
        if (updateScreenshotTexture()) {
            gui_.updateScreenShotImage(screenshotTexture_.id(), screenshotTexture_.width(), screenshotTexture_.height());
            gui_.setScreenshotPending(false);
        }
    }
    if (gui_.triggerScreenshot()) {
        takeScreenshotPBO();
        gui_.setTriggerScreenshot(false);
        gui_.setScreenshotPending(true);
    }
    if (gui_.saveScreenShot())
    {
        std::string_view s = gui_.savedScreenShotName();
        saveScreenshotPBO(s);
        gui_.setSaveScreenShot(false);
    }
}

// ============================================================
// onUpdateUI - Per-frame UI
// ============================================================

void ViewerApp::onUpdateUI(float deltaTime) {
    gui_.draw(deltaTime, model_, camera, windowHandle_, dpiScale,
              [this]() { resetCameraToModel(); });

    // Update window title with FPS
    static float titleTimer = 0.0f;
    static float smoothedFps = 0.0f;
    titleTimer += deltaTime;
    if (titleTimer >= 0.5f) {
        float instFps = (deltaTime > 0.0001f ? 1.0f / deltaTime : 0.0f);
        smoothedFps = (smoothedFps == 0.0f) ? instFps : (smoothedFps * 0.9f + instFps * 0.1f);
        char titleBuf[128];
        snprintf(titleBuf, sizeof(titleBuf), "glTF 2.0 Viewer (%s) | FPS: %.1f", currentAPIStr_.c_str(), smoothedFps);
        glfwSetWindowTitle(windowHandle_, titleBuf);
        titleTimer = 0.0f;
    }
}


void ViewerApp::loadNewModel(const char* path)
{
    if (!model_.loadFromFile(path)) {
        LOG_ERROR << "failed to load file: " << path << std::endl;
    } else {
        modelCenter_ = model_.getCenter();
        modelScale_ = model_.getScale();
        modelSize_ = model_.getModelSize();
        globalModelExtent_ = model_.getExtent();
        globalModelScale_ = model_.getScale();

        animationTimeTicks_ = 0.0f;
        gui_.setAnimationTimeTicks(0.0f);
        gui_.setActiveAnimationIndex(model_.hasAnimation() ? 1 : 0);
        resetCameraToModel();
    }
}

// ============================================================
// Helper methods
// ============================================================

void ViewerApp::handleModelDownloads() {
    std::vector<char> modelData;
    if (gui_.consumeDownloadedModel(modelData)) {
        LOG_INFO << "Loading model from web data (size: " << modelData.size() << " bytes)" << std::endl;

        if (!model_.loadFromMemory(modelData)) {
            LOG_ERROR << "Error: Invalid model file" << std::endl;
        } else {
            modelCenter_ = model_.getCenter();
            modelScale_ = model_.getScale();
            modelSize_ = model_.getModelSize();
            globalModelExtent_ = model_.getExtent();
            globalModelScale_ = model_.getScale();

            animationTimeTicks_ = 0.0f;
            gui_.setAnimationTimeTicks(0.0f);
            gui_.setActiveAnimationIndex(model_.hasAnimation() ? 1 : 0);
            resetCameraToModel();

            LOG_INFO << "Model loaded successfully!" << std::endl;
        }
    }
}

void ViewerApp::handleModelSelected()
{
    if (gui_.isSelectFileModel())
    {
        std::string filePath = gui_.getSelectedModelFile();
        if (!model_.loadFromFile(filePath))
        {
            LOG_ERROR << "Error: Invalid model file:" << filePath << std::endl;
		}
	    else {
		    modelCenter_ = model_.getCenter();
		    modelScale_ = model_.getScale();
		    modelSize_ = model_.getModelSize();
		    globalModelExtent_ = model_.getExtent();
		    globalModelScale_ = model_.getScale();

		    animationTimeTicks_ = 0.0f;
		    gui_.setAnimationTimeTicks(0.0f);
		    gui_.setActiveAnimationIndex(model_.hasAnimation() ? 1 : 0);
		    resetCameraToModel();
	    }
        gui_.clearSelectedFileModelStatus();
    }
}

void ViewerApp::updateAnimation(float deltaTime) {
    if (model_.hasAnimation()) {
        unsigned int animIdx = (gui_.activeAnimationIndex() > 0) ? (gui_.activeAnimationIndex() - 1) : 0;
        model_.setAnimationIndex(animIdx);

        if (gui_.isPlaying() && gui_.activeAnimationIndex() > 0) {
            float ticksPerSecond = model_.getAnimationTicksPerSecond();
            float duration = model_.getAnimationDurationInSeconds(animIdx);
            animationTimeTicks_ += deltaTime * ticksPerSecond * gui_.playbackSpeed();
            if (duration > 0.0f) {
                animationTimeTicks_ = std::fmod(animationTimeTicks_, duration * ticksPerSecond);
            }
        } else if (gui_.activeAnimationIndex() == 0) {
            animationTimeTicks_ = 0.0f;
        }

        // Sync animation time to GUI for the timeline slider
        gui_.setAnimationTimeTicks(animationTimeTicks_);
        model_.updateAnimation(animationTimeTicks_);
    }
}

void ViewerApp::populateRenderContext() {
    ctx_.model = &model_;
    ctx_.camera = &camera;

    // Compute viewport dimensions (excluding sidebar and bottom bar)
    int winWidth, winHeight;
    glfwGetWindowSize(windowHandle_, &winWidth, &winHeight);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(windowHandle_, &fbWidth, &fbHeight);

    float scaleX = (float)fbWidth / (float)winWidth;
    float scaleY = (float)fbHeight / (float)winHeight;

    float vpX = 0.0f;
    float vpY = gui_.bottomBarHeight();
    float vpWidth = (float)winWidth - gui_.sidebarWidth();
    float vpHeight = (float)winHeight - gui_.menuBarHeight() - gui_.bottomBarHeight();

    ctx_.viewportX = (int)(vpX * scaleX);
    ctx_.viewportY = (int)(vpY * scaleY);
    ctx_.viewportWidth = std::max(1, (int)(vpWidth * scaleX));
    ctx_.viewportHeight = std::max(1, (int)(vpHeight * scaleY));
    ctx_.framebufferWidth = fbWidth;
    ctx_.framebufferHeight = fbHeight;

    // Light direction from GUI
    ctx_.lightDir = gui_.getLightDirection();
    ctx_.lightColor = gui_.lightColor();
    ctx_.lightIntensity = gui_.lightIntensity();
    ctx_.ambientIntensity = gui_.ambientIntensity();

    // Model matrix
    ctx_.modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(modelScale_))
                       * glm::translate(glm::mat4(1.0f), -modelCenter_);
    ctx_.modelCenter = modelCenter_;
    ctx_.modelScale = modelScale_;

    // Rendering settings from GUI
    ctx_.wireframeMode = gui_.wireframeMode();
    ctx_.useMipmap = gui_.useMipmap();
    ctx_.orthoProjection = gui_.orthoProjection();
    ctx_.showBoundingBox = gui_.showBoundingBox();
    ctx_.bboxAlpha = gui_.boundingBoxAlpha();

    // Set the main render pass viewport and clear info
    auto* mainPass = renderer_.getPass("Main");
    if (mainPass) {
        ViewportInfo vp;
        vp.x = ctx_.viewportX;
        vp.y = ctx_.viewportY;
        vp.width = ctx_.viewportWidth;
        vp.height = ctx_.viewportHeight;
        mainPass->setViewport(vp);

        ClearInfo clear;
        clear.clearColor = true;
        clear.clearDepth = true;
        clear.clearColorValue[0] = 0.96f;
        clear.clearColorValue[1] = 0.96f;
        clear.clearColorValue[2] = 0.97f;
        clear.clearColorValue[3] = 1.0f;
        clear.clearDepthValue = 1.0f;
        mainPass->setClearInfo(clear);
    }
}

void ViewerApp::resetCameraToModel() {
    float scaledModelSize = glm::length(globalModelExtent_) * globalModelScale_;
    if (scaledModelSize <= 0.0f) scaledModelSize = 2.6f;

    int width, height;
    glfwGetWindowSize(windowHandle_, &width, &height);
    float glVpW = std::max(10.0f, (float)width - gui_.sidebarWidth());
    float glVpH = std::max(10.0f, (float)height - gui_.menuBarHeight() - gui_.bottomBarHeight());
    float aspect = glVpW / glVpH;

    float fovYRad = glm::radians(camera.fovY);
    float halfFov = fovYRad * 0.5f;
    if (aspect < 1.0f) {
        halfFov = std::atan(std::tan(halfFov) * aspect);
    }
    float radius = scaledModelSize * 0.5f;

    camera.target = glm::vec3(0.0f);
    camera.distance = std::max(0.5f, (radius / std::sin(halfFov)) * 1.2f);
    camera.setYawPitch(0.0f, 0.0f);
}

void ViewerApp::takeScreenshotPBO() {
    // Capture only the 3D viewport region (left display area), not the full UI window
    int width = ctx_.viewportWidth;
    int height = ctx_.viewportHeight;
    int x = ctx_.viewportX;
    int y = ctx_.viewportY;
    GLsizeiptr bufferSize = width * height * 4;

    screenshotPBO_.bind(GL_PIXEL_PACK_BUFFER);

    static int lastWidth = 0, lastHeight = 0;
    if (width != lastWidth || height != lastHeight) {
        screenshotPBO_.setData(bufferSize, nullptr, GL_STREAM_READ);
        lastWidth = width;
        lastHeight = height;
    }

    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    PixelBuffer::unbind(GL_PIXEL_PACK_BUFFER);
}

bool ViewerApp::updateScreenshotTexture()
{
	int width = ctx_.viewportWidth;
	int height = ctx_.viewportHeight;

	GLsizeiptr bufferSize = width * height * 4;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, screenshotPBO_.id());
	void* ptr = glMapNamedBufferRange(screenshotPBO_.id(), 0, bufferSize, GL_MAP_READ_BIT);
	if (!ptr) {
		// Data not ready yet; keep pending and retry next frame
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return false;
	}

	// Vertical flip because framebuffer origin is bottom-left
	std::vector<unsigned char> flipped(bufferSize);
	unsigned char* src = (unsigned char*)ptr;
	int rowSize = width * 4;
	for (int y = 0; y < height; ++y) {
		memcpy(&flipped[y * rowSize], &src[(height - 1 - y) * rowSize], rowSize);
	}
	glUnmapNamedBuffer(screenshotPBO_.id());
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// Allocate or resize the texture to match the captured size
	if (screenshotTexture_.id() == 0) {
		screenshotTexture_.storage2D(width, height, GL_RGBA8, 1);
	}
	else if (screenshotTexture_.width() != width || screenshotTexture_.height() != height) {
		screenshotTexture_.storage2D(width, height, GL_RGBA8, 1);
	}
	screenshotTexture_.subImage(0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
	screenshotTexture_.parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	screenshotTexture_.parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return true;
}

void ViewerApp::saveScreenshotPBO(const std::string_view& name) {
    int width = ctx_.viewportWidth;
    int height = ctx_.viewportHeight;
    GLsizeiptr bufferSize = width * height * 4;

    void* ptr = screenshotPBO_.mapRange(0, bufferSize, GL_MAP_READ_BIT);
    if (ptr) {
        // Ensure the output file has a .png extension
        std::string filename(name);
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".png")
            filename += ".png";

        std::vector<unsigned char> flippedPixels(bufferSize);
        unsigned char* src = (unsigned char*)ptr;
        int rowSize = width * 4;
        for (int y = 0; y < height; ++y) {
            memcpy(&flippedPixels[y * rowSize], &src[(height - 1 - y) * rowSize], rowSize);
        }
        stbi_write_png(filename.c_str(), width, height, 4, flippedPixels.data(), rowSize);
        screenshotPBO_.unmap();
        LOG_INFO << "Screenshot saved to " << filename << std::endl;
    } else {
        LOG_ERROR << "Failed to map screenshot PBO" << std::endl;
    }
}
