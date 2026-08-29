#pragma once

#include "model.h"
#include "camera.h"
#include "framebuffer.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

struct ClearInfo {
    bool clearColor = true;
    bool clearDepth = true;
    float clearColorValue[4] = {0.96f, 0.96f, 0.97f, 1.0f};
    float clearDepthValue = 1.0f;
};

struct ViewportInfo {
    int x = 0;
    int y = 0;
    int width = 1920;
    int height = 1080;
};

struct RenderContext {
    Model* model = nullptr;
    ArcballCamera* camera = nullptr;
    float deltaTime = 0.0f;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 1920;
    int viewportHeight = 1080;
    int framebufferWidth = 1920;
    int framebufferHeight = 1080;

    glm::vec3 lightDir = glm::vec3(0.5f, 0.7f, 0.3f);
    glm::vec3 lightColor = glm::vec3(1.0f);
    float lightIntensity = 1.2f;
    float ambientIntensity = 0.20f;

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::vec3 modelCenter = glm::vec3(0.0f);
    float modelScale = 1.0f;

    bool wireframeMode = false;
    bool useMipmap = true;
    bool orthoProjection = false;
    bool showBoundingBox = false;
    float bboxAlpha = 1.0f;
};

class RenderPass {
public:
    RenderPass(const std::string& name) : name_(name) {}
    virtual ~RenderPass() = default;

    virtual void setup() = 0;
    virtual void execute(RenderContext& ctx) = 0;
    virtual void resize(int w, int h) = 0;
    virtual void destroy() = 0;

    const std::string& name() const { return name_; }
    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    void setTargetFramebuffer(Framebuffer* fbo) { targetFBO_ = fbo; }
    Framebuffer* targetFramebuffer() const { return targetFBO_; }

    void setClearInfo(const ClearInfo& info) { clearInfo_ = info; }
    const ClearInfo& clearInfo() const { return clearInfo_; }

    void setViewport(const ViewportInfo& vp) { viewport_ = vp; }
    const ViewportInfo& viewport() const { return viewport_; }

protected:
    void beginPass() {
        if (targetFBO_) {
            targetFBO_->bind();
        } else {
            Framebuffer::bindDefault();
        }

        glViewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height);

        if (clearInfo_.clearColor || clearInfo_.clearDepth) {
            if (targetFBO_) {
                if (clearInfo_.clearColor) {
                    targetFBO_->clearColor(clearInfo_.clearColorValue[0],
                                           clearInfo_.clearColorValue[1],
                                           clearInfo_.clearColorValue[2],
                                           clearInfo_.clearColorValue[3]);
                }
                if (clearInfo_.clearDepth) {
                    targetFBO_->clearDepth(clearInfo_.clearDepthValue);
                }
            } else {
                if (clearInfo_.clearColor) {
                    glClearColor(clearInfo_.clearColorValue[0],
                                 clearInfo_.clearColorValue[1],
                                 clearInfo_.clearColorValue[2],
                                 clearInfo_.clearColorValue[3]);
                    glClear(GL_COLOR_BUFFER_BIT);
                }
                if (clearInfo_.clearDepth) {
                    glClearDepth(clearInfo_.clearDepthValue);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
            }
        }
    }

    void endPass() {
        // Default: restore to screen framebuffer (implicit in double-buffered swap)
        // Caller can bind another FBO or let the frame end
    }

    std::string name_;
    bool enabled_ = true;
    Framebuffer* targetFBO_ = nullptr;
    ClearInfo clearInfo_;
    ViewportInfo viewport_;
};
