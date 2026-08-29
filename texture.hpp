#pragma once

#include <GL/glew.h>
#include <utility>

class Texture2D {
public:
    Texture2D() : id_(0), width_(0), height_(0), levels_(0), internalFormat_(GL_RGBA8) {}

    Texture2D(int w, int h, GLenum internalFormat, int levels = 1)
        : width_(w), height_(h), levels_(levels), internalFormat_(internalFormat) {
        glCreateTextures(GL_TEXTURE_2D, 1, &id_);
        glTextureStorage2D(id_, levels_, internalFormat_, w, h);
    }

    ~Texture2D() {
        destroy();
    }

    // Delete copy semantics
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Move semantics
    Texture2D(Texture2D&& other) noexcept
        : id_(other.id_), width_(other.width_), height_(other.height_),
          levels_(other.levels_), internalFormat_(other.internalFormat_) {
        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.levels_ = 0;
    }

    Texture2D& operator=(Texture2D&& other) noexcept {
        if (this != &other) {
            destroy();
            id_ = other.id_;
            width_ = other.width_;
            height_ = other.height_;
            levels_ = other.levels_;
            internalFormat_ = other.internalFormat_;
            other.id_ = 0;
            other.width_ = 0;
            other.height_ = 0;
            other.levels_ = 0;
        }
        return *this;
    }

    GLuint id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int levels() const { return levels_; }
    GLenum internalFormat() const { return internalFormat_; }

    void storage2D(int w, int h, GLenum internalFormat, int levels) {
        if (id_ == 0) {
            glCreateTextures(GL_TEXTURE_2D, 1, &id_);
        }
        width_ = w;
        height_ = h;
        levels_ = levels;
        internalFormat_ = internalFormat;
        glTextureStorage2D(id_, levels_, internalFormat_, w, h);
    }

    void subImage(int level, int x, int y, int w, int h,
                  GLenum format, GLenum type, const void* data) {
        if (id_ != 0) {
            glTextureSubImage2D(id_, level, x, y, w, h, format, type, data);
        }
    }

    void subImage(int level, GLenum format, GLenum type, const void* data) {
        subImage(level, 0, 0, width_, height_, format, type, data);
    }

    void parameteri(GLenum pname, GLint param) {
        if (id_ != 0) {
            glTextureParameteri(id_, pname, param);
        }
    }

    void parameterf(GLenum pname, GLfloat param) {
        if (id_ != 0) {
            glTextureParameterf(id_, pname, param);
        }
    }

    void bind(GLuint unit) const {
        if (id_ != 0) {
            glBindTextureUnit(unit, id_);
        }
    }

    static void unbind(GLuint unit) {
        glBindTextureUnit(unit, 0);
    }

    void generateMipmaps() {
        if (id_ != 0) {
            glGenerateTextureMipmap(id_);
        }
    }

    void destroy() {
        if (id_ != 0) {
            glDeleteTextures(1, &id_);
            id_ = 0;
            width_ = 0;
            height_ = 0;
            levels_ = 0;
        }
    }

private:
    GLuint id_;
    int width_;
    int height_;
    int levels_;
    GLenum internalFormat_;
};
