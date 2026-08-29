#pragma once

#include "render_pass.h"
#include "pipeline.hpp"
#include "descriptor.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include "buffer.hpp"
#include "shaderprogram.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

struct MatricesBlock {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraPos;
    float pad;
};

class MainRenderPass : public RenderPass {
public:
    MainRenderPass();
    ~MainRenderPass() override;

    void setup() override;
    void execute(RenderContext& ctx) override;
    void resize(int w, int h) override;
    void destroy() override;

    bool reloadShaders();  // Hot-reload shaders from disk, returns true on success

private:
    std::shared_ptr<ShaderProgram> compileShaders(const char* vertPath, const char* fragPath);  // Compile shaders (GLSL or SPIR-V)

    GraphicsPipeline pipeline_;
    DescriptorSet descriptorSet_;
    UniformBuffer ubo_;
    std::shared_ptr<ShaderProgram> shader_;
    std::shared_ptr<ShaderProgram> bboxShader_;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    void init(int width, int height);
    void render(RenderContext& ctx);
    void resize(int width, int height);
    void destroy();

    void addPass(std::unique_ptr<RenderPass> pass);
    RenderPass* getPass(const std::string& name) const;

    bool reloadShaders();  // Hot-reload all shader passes

    Framebuffer& createFramebuffer(const std::string& name, int w, int h);
    Framebuffer* getFramebuffer(const std::string& name);

private:
    std::vector<std::unique_ptr<RenderPass>> passes_;
    std::unordered_map<std::string, Framebuffer> framebuffers_;
    int screenWidth_ = 0;
    int screenHeight_ = 0;
};
