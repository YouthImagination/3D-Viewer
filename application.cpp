#include "application.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "backend.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stb_image.h>

#include <openssl/ssl.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) { glfwGetCursorPos(window, &app->lastX, &app->lastY); app->leftMouseDown = true; }
        else if (action == GLFW_RELEASE) { app->leftMouseDown = false; }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) { glfwGetCursorPos(window, &app->lastX, &app->lastY); app->rightMouseDown = true; }
        else if (action == GLFW_RELEASE) { app->rightMouseDown = false; }
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app->leftMouseDown && !app->rightMouseDown) return;
    double dx = xpos - app->lastX;
    double dy = ypos - app->lastY;
    app->lastX = xpos;
    app->lastY = ypos;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    if (app->leftMouseDown) app->camera.rotate(static_cast<float>(dx), static_cast<float>(-dy));
    else if (app->rightMouseDown) app->camera.pan(static_cast<float>(dx), static_cast<float>(dy));
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float menuBarHeight = 25.0f * app->dpiScale;
    float sidebarWidth = 320.0f * app->dpiScale;
    float timelineHeight = 50.0f * app->dpiScale;
    float vpLeft = 0.0f;
    float vpRight = static_cast<float>(width) - sidebarWidth;
    float vpTop = menuBarHeight;
    float vpBottom = static_cast<float>(height) - timelineHeight;
    bool inViewport = (xpos >= vpLeft && xpos <= vpRight && ypos >= vpTop && ypos <= vpBottom);
    if (!inViewport && ImGui::GetIO().WantCaptureMouse) return;
    app->camera.zoom(static_cast<float>(yoffset));
}

static void drop_callback(GLFWwindow* window, int count, const char** paths) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (count > 0) app->loadNewModel(paths[0]);
}

Application::Application(uint32_t w, uint32_t h)
    : height_(h), width_(w)
{
    OPENSSL_init_ssl(0, nullptr);
    srand((unsigned int)time(nullptr));
    if (!glfwInit()) LOG_ERROR << "failed to init glfw" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    if (!windowHandle_) {
        const char* apiStr = BackendInstance->isOpenGL() ? "gl" : "vk";
        title_ = title_ + " (" + apiStr + ")";
        char titleBuf[256];
        snprintf(titleBuf, sizeof(titleBuf), "%s | FPS: %.1f", title_.c_str(), 0.0f);
        windowHandle_ = glfwCreateWindow(w, h, titleBuf, nullptr, nullptr);
        if (!windowHandle_) glfwTerminate();
    }
    glfwMakeContextCurrent(windowHandle_);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(windowHandle_, this);

    glfwSetMouseButtonCallback(windowHandle_, mouse_button_callback);
    glfwSetCursorPosCallback(windowHandle_, cursor_position_callback);
    glfwSetScrollCallback(windowHandle_, scroll_callback);
    glfwSetDropCallback(windowHandle_, drop_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(windowHandle_);
        glfwTerminate();
    }

    LOG_INFO << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    std::vector<GLFWimage> icons;
    const char* iconSize[6] = { "16", "32", "48", "64", "128", "256" };
    for (int i = 0; i < 6; i++) {
        GLFWimage icon;
        std::string icon_name = "./icons/icon_" + std::string(iconSize[i]) + ".png";
        icon.pixels = stbi_load(icon_name.c_str(), &icon.width, &icon.height, 0, 4);
        if (!icon.pixels) {
            LOG_ERROR << "failed to load icon: " << icon_name << std::endl;
            icon.width = icon.height = 0;
            stbi_image_free(icon.pixels);
            continue;
        }
        icons.push_back(icon);
    }
    glfwSetWindowIcon(windowHandle_, icons.size(), icons.data());
    for (int i = 0; i < (int)icons.size(); ++i) stbi_image_free(icons[i].pixels);

    float xScale = 1.0f, yScale = 1.0f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    dpiScale = xScale;
}

Application::~Application() {
    if (windowHandle_) { glfwDestroyWindow(windowHandle_); windowHandle_ = nullptr; }
    glfwTerminate();
}

void Application::onAttach() {}
void Application::onDetach() {}
void Application::onUpdate(float deltaTime) {}

void Application::onUpdateUI(float deltaTime) {
    updateTitle(deltaTime);
}

void Application::run() {
    onAttach();
    lastFrameTime_ = (float)glfwGetTime();
    while (!glfwWindowShouldClose(windowHandle_)) {
        glfwPollEvents();
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime_;
        lastFrameTime_ = currentFrameTime;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        onUpdate(deltaTime);
        onUpdateUI(deltaTime);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
        glfwSwapBuffers(windowHandle_);
    }
    onDetach();
}

void Application::loadNewModel(const char* path) {
	
}

void Application::updateTitle(float deltaTime) {
    static float titleTimer = 0.0f;
    titleTimer += deltaTime;
    if (titleTimer >= 0.5f) {
        float fps = ImGui::GetCurrentContext() ? ImGui::GetIO().Framerate : (1.0f / (deltaTime > 0.0001f ? deltaTime : 0.0001f));
        char titleBuf[256];
        snprintf(titleBuf, sizeof(titleBuf), "%s | FPS: %.1f", title_.c_str(), fps);
        glfwSetWindowTitle(windowHandle_, titleBuf);
        titleTimer = 0.0f;
    }
}
