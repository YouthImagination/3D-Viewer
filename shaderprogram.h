#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "logger.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace shaderUtils
{
    inline std::vector<char> readBinaryFile(const std::string& filename) {
        std::vector<std::string> searchPaths = {
            filename, "../" + filename, "../../" + filename,
            "build/" + filename, "../build/" + filename
        };
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                std::ifstream file(path, std::ios::ate | std::ios::binary);
                if (file.is_open()) {
                    size_t fileSize = (size_t)file.tellg();
                    std::vector<char> buffer(fileSize);
                    file.seekg(0);
                    file.read(buffer.data(), fileSize);
                    file.close();
                    LOG_INFO << "Successfully read SPIR-V shader: " << path << std::endl;
                    return buffer;
                }
            }
        }
        throw std::runtime_error("failed to open SPIR-V file: " + filename);
    }

    inline std::string readSourceFile(const std::string& filename) {
        std::vector<std::string> searchPaths = {
            filename, "../" + filename, "../../" + filename,
            "build/" + filename, "../build/" + filename
        };
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                std::ifstream file(path);
                if (file.is_open()) {
                    std::string source((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    file.close();
                    LOG_INFO << "Successfully read GLSL source: " << path << std::endl;
                    return source;
                }
            }
        }
        return "";  // Return empty string if not found (not an error for GLSL fallback)
    }

    inline bool fileExists(const std::string& filename) {
        std::vector<std::string> searchPaths = {
            filename, "../" + filename, "../../" + filename,
            "build/" + filename, "../build/" + filename
        };
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) return true;
        }
        return false;
    }

    // Compile a single GLSL shader from source file
    inline GLuint compileGLSLShader(GLenum shaderType, const std::string& filename) {
        std::string source = readSourceFile(filename);
        if (source.empty()) return 0;
        const char* src = source.c_str();
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            LOG_ERROR << "GLSL Shader (" << filename << ") compilation failed:\n" << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    // Link a shader program from two compiled shader handles
    inline GLuint linkProgram(GLuint vs, GLuint fs) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        GLint linkSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            char log[1024];
            glGetProgramInfoLog(program, 1024, nullptr, log);
            LOG_ERROR << "Shader Program linking failed:\n" << log << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }
}

class ShaderProgram
{
public:
    ShaderProgram(const std::string& vertShader, const std::string& fragShader)
    {
        GLuint vs = loadSPIRVShader(GL_VERTEX_SHADER, vertShader.c_str());
        GLuint fs = loadSPIRVShader(GL_FRAGMENT_SHADER, fragShader.c_str());
        program_ = glCreateProgram();
        glAttachShader(program_, vs);
        glAttachShader(program_, fs);
        glLinkProgram(program_);
        GLint linkSuccess;
        glGetProgramiv(program_, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            char log[1024];
            glGetProgramInfoLog(program_, 1024, nullptr, log);
            LOG_ERROR << "Shader Program linking failed:\n" << log << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // Construct from an existing OpenGL program handle (takes ownership)
    explicit ShaderProgram(GLuint program) : program_(program) {}

    ~ShaderProgram() { glDeleteProgram(program_); program_ = 0; }

    void use()   { glUseProgram(program_); }
    void unuse() { glUseProgram(0); }

    int getUniformLocation(const std::string_view& name) {
        return glGetUniformLocation(program_, name.data());
    }

    void bindUniformBlock(const std::string_view& name, int binding) {
        GLuint blockIndex = glGetUniformBlockIndex(program_, name.data());
        glUniformBlockBinding(program_, blockIndex, binding);
    }

    void setUniform(int loc, int value) { glUniform1i(loc, value); }
    void setUniform(int loc, float x) { glUniform1f(loc, x); }
    void setUniform(int loc, const glm::vec2& v) { glUniform2fv(loc, 1, glm::value_ptr(v)); }
    void setUniform(int loc, float x, float y) { glUniform2f(loc, x, y); }
    void setUniform(int loc, const glm::vec3& v) { glUniform3fv(loc, 1, glm::value_ptr(v)); }
    void setUniform(int loc, float x, float y, float z) { glUniform3f(loc, x, y, z); }
    void setUniform(int loc, const glm::vec4& v) { glUniform4fv(loc, 1, glm::value_ptr(v)); }
    void setUniform(int loc, float x, float y, float z, float w) { glUniform4f(loc, x, y, z, w); }
    void setUniform(int loc, const glm::mat4& v, int count = 1, bool transpose = false) {
        glUniformMatrix4fv(loc, count, transpose, glm::value_ptr(v));
    }

private:
    GLuint loadSPIRVShader(GLenum shaderType, const std::string& filename) {
        std::vector<char> spirvCode = shaderUtils::readBinaryFile(filename);
        GLuint shader = glCreateShader(shaderType);
        glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, spirvCode.data(), (GLsizei)spirvCode.size());
        glSpecializeShader(shader, "main", 0, nullptr, nullptr);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            LOG_ERROR << "SPIR-V Shader (" << filename << ") specialization failed:\n" << infoLog << std::endl;
        }
        return shader;
    }

    GLuint program_{0};
};
