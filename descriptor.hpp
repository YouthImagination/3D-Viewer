#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <utility>

class DescriptorSet {
public:
    struct BufferBinding {
        GLenum target;          // GL_UNIFORM_BUFFER or GL_SHADER_STORAGE_BUFFER
        GLuint bindingIndex;
        GLuint bufferID;
    };

    struct TextureBinding {
        GLuint unit;
        GLuint textureID;
    };

    DescriptorSet() = default;

    // Buffer bindings
    void bindBufferBase(GLenum target, GLuint binding, GLuint bufferID) {
        bufferBindings_.push_back({target, binding, bufferID});
    }

    // Texture bindings
    void bindTexture(GLuint unit, GLuint textureID) {
        textureBindings_.push_back({unit, textureID});
    }

    // Uniform setters (location, value)
    void setUniformInt(GLint loc, int value) {
        uniformInts_.push_back({loc, value});
    }

    void setUniformFloat(GLint loc, float value) {
        uniformFloats_.push_back({loc, value});
    }

    void setUniformVec2(GLint loc, const glm::vec2& value) {
        uniformVec2s_.push_back({loc, value});
    }

    void setUniformVec3(GLint loc, const glm::vec3& value) {
        uniformVec3s_.push_back({loc, value});
    }

    void setUniformVec4(GLint loc, const glm::vec4& value) {
        uniformVec4s_.push_back({loc, value});
    }

    void setUniformMat4(GLint loc, const glm::mat4& value) {
        uniformMat4s_.push_back({loc, value});
    }

    // Issue all stored bindings
    void bind() const {
        for (const auto& b : bufferBindings_) {
            glBindBufferBase(b.target, b.bindingIndex, b.bufferID);
        }
        for (const auto& t : textureBindings_) {
            glBindTextureUnit(t.unit, t.textureID);
        }
        for (const auto& u : uniformInts_) {
            glUniform1i(u.first, u.second);
        }
        for (const auto& u : uniformFloats_) {
            glUniform1f(u.first, u.second);
        }
        for (const auto& u : uniformVec2s_) {
            glUniform2fv(u.first, 1, glm::value_ptr(u.second));
        }
        for (const auto& u : uniformVec3s_) {
            glUniform3fv(u.first, 1, glm::value_ptr(u.second));
        }
        for (const auto& u : uniformVec4s_) {
            glUniform4fv(u.first, 1, glm::value_ptr(u.second));
        }
        for (const auto& u : uniformMat4s_) {
            glUniformMatrix4fv(u.first, 1, GL_FALSE, glm::value_ptr(u.second));
        }
    }

    // Reset all bindings
    void clear() {
        bufferBindings_.clear();
        textureBindings_.clear();
        uniformInts_.clear();
        uniformFloats_.clear();
        uniformVec2s_.clear();
        uniformVec3s_.clear();
        uniformVec4s_.clear();
        uniformMat4s_.clear();
    }

    // Reserve hints for performance
    void reserve(size_t buffers, size_t textures, size_t uniforms) {
        bufferBindings_.reserve(buffers);
        textureBindings_.reserve(textures);
        uniformInts_.reserve(uniforms);
    }

private:
    std::vector<BufferBinding> bufferBindings_;
    std::vector<TextureBinding> textureBindings_;
    std::vector<std::pair<GLint, int>> uniformInts_;
    std::vector<std::pair<GLint, float>> uniformFloats_;
    std::vector<std::pair<GLint, glm::vec2>> uniformVec2s_;
    std::vector<std::pair<GLint, glm::vec3>> uniformVec3s_;
    std::vector<std::pair<GLint, glm::vec4>> uniformVec4s_;
    std::vector<std::pair<GLint, glm::mat4>> uniformMat4s_;
};
