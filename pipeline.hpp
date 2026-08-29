#pragma once

#include <GL/glew.h>
#include "shaderprogram.h"
#include <memory>

struct BlendState {
    bool enabled = true;
    GLenum srcRGB = GL_SRC_ALPHA;
    GLenum dstRGB = GL_ONE_MINUS_SRC_ALPHA;
    GLenum srcAlpha = GL_ONE;
    GLenum dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    GLenum equationRGB = GL_FUNC_ADD;
    GLenum equationAlpha = GL_FUNC_ADD;
};

struct DepthState {
    bool testEnabled = true;
    bool writeEnabled = true;
    GLenum func = GL_LESS;
};

struct RasterizerState {
    GLenum polygonMode = GL_FILL;
    GLenum cullMode = GL_NONE;     // GL_NONE = no culling, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK
    GLenum frontFace = GL_CCW;
    float lineWidth = 1.0f;
};

struct MultisampleState {
    bool enabled = true;
};

class GraphicsPipeline {
public:
    GraphicsPipeline() = default;

    GraphicsPipeline(std::shared_ptr<ShaderProgram> shader,
                     BlendState blend = {},
                     DepthState depth = {},
                     RasterizerState rasterizer = {},
                     MultisampleState ms = {})
        : shader_(std::move(shader)), blend_(blend), depth_(depth),
          rasterizer_(rasterizer), multisample_(ms) {}

    void bind() {
        // Shader
        if (shader_) {
            shader_->use();
        }

        // Blend state
        if (blend_.enabled) {
            glEnable(GL_BLEND);
            if (blend_.srcRGB == blend_.srcAlpha && blend_.dstRGB == blend_.dstAlpha) {
                glBlendFunc(blend_.srcRGB, blend_.dstRGB);
            } else {
                glBlendFuncSeparate(blend_.srcRGB, blend_.dstRGB,
                                    blend_.srcAlpha, blend_.dstAlpha);
            }
            if (blend_.equationRGB != GL_FUNC_ADD || blend_.equationAlpha != GL_FUNC_ADD) {
                glBlendEquationSeparate(blend_.equationRGB, blend_.equationAlpha);
            }
        } else {
            glDisable(GL_BLEND);
        }

        // Depth state
        if (depth_.testEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(depth_.writeEnabled ? GL_TRUE : GL_FALSE);
        glDepthFunc(depth_.func);

        // Rasterizer state
        if (rasterizer_.polygonMode != GL_FILL) {
            glPolygonMode(GL_FRONT_AND_BACK, rasterizer_.polygonMode);
        }
        if (rasterizer_.cullMode != GL_NONE) {
            glEnable(GL_CULL_FACE);
            glCullFace(rasterizer_.cullMode);
        } else {
            glDisable(GL_CULL_FACE);
        }
        glFrontFace(rasterizer_.frontFace);
        if (rasterizer_.lineWidth != 1.0f) {
            glLineWidth(rasterizer_.lineWidth);
        }

        // Multisample state
        if (multisample_.enabled) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
    }

    void unbind() {
        if (shader_) {
            shader_->unuse();
        }
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glEnable(GL_MULTISAMPLE);
    }

    ShaderProgram* shader() const { return shader_.get(); }
    std::shared_ptr<ShaderProgram> shaderShared() const { return shader_; }

    BlendState& blendState() { return blend_; }
    DepthState& depthState() { return depth_; }
    RasterizerState& rasterizerState() { return rasterizer_; }
    MultisampleState& multisampleState() { return multisample_; }

    const BlendState& blendState() const { return blend_; }
    const DepthState& depthState() const { return depth_; }
    const RasterizerState& rasterizerState() const { return rasterizer_; }
    const MultisampleState& multisampleState() const { return multisample_; }

    void setShader(std::shared_ptr<ShaderProgram> shader) { shader_ = std::move(shader); }
    void setBlendState(const BlendState& blend) { blend_ = blend; }
    void setDepthState(const DepthState& depth) { depth_ = depth; }
    void setRasterizerState(const RasterizerState& rasterizer) { rasterizer_ = rasterizer; }
    void setMultisampleState(const MultisampleState& ms) { multisample_ = ms; }

private:
    std::shared_ptr<ShaderProgram> shader_;
    BlendState blend_;
    DepthState depth_;
    RasterizerState rasterizer_;
    MultisampleState multisample_;
};
