#include "renderer.h"
#include "logger.h"
#include <algorithm>

// --- MainRenderPass ---

MainRenderPass::MainRenderPass()
    : RenderPass("Main") {}

MainRenderPass::~MainRenderPass() {
    destroy();
}

void MainRenderPass::setup() {
    shader_ = compileShaders("shader.vert", "shader.frag");
    shader_->bindUniformBlock("MatricesBlock", 1);

    bboxShader_ = compileShaders("bbox.vert", "bbox.frag");
    bboxShader_->bindUniformBlock("MatricesBlock", 1);

    BlendState blend{};
    blend.enabled = true;
    blend.srcRGB = GL_SRC_ALPHA;
    blend.dstRGB = GL_ONE_MINUS_SRC_ALPHA;
    blend.srcAlpha = GL_ONE;
    blend.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;

    DepthState depth{};
    depth.testEnabled = true;
    depth.writeEnabled = true;
    depth.func = GL_LESS;

    RasterizerState rasterizer{};
    rasterizer.polygonMode = GL_FILL;
    rasterizer.cullMode = GL_NONE;

    MultisampleState ms{};
    ms.enabled = true;

    pipeline_ = GraphicsPipeline(shader_, blend, depth, rasterizer, ms);
    ubo_ = UniformBuffer(sizeof(MatricesBlock));
    LOG_INFO << "MainRenderPass setup complete" << std::endl;
}

std::shared_ptr<ShaderProgram> MainRenderPass::compileShaders(const char* vertPath, const char* fragPath) {
    // Try GLSL source compilation first, fall back to SPIR-V binary
    if (shaderUtils::fileExists(vertPath) && shaderUtils::fileExists(fragPath)) {
        GLuint vs = shaderUtils::compileGLSLShader(GL_VERTEX_SHADER, vertPath);
        GLuint fs = shaderUtils::compileGLSLShader(GL_FRAGMENT_SHADER, fragPath);
        if (vs != 0 && fs != 0) {
            GLuint program = shaderUtils::linkProgram(vs, fs);
            LOG_INFO << "MainRenderPass: Compiled from GLSL source" << std::endl;
            return std::make_shared<ShaderProgram>(program);
        }
        LOG_WARN << "MainRenderPass: GLSL compilation failed, trying SPIR-V" << std::endl;
    }
    try {
        auto sp = std::make_shared<ShaderProgram>("vert.spv", "frag.spv");
        LOG_INFO << "MainRenderPass: Loaded from SPIR-V binary" << std::endl;
        return sp;
    } catch (const std::exception& e) {
        LOG_ERROR << "MainRenderPass: All shader loading failed: " << e.what() << std::endl;
        return nullptr;
    }
}

bool MainRenderPass::reloadShaders() {
    LOG_INFO << "=== Hot-reloading shaders ===" << std::endl;
    auto newShader = compileShaders("vert.shader", "frag.shader");
    if (newShader) {
        newShader->bindUniformBlock("MatricesBlock", 1);
        shader_ = newShader;
        pipeline_.setShader(shader_);
        LOG_INFO << "=== Shader hot-reload SUCCESS ===" << std::endl;
        return true;
    }
    LOG_ERROR << "=== Shader hot-reload FAILED, keeping old shader ===" << std::endl;
    return false;
}

void MainRenderPass::execute(RenderContext& ctx) {
    // Clear full screen first (before setting viewport)
    Framebuffer::bindDefault();
    glViewport(0, 0, ctx.framebufferWidth, ctx.framebufferHeight);
    glClearColor(clearInfo_.clearColorValue[0], clearInfo_.clearColorValue[1],
                 clearInfo_.clearColorValue[2], clearInfo_.clearColorValue[3]);
    glClearDepth(clearInfo_.clearDepthValue);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set 3D viewport for rendering
    glViewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height);

    pipeline_.bind();

    if (ctx.wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    MatricesBlock block{};
    block.view = ctx.camera->getViewMatrix();
    float aspect = static_cast<float>(ctx.viewportWidth) / static_cast<float>(ctx.viewportHeight);
    block.projection = ctx.camera->getProjectionMatrix(aspect);
    block.cameraPos = ctx.camera->getPosition();
    block.pad = 0.0f;
    ubo_.update(block);
    ubo_.bindBase(1);

    if (ctx.model && ctx.model->getSSBO().id() != 0) {
        ctx.model->getSSBO().bindBase(0);
    }

    shader_->setUniform(0, ctx.modelMatrix);
    shader_->setUniform(4, ctx.lightDir);
    shader_->setUniform(5, ctx.lightColor * ctx.lightIntensity);
    shader_->setUniform(9, ctx.ambientIntensity);

    if (ctx.model) {
        ctx.model->draw(ctx.useMipmap);
    }

    if (ctx.wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (ctx.showBoundingBox && ctx.model)
    {
        bboxShader_->use();
        bboxShader_->setUniform(0, ctx.modelMatrix);
        bboxShader_->setUniform(1, ctx.bboxAlpha);
        glDisable(GL_DEPTH_TEST);  // draw bbox as overlay so it is visible through the mesh
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        ctx.model->drawBoundingBox();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    }

    endPass();
}

void MainRenderPass::resize(int w, int h) {}
void MainRenderPass::destroy() { ubo_.destroy(); shader_.reset(); }

// --- Renderer ---

Renderer::~Renderer() { destroy(); }

void Renderer::init(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    LOG_INFO << "Renderer initialized (" << width << "x" << height << ")" << std::endl;

	for (auto& pass : passes_) {
		if (pass) pass->setup();
	}
}

void Renderer::render(RenderContext& ctx) {
    for (auto& pass : passes_) {
        if (pass && pass->enabled()) pass->execute(ctx);
    }
}

void Renderer::resize(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    for (auto& pass : passes_) { if (pass) pass->resize(width, height); }
}

void Renderer::destroy() {
    for (auto& pass : passes_) { if (pass) pass->destroy(); }
    passes_.clear();
    for (auto& [name, fbo] : framebuffers_) { fbo.destroy(); }
    framebuffers_.clear();
}

void Renderer::addPass(std::unique_ptr<RenderPass> pass) {
    passes_.push_back(std::move(pass));
}

RenderPass* Renderer::getPass(const std::string& name) const {
    for (const auto& pass : passes_) {
        if (pass && pass->name() == name) return pass.get();
    }
    return nullptr;
}

Framebuffer& Renderer::createFramebuffer(const std::string& name, int w, int h) {
    auto it = framebuffers_.find(name);
    if (it != framebuffers_.end()) {
        it->second.destroy();
        it->second.create(w, h);
        return it->second;
    }
    auto [insertIt, _] = framebuffers_.emplace(name, Framebuffer(w, h));
    return insertIt->second;
}

Framebuffer* Renderer::getFramebuffer(const std::string& name) {
    auto it = framebuffers_.find(name);
    if (it != framebuffers_.end()) return &it->second;
    return nullptr;
}

bool Renderer::reloadShaders() {
    bool anySuccess = false;
    for (auto& pass : passes_) {
        auto* mainPass = dynamic_cast<MainRenderPass*>(pass.get());
        if (mainPass) {
            if (mainPass->reloadShaders()) anySuccess = true;
        }
    }
    return anySuccess;
}
