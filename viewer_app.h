#pragma once

#include "application.h"
#include "renderer.h"
#include "gui.h"
#include "model.h"
#include "render_pass.h"
#include "buffer.hpp"

#include <GL/glew.h>
#include <stb_image_write.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ViewerApp : public Application {
public:
    ViewerApp(uint32_t w = 1920, uint32_t h = 1080);
    ~ViewerApp() override = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float deltaTime) override;
    void onUpdateUI(float deltaTime) override;
    virtual void loadNewModel(const char* path) override;

private:
    void handleModelDownloads();
    void handleModelSelected();
    void updateAnimation(float deltaTime);
    void populateRenderContext();
    void resetCameraToModel();

    void takeScreenshotPBO();
    bool updateScreenshotTexture();
    void saveScreenshotPBO(const std::string_view& name);

    Renderer renderer_;
    GUI gui_;
    Model model_;
    RenderContext ctx_;

    Texture2D screenshotTexture_;
    PixelBuffer screenshotPBO_;
    float animationTimeTicks_ = 0.0f;

    glm::vec3 modelCenter_ = glm::vec3(0.0f);
    float modelScale_ = 1.0f;
    float modelSize_ = 3.0f;
    glm::vec3 globalModelExtent_ = glm::vec3(0.0f);
    float globalModelScale_ = 1.0f;

    std::string currentAPIStr_ = "gl";
};
