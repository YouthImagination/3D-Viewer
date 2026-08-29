#pragma once

#include <GL/glew.h>
#include <utility>
#include <algorithm>

// Base class for RAII encapsulation of OpenGL Buffers
class Buffer {
public:
    Buffer() : id_(0), size_(0) {}
    
    Buffer(GLsizeiptr size, const void* data, GLbitfield flags) : size_(size) {
        glCreateBuffers(1, &id_);
        glNamedBufferStorage(id_, size, data, flags);
    }
    
    virtual ~Buffer() {
        destroy();
    }
    
    // Delete copy semantics to prevent double free of OpenGL resource
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    // Support move semantics
    Buffer(Buffer&& other) noexcept : id_(other.id_), size_(other.size_) {
        other.id_ = 0;
        other.size_ = 0;
    }
    
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            destroy();
            id_ = other.id_;
            size_ = other.size_;
            other.id_ = 0;
            other.size_ = 0;
        }
        return *this;
    }
    
    GLuint id() const { return id_; }
    GLsizeiptr size() const { return size_; }
    
    virtual void destroy() {
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
            id_ = 0;
            size_ = 0;
        }
    }

protected:
    GLuint id_{0};
    GLsizeiptr size_{0};
};

// Vertex Buffer Object (VBO) wrapper
class VertexBuffer : public Buffer {
public:
    VertexBuffer() = default;
    VertexBuffer(GLsizeiptr size, const void* data, GLbitfield flags = 0)
        : Buffer(size, data, flags) {}
};

// Index Buffer Object (IBO) wrapper
class IndexBuffer : public Buffer {
public:
    IndexBuffer() = default;
    IndexBuffer(GLsizeiptr size, const void* data, GLbitfield flags = 0)
        : Buffer(size, data, flags) {}
};

// Uniform Buffer Object (UBO) wrapper
class UniformBuffer : public Buffer {
public:
    UniformBuffer() = default;
    UniformBuffer(GLsizeiptr size, const void* data = nullptr, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT)
        : Buffer(size, data, flags) {}
        
    void update(const void* data, GLsizeiptr size, GLintptr offset = 0) {
        if (id_ != 0) {
            glNamedBufferSubData(id_, offset, size, data);
        }
    }
    
    template<typename T>
    void update(const T& data) {
        update(&data, sizeof(T), 0);
    }
    
    void bindBase(GLuint index) const {
        if (id_ != 0) {
            glBindBufferBase(GL_UNIFORM_BUFFER, index, id_);
        }
    }
};

// Shader Storage Buffer Object (SSBO) wrapper
class StorageBuffer : public Buffer {
public:
    StorageBuffer() : Buffer(), mappedPtr_(nullptr) {}
    
    StorageBuffer(GLsizeiptr size, const void* data = nullptr, GLbitfield flags = 0)
        : Buffer(size, data, flags), mappedPtr_(nullptr) {}
        
    ~StorageBuffer() override {
        unmap();
    }
    
    StorageBuffer(StorageBuffer&& other) noexcept 
        : Buffer(std::move(other)), mappedPtr_(other.mappedPtr_) {
        other.mappedPtr_ = nullptr;
    }
    
    StorageBuffer& operator=(StorageBuffer&& other) noexcept {
        if (this != &other) {
            unmap();
            Buffer::operator=(std::move(other));
            mappedPtr_ = other.mappedPtr_;
            other.mappedPtr_ = nullptr;
        }
        return *this;
    }
    
    void* mapRange(GLintptr offset, GLsizeiptr length, GLbitfield accessFlags) {
        if (!mappedPtr_ && id_ != 0) {
            mappedPtr_ = glMapNamedBufferRange(id_, offset, length, accessFlags);
        }
        return mappedPtr_;
    }
    
    void unmap() {
        if (mappedPtr_ && id_ != 0) {
            glUnmapNamedBuffer(id_);
            mappedPtr_ = nullptr;
        }
    }
    
    void* mappedPtr() const { return mappedPtr_; }
    
    void destroy() override {
        unmap();
        Buffer::destroy();
    }
    
    void bindBase(GLuint index) const {
        if (id_ != 0) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id_);
        }
    }
    
    void update(const void* data, GLsizeiptr size, GLintptr offset = 0) {
        if (id_ != 0) {
            glNamedBufferSubData(id_, offset, size, data);
        }
    }
    
private:
    void* mappedPtr_{nullptr};
};

// Vertex Array Object (VAO) wrapper
class VertexArray {
public:
    VertexArray() {
        glCreateVertexArrays(1, &id_);
    }
    
    ~VertexArray() {
        destroy();
    }
    
    // Delete copy semantics
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    
    // Support move semantics
    VertexArray(VertexArray&& other) noexcept : id_(other.id_) {
        other.id_ = 0;
    }
    
    VertexArray& operator=(VertexArray&& other) noexcept {
        if (this != &other) {
            destroy();
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    GLuint id() const { return id_; }
    
    void bind() const {
        if (id_ != 0) {
            glBindVertexArray(id_);
        }
    }
    
    static void unbind() {
        glBindVertexArray(0);
    }
    
    void destroy() {
        if (id_ != 0) {
            glDeleteVertexArrays(1, &id_);
            id_ = 0;
        }
    }
    
    void setVertexBuffer(GLuint bindingIndex, const VertexBuffer& vbo, GLintptr offset, GLsizei stride) {
        if (id_ != 0) {
            glVertexArrayVertexBuffer(id_, bindingIndex, vbo.id(), offset, stride);
        }
    }
    
    void setIndexBuffer(const IndexBuffer& ibo) {
        if (id_ != 0) {
            glVertexArrayElementBuffer(id_, ibo.id());
        }
    }
    
    void enableAttrib(GLuint attribIndex) {
        if (id_ != 0) {
            glEnableVertexArrayAttrib(id_, attribIndex);
        }
    }
    
    void attribFormat(GLuint attribIndex, GLint size, GLenum type, GLboolean normalized, GLuint relativeOffset) {
        if (id_ != 0) {
            glVertexArrayAttribFormat(id_, attribIndex, size, type, normalized, relativeOffset);
        }
    }
    
    void attribIFormat(GLuint attribIndex, GLint size, GLenum type, GLuint relativeOffset) {
        if (id_ != 0) {
            glVertexArrayAttribIFormat(id_, attribIndex, size, type, relativeOffset);
        }
    }
    
    void attribBinding(GLuint attribIndex, GLuint bindingIndex) {
        if (id_ != 0) {
            glVertexArrayAttribBinding(id_, attribIndex, bindingIndex);
        }
    }
    
private:
    GLuint id_{0};
};

// Pixel Buffer Object (PBO) wrapper for pixel transfers (read/write)
class PixelBuffer {
public:
    PixelBuffer() : id_(0), size_(0), mappedPtr_(nullptr) {}
    
    ~PixelBuffer() {
        destroy();
    }
    
    // Delete copy semantics
    PixelBuffer(const PixelBuffer&) = delete;
    PixelBuffer& operator=(const PixelBuffer&) = delete;
    
    // Support move semantics
    PixelBuffer(PixelBuffer&& other) noexcept 
        : id_(other.id_), size_(other.size_), mappedPtr_(other.mappedPtr_) {
        other.id_ = 0;
        other.size_ = 0;
        other.mappedPtr_ = nullptr;
    }
    
    PixelBuffer& operator=(PixelBuffer&& other) noexcept {
        if (this != &other) {
            destroy();
            id_ = other.id_;
            size_ = other.size_;
            mappedPtr_ = other.mappedPtr_;
            other.id_ = 0;
            other.size_ = 0;
            other.mappedPtr_ = nullptr;
        }
        return *this;
    }
    
    void create() {
        if (id_ == 0) {
            glCreateBuffers(1, &id_);
        }
    }
    
    GLuint id() const { return id_; }
    GLsizeiptr size() const { return size_; }
    
    void bind(GLenum target) {
        create();
        glBindBuffer(target, id_);
    }
    
    static void unbind(GLenum target) {
        glBindBuffer(target, 0);
    }
    
    void setData(GLsizeiptr size, const void* data, GLenum usage) {
        create();
        glNamedBufferData(id_, size, data, usage);
        size_ = size;
    }
    
    void* mapRange(GLintptr offset, GLsizeiptr length, GLbitfield accessFlags) {
        create();
        if (!mappedPtr_ && id_ != 0) {
            mappedPtr_ = glMapNamedBufferRange(id_, offset, length, accessFlags);
        }
        return mappedPtr_;
    }
    
    void unmap() {
        if (mappedPtr_ && id_ != 0) {
            glUnmapNamedBuffer(id_);
            mappedPtr_ = nullptr;
        }
    }
    
    void destroy() {
        unmap();
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
            id_ = 0;
            size_ = 0;
        }
    }
    
private:
    GLuint id_;
    GLsizeiptr size_;
    void* mappedPtr_;
};