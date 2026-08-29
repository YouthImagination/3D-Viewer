#pragma once

#include <GL/glew.h>
#include <utility>

class Framebuffer {
public:
    Framebuffer() : id_(0), width_(0), height_(0) {}

    explicit Framebuffer(int w, int h) : width_(w), height_(h) {
        glCreateFramebuffers(1, &id_);
    }

    ~Framebuffer() {
        destroy();
    }

    // Delete copy semantics
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Move semantics
    Framebuffer(Framebuffer&& other) noexcept
        : id_(other.id_), width_(other.width_), height_(other.height_) {
        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
    }

    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            destroy();
            id_ = other.id_;
            width_ = other.width_;
            height_ = other.height_;
            other.id_ = 0;
            other.width_ = 0;
            other.height_ = 0;
        }
        return *this;
    }

    GLuint id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }

    void create(int w, int h) {
        if (id_ == 0) {
            glCreateFramebuffers(1, &id_);
        }
        width_ = w;
        height_ = h;
    }

    void attachColor(GLuint textureID, int colorIndex = 0, int level = 0) {
        if (id_ != 0) {
            glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT0 + colorIndex, textureID, level);
        }
    }

    void attachDepth(GLuint textureID, int level = 0) {
        if (id_ != 0) {
            glNamedFramebufferTexture(id_, GL_DEPTH_ATTACHMENT, textureID, level);
        }
    }

    void attachStencilDepth(GLuint textureID, int level = 0) {
        if (id_ != 0) {
            glNamedFramebufferTexture(id_, GL_DEPTH_STENCIL_ATTACHMENT, textureID, level);
        }
    }

    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
    }

    void unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static void bindDefault() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool checkStatus() const {
        if (id_ == 0) return false;
        GLenum status = glCheckNamedFramebufferStatus(id_, GL_FRAMEBUFFER);
        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    GLenum getStatus() const {
        if (id_ == 0) return 0;
        return glCheckNamedFramebufferStatus(id_, GL_FRAMEBUFFER);
    }

    void clearColor(int index, float r, float g, float b, float a) {
        if (id_ != 0) {
            GLfloat color[4] = {r, g, b, a};
            glClearNamedFramebufferfv(id_, GL_COLOR, index, color);
        }
    }

    void clearColor(float r, float g, float b, float a) {
        clearColor(0, r, g, b, a);
    }

    void clearDepth(float value) {
        if (id_ != 0) {
            GLfloat depth = value;
            glClearNamedFramebufferfv(id_, GL_DEPTH, 0, &depth);
        }
    }

    void clearStencil(int value) {
        if (id_ != 0) {
            GLint stencil = value;
            glClearNamedFramebufferiv(id_, GL_STENCIL, 0, &stencil);
        }
    }

    void setViewport(int x, int y, int w, int h) const {
        glViewport(x, y, w, h);
    }

    void setViewport() const {
        glViewport(0, 0, width_, height_);
    }

    void destroy() {
        if (id_ != 0) {
            glDeleteFramebuffers(1, &id_);
            id_ = 0;
            width_ = 0;
            height_ = 0;
        }
    }

private:
    GLuint id_;
    int width_;
    int height_;
};
