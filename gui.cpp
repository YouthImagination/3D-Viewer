#include "gui.h"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include <httplib.h>
#include <openssl/ssl.h>

#include <ImGuiFileDialog.h>

#include <thread>
#include <nlohmann/json.hpp>
#include <random>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstdarg>

#include <glm/gtc/type_ptr.hpp>

// ============================================================
// ImGui Lifecycle
// ============================================================

void GUI::init(GLFWwindow* window, float dpiScale) {
    dpiScale_ = dpiScale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Light theme
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4 bg = ImVec4(0.96f, 0.96f, 0.97f, 1.0f);
    ImVec4 cardBg = ImVec4(0.91f, 0.91f, 0.92f, 1.0f);
    ImVec4 activeBlue = ImVec4(0.0f, 0.47f, 0.83f, 1.0f);
    ImVec4 textDark = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);

    style.Colors[ImGuiCol_WindowBg] = bg;
    style.Colors[ImGuiCol_ChildBg] = bg;
    style.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.86f, 1.0f);
    style.Colors[ImGuiCol_Text] = textDark;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.95f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.92f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = bg;
    style.Colors[ImGuiCol_TitleBgActive] = bg;
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.95f, 0.98f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.82f, 0.90f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_Header] = cardBg;
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.95f, 0.98f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.82f, 0.90f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = activeBlue;
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.38f, 0.68f, 1.0f);

    style.ScaleAllSizes(dpiScale_);
    sidebarWidth_ *= dpiScale_;
    bottomBarHeight_ *= dpiScale_;
    menuBarHeight_ *= dpiScale_;

    // Load fonts
    fontDefault_ = io.Fonts->AddFontDefault();
    float fontSize = 16.0f * dpiScale_;

    std::string savedFontPath = "";
    std::ifstream settingsFile("settings.txt");
    if (settingsFile.is_open()) {
        std::getline(settingsFile, savedFontPath);
    }

    ImFontConfig fontConfig;
    fontConfig.Flags |= ImFontFlags_NoLoadError;
    if (!savedFontPath.empty() && std::filesystem::exists(savedFontPath)) {
        activeFont_ = io.Fonts->AddFontFromFileTTF(savedFontPath.c_str(), fontSize, &fontConfig, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        currentFontPath_ = savedFontPath;
    }

    if (!activeFont_) {
#ifdef _WIN32
        std::vector<std::string> candidates = {
            "C:\\Windows\\Fonts\\msyh.ttc",
            "C:\\Windows\\Fonts\\msyh.ttf",
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\arial.ttf"
        };
#else
        std::vector<std::string> candidates = {
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
            "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
            "/usr/share/fonts/truetype/arphic/uming.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        };
#endif
        for (const auto& path : candidates) {
            if (std::filesystem::exists(path)) {
                activeFont_ = io.Fonts->AddFontFromFileTTF(path.c_str(), fontSize, &fontConfig, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                currentFontPath_ = path;
                break;
            }
        }
    }

    if (!activeFont_) {
        activeFont_ = fontDefault_;
        currentFontPath_ = "";
    }

    scanAvailableFonts();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
}

void GUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUI::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUI::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

// ============================================================
// Main draw entry
// ============================================================

void GUI::draw(float deltaTime, Model& model, ArcballCamera& camera,
               GLFWwindow* window, float dpiScale,
               const std::function<void()>& resetCameraFn) {
    if (activeFont_) ImGui::PushFont(activeFont_);

    drawMenuBar(window, resetCameraFn);
    drawSidebar(deltaTime, model, camera, window, resetCameraFn);
    drawBottomPlayback(model);
    drawSettingsWindow();
    drawLogWindow();

    if (activeFont_) ImGui::PopFont();
}

// ============================================================
// Settings accessors
// ============================================================

glm::vec3 GUI::getLightDirection() const {
    float radAzimuth = glm::radians(lightAzimuth_);
    float radElevation = glm::radians(lightElevation_);
    return glm::vec3(
        std::cos(radElevation) * std::sin(radAzimuth),
        std::sin(radElevation),
        std::cos(radElevation) * std::cos(radAzimuth)
    );
}

// ============================================================
// Download / Model management
// ============================================================

namespace {

std::string sanitizeURL(const std::string& inputURL) {
    std::string cleanURL = inputURL;
    size_t ghPos = cleanURL.find("github.com");
    if (ghPos != std::string::npos) {
        cleanURL.replace(ghPos, 10, "raw.githubusercontent.com");
        size_t blobPos = cleanURL.find("/blob/");
        if (blobPos != std::string::npos) {
            cleanURL.erase(blobPos, 5);
        }
    }
    return cleanURL;
}

} // anonymous namespace

void GUI::startDownload(const std::string& url) {
    std::lock_guard<std::mutex> lock(downloadMutex_);
    if (downloadRunning_) return;
    downloadRunning_ = true;
    downloadReady_ = false;
    downloadStatus_ = "Downloading...";

    // Capture this and url for the thread
    std::thread t([this](std::string url) {
        LOG_INFO << "[Downloader] Starting download thread for URL: " << url << std::endl;
        std::string cleanURL = sanitizeURL(url);
        LOG_INFO << "[Downloader] Sanitized URL: " << cleanURL << std::endl;

        std::string host, path;
        bool isHTTPS = false;
        if (cleanURL.rfind("https://", 0) == 0) {
            isHTTPS = true;
            std::string stripped = cleanURL.substr(8);
            size_t slashPos = stripped.find('/');
            if (slashPos != std::string::npos) { host = stripped.substr(0, slashPos); path = stripped.substr(slashPos); }
            else { host = stripped; path = "/"; }
        } else if (cleanURL.rfind("http://", 0) == 0) {
            std::string stripped = cleanURL.substr(7);
            size_t slashPos = stripped.find('/');
            if (slashPos != std::string::npos) { host = stripped.substr(0, slashPos); path = stripped.substr(slashPos); }
            else { host = stripped; path = "/"; }
        } else {
            std::lock_guard<std::mutex> lock(downloadMutex_);
            downloadStatus_ = "Failed: Invalid protocol (must be http/https)";
            downloadRunning_ = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(downloadMutex_);
            downloadStatus_ = "Connecting to " + host + "...";
        }

        try {
            std::vector<char> localData;
            int status = 0;
            if (isHTTPS) {
                httplib::SSLClient cli(host);
                cli.enable_server_certificate_verification(false);
                cli.set_follow_location(true);
                cli.set_connection_timeout(10, 0);
                cli.set_read_timeout(30, 0);
                auto res = cli.Get(path.c_str());
                if (res) { status = res->status; if (status == 200) localData.assign(res->body.begin(), res->body.end()); }
                else status = -1;
            } else {
                httplib::Client cli(host);
                cli.set_follow_location(true);
                cli.set_connection_timeout(10, 0);
                cli.set_read_timeout(30, 0);
                auto res = cli.Get(path.c_str());
                if (res) { status = res->status; if (status == 200) localData.assign(res->body.begin(), res->body.end()); }
                else status = -1;
            }
            std::lock_guard<std::mutex> lock(downloadMutex_);
            if (status == 200) { webModelData_ = std::move(localData); downloadReady_ = true; downloadStatus_ = "Download Complete! Processing model..."; }
            else if (status == -1) downloadStatus_ = "Failed: Connection Error";
            else downloadStatus_ = "Failed: HTTP Status " + std::to_string(status);
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(downloadMutex_);
            downloadStatus_ = std::string("Failed: Exception: ") + e.what();
        } catch (...) {
            std::lock_guard<std::mutex> lock(downloadMutex_);
            downloadStatus_ = "Failed: Unknown Exception";
        }

        std::lock_guard<std::mutex> lock(downloadMutex_);
        downloadRunning_ = false;
    }, url);
    t.detach();
}

void GUI::startFetchIndex() {
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (indexFetching_) return;
    indexFetching_ = true;
    indexReady_ = false;
    indexStatus_ = "Fetching model index...";

    std::thread t([this]() {
        std::string host = "raw.githubusercontent.com";
        std::string path = "/KhronosGroup/glTF-Sample-Assets/main/Models/model-index.json";

        {
            std::lock_guard<std::mutex> lock(indexMutex_);
            indexStatus_ = "Connecting to raw.githubusercontent.com...";
        }

        try {
            httplib::SSLClient cli(host);
            cli.enable_server_certificate_verification(false);
            cli.set_follow_location(true);
            cli.set_connection_timeout(10, 0);
            cli.set_read_timeout(15, 0);
            auto res = cli.Get(path.c_str());
            if (res && res->status == 200) {
                auto j = nlohmann::json::parse(res->body);
                std::vector<std::pair<std::string, std::string>> parsedModels;
                for (auto& item : j) {
                    if (item.contains("variants") && item["variants"].contains("glTF-Binary")) {
                        std::string label = item["label"].get<std::string>();
                        std::string name = item["name"].get<std::string>();
                        std::string filename = item["variants"]["glTF-Binary"].get<std::string>();
                        std::string url = "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/" + name + "/glTF-Binary/" + filename;
                        parsedModels.push_back({label, url});
                    }
                }
                std::sort(parsedModels.begin(), parsedModels.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
                std::lock_guard<std::mutex> lock(indexMutex_);
                webModels_ = std::move(parsedModels);
                indexReady_ = true;
                indexStatus_ = "Success: Loaded " + std::to_string(webModels_.size()) + " models!";
            } else {
                int status = res ? res->status : -1;
                std::lock_guard<std::mutex> lock(indexMutex_);
                if (status == -1) indexStatus_ = "Failed: Connection Error";
                else indexStatus_ = "Failed: HTTP Status " + std::to_string(status);
            }
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(indexMutex_);
            indexStatus_ = std::string("Failed: JSON/HTTP Error: ") + e.what();
        } catch (...) {
            std::lock_guard<std::mutex> lock(indexMutex_);
            indexStatus_ = "Failed: Unknown Error";
        }
        std::lock_guard<std::mutex> lock(indexMutex_);
        indexFetching_ = false;
    });
    t.detach();
}

bool GUI::consumeDownloadedModel(std::vector<char>& outData) {
    std::lock_guard<std::mutex> lock(downloadMutex_);
    if (downloadReady_) {
        outData = std::move(webModelData_);
        downloadReady_ = false;
        downloadStatus_ = "Processing model...";
        return true;
    }
    return false;
}

bool GUI::checkPendingRandomLoad() {
    std::lock_guard<std::mutex> lock(indexMutex_);
    if (pendingRandomLoad_ && indexReady_ && !webModels_.empty()) {
        pendingRandomLoad_ = false;
        return true;
    }
    return false;
}

void GUI::processRandomLoad() {
    std::lock_guard<std::mutex> lock(indexMutex_);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)webModels_.size() - 1);
    int idx = dis(gen);
    selectedModelIndex_ = idx;
    strncpy(urlInputBuf_, webModels_[idx].second.c_str(), sizeof(urlInputBuf_) - 1);
    urlInputBuf_[sizeof(urlInputBuf_) - 1] = '\0';
    startDownload(urlInputBuf_);
}

bool GUI::isDownloadRunning() const {
    std::lock_guard<std::mutex> lock(downloadMutex_);
    return downloadRunning_;
}

std::string GUI::getDownloadStatus() {
    std::lock_guard<std::mutex> lock(downloadMutex_);
    return downloadStatus_;
}


bool GUI::isSelectFileModel() const
{
    return fileModelSelected_;
}

void GUI::clearSelectedFileModelStatus()
{
    fileModelSelected_ = false;
}

std::string GUI::getSelectedModelFile()
{
    return selectedModelFile_;
}

bool GUI::isIndexReady() const {
    std::lock_guard<std::mutex> lock(indexMutex_);
    return indexReady_;
}

bool GUI::isIndexFetching() const {
    std::lock_guard<std::mutex> lock(indexMutex_);
    return indexFetching_;
}

std::string GUI::getIndexStatus() {
    std::lock_guard<std::mutex> lock(indexMutex_);
    return indexStatus_;
}

std::vector<std::pair<std::string, std::string>> GUI::getWebModels() {
    std::lock_guard<std::mutex> lock(indexMutex_);
    return webModels_;
}

// ============================================================
// Font management
// ============================================================

void GUI::scanAvailableFonts() {
    availableFonts_.clear();
    availableFonts_.push_back({ "Default", "", "ImGui Default" });

    #ifdef _WIN32
    std::vector<std::string> fontFolders = { "C:\\Windows\\Fonts" };
#else
    std::vector<std::string> fontFolders = {
        "/usr/share/fonts/truetype",
        "/usr/share/fonts/opentype",
        "/usr/share/fonts/truetype/wqy",
        "/usr/share/fonts/truetype/dejavu"
    };
#endif
    for (const auto& fontsFolder : fontFolders) {
        if (!std::filesystem::exists(fontsFolder)) continue;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(fontsFolder)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ttf" || ext == ".ttc" || ext == ".otf") {
                        FontInfo info;
                        info.path = entry.path().string();
                        info.filename = entry.path().filename().string();
                        info.displayName = entry.path().stem().string();
                        availableFonts_.push_back(info);
                    }
                }
            }
        } catch (...) {}
    }
}

void GUI::saveSettings() {
    std::ofstream file("settings.txt");
    if (file.is_open()) {
        file << currentFontPath_ << "\n";
    }
}

void GUI::changeActiveFont(const std::string& fontPath) {
    if (fontPath == currentFontPath_) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    fontDefault_ = io.Fonts->AddFontDefault();

    float xS = 1.0f, yS = 1.0f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) glfwGetMonitorContentScale(monitor, &xS, &yS);
    float fontSize = 16.0f * xS;

    if (!fontPath.empty() && std::filesystem::exists(fontPath)) {
        ImFontConfig fontConfig;
        fontConfig.Flags |= ImFontFlags_NoLoadError;
        ImFont* loaded = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (loaded) { activeFont_ = loaded; currentFontPath_ = fontPath; }
        else { activeFont_ = fontDefault_; currentFontPath_ = ""; }
    } else {
        activeFont_ = fontDefault_;
        currentFontPath_ = "";
    }

    unsigned char* texPixels;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&texPixels, &texWidth, &texHeight);
    ImGui_ImplOpenGL3_Init("#version 450");
}

// ============================================================
// Custom ImGui Widgets
// ============================================================

bool GUI::drawIconTab(const char* id_str, int index, int& activeIndex, ImVec2 size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiID id = window->GetID(id_str);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) activeIndex = index;

    bool isActive = (activeIndex == index);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bg_col = ImGui::GetColorU32(isActive ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_ButtonHovered : ImGuiCol_WindowBg));
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg_col, 4.0f);
    ImU32 icon_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

    if (index == 0) {
        drawList->AddCircle(center, 6.0f, icon_col, 16, 2.0f);
        for (int i = 0; i < 8; i++) {
            float angle = i * (2.0f * 3.1415926f / 8.0f);
            ImVec2 p1 = ImVec2(center.x + 8.0f * std::cos(angle), center.y + 8.0f * std::sin(angle));
            ImVec2 p2 = ImVec2(center.x + 11.0f * std::cos(angle), center.y + 11.0f * std::sin(angle));
            drawList->AddLine(p1, p2, icon_col, 1.5f);
        }
    } else if (index == 1) {
        ImVec2 box_min = ImVec2(center.x - 9.0f, center.y - 7.0f);
        ImVec2 box_max = ImVec2(center.x + 9.0f, center.y + 7.0f);
        drawList->AddRect(box_min, box_max, icon_col, 1.0f, 0, 1.5f);
        drawList->AddLine(ImVec2(box_min.x, box_min.y + 4.0f), ImVec2(box_max.x, box_min.y + 4.0f), icon_col, 1.0f);
        drawList->AddLine(ImVec2(center.x - 5.0f, center.y + 4.0f), ImVec2(center.x - 5.0f, center.y + 0.0f), icon_col, 2.0f);
        drawList->AddLine(ImVec2(center.x - 1.0f, center.y + 4.0f), ImVec2(center.x - 1.0f, center.y - 2.0f), icon_col, 2.0f);
        drawList->AddLine(ImVec2(center.x + 3.0f, center.y + 4.0f), ImVec2(center.x + 3.0f, center.y + 1.0f), icon_col, 2.0f);
    } else if (index == 2) {
        float grid_r = 8.0f;
        ImVec2 grid_min = ImVec2(center.x - grid_r, center.y - grid_r);
        ImVec2 grid_max = ImVec2(center.x + grid_r, center.y + grid_r);
        drawList->AddRect(grid_min, grid_max, icon_col, 2.0f, 0, 1.5f);
        float step_x = (grid_max.x - grid_min.x) / 3.0f;
        float step_y = (grid_max.y - grid_min.y) / 3.0f;
        drawList->AddLine(ImVec2(grid_min.x + step_x, grid_min.y), ImVec2(grid_min.x + step_x, grid_max.y), icon_col, 1.0f);
        drawList->AddLine(ImVec2(grid_min.x + 2 * step_x, grid_min.y), ImVec2(grid_min.x + 2 * step_x, grid_max.y), icon_col, 1.0f);
        drawList->AddLine(ImVec2(grid_min.x, grid_min.y + step_y), ImVec2(grid_max.x, grid_min.y + step_y), icon_col, 1.0f);
        drawList->AddLine(ImVec2(grid_min.x, grid_min.y + 2 * step_y), ImVec2(grid_max.x, grid_min.y + 2 * step_y), icon_col, 1.0f);
    } else if (index == 3) {
        ImVec2 p_top = ImVec2(center.x, center.y - 8.0f);
        ImVec2 p_left = ImVec2(center.x - 8.0f, center.y - 3.0f);
        ImVec2 p_right = ImVec2(center.x + 8.0f, center.y - 3.0f);
        ImVec2 p_mid = ImVec2(center.x, center.y + 2.0f);
        ImVec2 p_bot_left = ImVec2(center.x - 8.0f, center.y + 7.0f);
        ImVec2 p_bot_right = ImVec2(center.x + 8.0f, center.y + 7.0f);
        ImVec2 p_bot = ImVec2(center.x, center.y + 12.0f);
        drawList->AddLine(p_top, p_left, icon_col, 1.5f);
        drawList->AddLine(p_top, p_right, icon_col, 1.5f);
        drawList->AddLine(p_left, p_mid, icon_col, 1.5f);
        drawList->AddLine(p_right, p_mid, icon_col, 1.5f);
        drawList->AddLine(p_left, p_bot_left, icon_col, 1.5f);
        drawList->AddLine(p_right, p_bot_right, icon_col, 1.5f);
        drawList->AddLine(p_mid, p_bot, icon_col, 1.5f);
        drawList->AddLine(p_bot_left, p_bot, icon_col, 1.5f);
        drawList->AddLine(p_bot_right, p_bot, icon_col, 1.5f);
    }

    if (isActive) {
        float barHeight = 3.0f;
        ImU32 active_col = ImGui::GetColorU32(ImVec4(0.0f, 0.47f, 0.83f, 1.0f));
        drawList->AddRectFilled(ImVec2(pos.x + 5.0f, pos.y + size.y - barHeight), ImVec2(pos.x + size.x - 5.0f, pos.y + size.y), active_col, 1.5f);
    }

    return pressed;
}

bool GUI::drawLightRotationDial(const char* label, float& azimuth) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    float radius = 70.0f * dpiScale_;
    float width = ImGui::GetContentRegionAvail().x;
    ImRect bb(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y), ImVec2(ImGui::GetCursorScreenPos().x + width, ImGui::GetCursorScreenPos().y + 2.0f * radius + 20.0f));
    ImVec2 center = ImVec2(bb.Min.x + width * 0.5f, bb.Min.y + radius + 10.0f);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (held) {
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        float dx = mousePos.x - center.x;
        float dy = mousePos.y - center.y;
        float angle = std::atan2(dy, dx);
        float deg = glm::degrees(angle) + 90.0f;
        if (deg < 0.0f) deg += 360.0f;
        azimuth = deg;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_Border), 64, 2.0f);
    drawList->AddCircleFilled(center, radius - 6.0f, ImGui::GetColorU32(ImGuiCol_FrameBg), 64);

    float knobAngle = glm::radians(azimuth - 90.0f);
    ImVec2 knobPos = ImVec2(center.x + radius * std::cos(knobAngle), center.y + radius * std::sin(knobAngle));
    drawList->AddCircleFilled(knobPos, 10.0f, ImGui::GetColorU32(ImGuiCol_ButtonActive), 16);
    drawList->AddCircle(knobPos, 10.0f, ImGui::GetColorU32(ImGuiCol_Border), 16, 2.0f);
    for (int i = 0; i < 8; i++) {
        float rayAngle = i * (2.0f * 3.1415926f / 8.0f);
        ImVec2 p1 = ImVec2(knobPos.x + 6.0f * std::cos(rayAngle), knobPos.y + 6.0f * std::sin(rayAngle));
        ImVec2 p2 = ImVec2(knobPos.x + 12.0f * std::cos(rayAngle), knobPos.y + 12.0f * std::sin(rayAngle));
        drawList->AddLine(p1, p2, ImGui::GetColorU32(ImGuiCol_Border), 1.5f);
    }

    return held;
}

// ============================================================
// Sub-panel draw methods
// ============================================================

void GUI::drawMenuBar(GLFWwindow* window, const std::function<void()>& resetCameraFn) {
	{
		if (showFileDialog_) {
			IGFD::FileDialogConfig config;
			config.path = ".";  // ���ó�ʼĿ¼
			// �����������Ҫ��Ҳ�������� config.flags ��

			ImGuiFileDialog::Instance()->OpenDialog(
				"ChooseModelDlg",                   // vKey
				"Choose a 3D Model",                // vTitle
				"Model files (*.gltf *.glb *.obj *.fbx){.gltf,.glb,.obj,.fbx},.*", // vFilters
				config                              // vConfig
			);
			showFileDialog_ = false;
		}

		if (ImGuiFileDialog::Instance()->Display("ChooseModelDlg")) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				selectedModelFile_ = ImGuiFileDialog::Instance()->GetFilePathName();
                fileModelSelected_ = true;
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open File..."))
            {
                showFileDialog_ = true;
            }

            if (ImGui::MenuItem("Save settings")) saveSettings();
            if (ImGui::MenuItem("Load settings")) {
                std::ifstream settingsFile("settings.txt");
                std::string path;
                if (settingsFile.is_open()) { std::getline(settingsFile, path); changeActiveFont(path); }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(window, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr, &showSettingsWindow_);
            ImGui::MenuItem("Log Window", nullptr, &showLogWindow_);
            if (ImGui::MenuItem("Reset Camera")) resetCameraFn();
            ImGui::Separator();
            if (ImGui::MenuItem("Reload Shader (F5)")) shaderReloadRequested_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About 3D Viewer")) LOG_INFO << "OpenGL DSA 3D Viewer" << std::endl;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GUI::drawSidebar(float deltaTime, Model& model, ArcballCamera& camera,
                       GLFWwindow* window, const std::function<void()>& resetCameraFn) {
    ImVec2 mainPos = ImGui::GetMainViewport()->Pos;
    ImVec2 mainSize = ImGui::GetMainViewport()->Size;

    ImGui::SetNextWindowPos(ImVec2(mainPos.x + mainSize.x - sidebarWidth_, mainPos.y + menuBarHeight_));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth_, mainSize.y - menuBarHeight_));

    ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::Begin("SidebarPanel", nullptr, sidebarFlags);

    float buttonWidth = ImGui::GetContentRegionAvail().x / 4.0f;
    float iconSizeVal = 36.0f * dpiScale_;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    drawIconTab("##TabLighting", 0, activeTab_, ImVec2(buttonWidth, iconSizeVal));
    ImGui::SameLine();
    drawIconTab("##TabStats", 1, activeTab_, ImVec2(buttonWidth, iconSizeVal));
    ImGui::SameLine();
    drawIconTab("##TabGrid", 2, activeTab_, ImVec2(buttonWidth, iconSizeVal));
    ImGui::SameLine();
    drawIconTab("##TabLibrary", 3, activeTab_, ImVec2(buttonWidth, iconSizeVal));
    ImGui::PopStyleVar();

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Spacing();

    struct LightThemePreset {
        const char* name;
        float azimuth;
        float elevation;
        float intensity;
        float ambient;
        glm::vec3 color;
    };

    const std::vector<LightThemePreset> lightThemes = {
        { "Default", 45.0f, 45.0f, 1.2f, 0.20f, glm::vec3(1.0f) },
        { "Sunset", 270.0f, 10.0f, 1.5f, 0.10f, glm::vec3(0.95f, 0.55f, 0.35f) },
        { "Cool", 180.0f, 40.0f, 1.0f, 0.30f, glm::vec3(0.6f, 0.8f, 1.0f) },
        { "Teal", 45.0f, 30.0f, 1.1f, 0.20f, glm::vec3(0.2f, 0.8f, 0.7f) },
        { "Studio", 90.0f, 60.0f, 1.3f, 0.25f, glm::vec3(1.0f, 0.95f, 0.9f) },
        { "Light", 45.0f, 75.0f, 1.4f, 0.35f, glm::vec3(0.9f, 0.9f, 0.95f) },
        { "Dark", 0.0f, 20.0f, 0.5f, 0.05f, glm::vec3(0.4f, 0.3f, 0.6f) },
        { "White", 45.0f, 90.0f, 1.5f, 0.50f, glm::vec3(1.0f) },
        { "Brick", 120.0f, 25.0f, 1.3f, 0.15f, glm::vec3(0.8f, 0.4f, 0.3f) }
    };

    if (activeTab_ == 0) {
        // Environment & Lighting tab
        ImGui::Text("Environment & Lighting");
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Themes", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            float itemWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
            float boxSize = 20.0f * dpiScale_;

            for (size_t i = 0; i < lightThemes.size(); i++) {
                const auto& theme = lightThemes[i];
                ImGui::PushID(static_cast<int>(i));
                ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(cursor, ImVec2(cursor.x + boxSize, cursor.y + boxSize),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(theme.color.x, theme.color.y, theme.color.z, 1.0f)), 3.0f);
                drawList->AddRect(cursor, ImVec2(cursor.x + boxSize, cursor.y + boxSize),
                    ImGui::GetColorU32(ImGuiCol_Border), 3.0f, 0, 1.0f);
                ImGui::Dummy(ImVec2(boxSize, boxSize));
                ImGui::SameLine();
                float btnW = itemWidth - boxSize - ImGui::GetStyle().ItemSpacing.x;
                if (ImGui::Button(theme.name, ImVec2(btnW, boxSize))) {
                    lightAzimuth_ = theme.azimuth;
                    lightElevation_ = theme.elevation;
                    lightIntensity_ = theme.intensity;
                    ambientIntensity_ = theme.ambient;
                    lightColor_ = theme.color;
                }
                ImGui::PopID();
                if ((i + 1) % 3 != 0 && (i + 1) < lightThemes.size()) ImGui::SameLine();
            }

            ImGui::Spacing();
            if (ImGui::Button("Save settings", ImVec2(-1, 32.0f * dpiScale_))) saveSettings();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.95f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.90f, 0.92f, 1.0f));
            if (ImGui::Button("Load settings", ImVec2(-1, 32.0f * dpiScale_))) {
                std::ifstream settingsFile("settings.txt");
                std::string path;
                if (settingsFile.is_open()) { std::getline(settingsFile, path); changeActiveFont(path); }
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Light Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            drawLightRotationDial("##LightRotation", lightAzimuth_);
            ImGui::Spacing();
            ImGui::Text("Azimuth: %.1f deg", lightAzimuth_);
            ImGui::Spacing();
            ImGui::SliderFloat("Elevation", &lightElevation_, -90.0f, 90.0f, "%.1f");
            ImGui::SliderFloat("Intensity", &lightIntensity_, 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Ambient", &ambientIntensity_, 0.0f, 1.0f, "%.2f");
        }

    } else if (activeTab_ == 1) {
        // Stats & Shading tab
        auto& subMeshes = model.getSubMeshes();
        auto& meshVisibility = model.getMeshVisibility();

        ImGui::Text("Stats & Shading");
        ImGui::Spacing();
        ImGui::Text("Submeshes / Components:");

        if (!subMeshes.empty()) {
            bool anyChecked = false;
            for (size_t i = 0; i < meshVisibility.size(); i++) { if (meshVisibility[i]) { anyChecked = true; break; } }
            if (anyChecked) { if (ImGui::Button("Deselect All", ImVec2(-1, 32.0f * dpiScale_))) std::fill(meshVisibility.begin(), meshVisibility.end(), false); }
            else { if (ImGui::Button("Select All", ImVec2(-1, 32.0f * dpiScale_))) std::fill(meshVisibility.begin(), meshVisibility.end(), true); }

            ImGui::Spacing();
            if (ImGui::BeginChild("SubmeshesListChild", ImVec2(0, 110.0f * dpiScale_), true)) {
                for (size_t i = 0; i < subMeshes.size(); i++) {
                    std::string name = "Submesh " + std::to_string(i);
                    bool checked = meshVisibility[i];
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Checkbox(name.c_str(), &checked)) meshVisibility[i] = checked;
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
        } else {
            if (triangleVisibility_) { if (ImGui::Button("Deselect All", ImVec2(-1, 32.0f * dpiScale_))) triangleVisibility_ = false; }
            else { if (ImGui::Button("Select All", ImVec2(-1, 32.0f * dpiScale_))) triangleVisibility_ = true; }
            ImGui::Spacing();
            if (ImGui::BeginChild("SubmeshesListChild", ImVec2(0, 110.0f * dpiScale_), true)) {
                bool checked = triangleVisibility_;
                if (ImGui::Checkbox("Spinning Triangle", &checked)) triangleVisibility_ = checked;
                ImGui::EndChild();
            }
        }
        ImGui::Spacing();

        size_t totalIndices = 0, visibleMeshes = 0;
        for (size_t i = 0; i < subMeshes.size(); ++i) {
            if (i < meshVisibility.size() && meshVisibility[i]) { totalIndices += subMeshes[i].indexCount; visibleMeshes++; }
        }

        auto drawStatRow = [&](const char* key, const char* value) { ImGui::Text("%s", key); ImGui::SameLine(ImGui::GetWindowWidth() - 110.0f); ImGui::Text("%s", value); };
        auto drawStatRowInt = [&](const char* key, size_t value) { drawStatRow(key, std::to_string(value).c_str()); };

        if (ImGui::CollapsingHeader("Mesh Data", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            drawStatRowInt("Visible Meshes", visibleMeshes);
            drawStatRowInt("Total Meshes", subMeshes.size());
            drawStatRowInt("Indices Count", totalIndices);
            ImGui::Spacing();
        }
        if (ImGui::CollapsingHeader("Performance Data", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            drawStatRowInt("Active Draw Calls", visibleMeshes);
            float fps = ImGui::GetIO().Framerate;
            float frameTimeMs = 1000.0f / (fps > 0.001f ? fps : 0.001f);
            char fpsBuf[32], msBuf[32];
            snprintf(fpsBuf, sizeof(fpsBuf), "%.1f", fps);
            snprintf(msBuf, sizeof(msBuf), "%.2f ms", frameTimeMs);
            drawStatRow("FPS", fpsBuf);
            drawStatRow("Frame Time", msBuf);
            ImGui::Spacing();
        }
        ImGui::Separator();
        ImGui::Checkbox("Show BoundingBox", &showBoundingBox_);

        ImGui::Spacing();
        if (showBoundingBox_)
        {
            ImGui::SliderFloat("Alpha", &boundingBoxAlpha_, 0.0f, 1.0f);

            ImGui::Spacing();
        }


    } else if (activeTab_ == 2) {
        // Grid & Views tab
        ImGui::Text("Grid & Views");
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Viewpoints", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::Text("Presets");
            ImGui::Spacing();

            auto setViewpoint = [&](float yawDeg, float pitchDeg) {
                resetCameraFn();
                camera.setYawPitch(glm::radians(yawDeg), glm::radians(pitchDeg));
            };

            ImGui::Columns(3, "presets_columns", false);
            if (ImGui::Button("Iso", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(45.0f, 30.0f);
            ImGui::NextColumn();
            if (ImGui::Button("Front", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(0.0f, 0.0f);
            ImGui::NextColumn();
            if (ImGui::Button("Right", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(-90.0f, 0.0f);
            ImGui::NextColumn();
            if (ImGui::Button("Top", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(0.0f, 89.0f);
            ImGui::NextColumn();
            if (ImGui::Button("Left", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(90.0f, 0.0f);
            ImGui::NextColumn();
            if (ImGui::Button("Back", ImVec2(-1, 28.0f * dpiScale_))) setViewpoint(180.0f, 0.0f);
            ImGui::Columns(1);
            ImGui::Spacing();
            if (ImGui::Button("Reset View", ImVec2(-1, 32.0f * dpiScale_))) setViewpoint(0.0f, 0.0f);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            const char* projectionModes[] = { "Perspective", "Orthographic" };
            int projIdx = orthoProjection_ ? 1 : 0;
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##ProjectionCombo", &projIdx, projectionModes, 2)) {
                orthoProjection_ = (projIdx == 1);
                camera.orthoProjection = orthoProjection_;
            }
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::Checkbox("Wireframe Overlay", &wireframeMode_);
            ImGui::Spacing();
        }

        // screen shot
        if (ImGui::CollapsingHeader("Screen Shot", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            if (ImGui::Button("capture"))
                triggerScreenshot_ = true;

            if (texRef_.GetTexID() != ImTextureID_Invalid && screenshotWidth_ > 0 && screenshotHeight_ > 0)
            {
                ImGui::Spacing();
                ImGui::TextUnformatted("Preview:");

                // Fit the captured image to the sidebar width, keeping aspect ratio
                float availWidth = ImGui::GetContentRegionAvail().x;
                float aspect = (float)screenshotHeight_ / (float)screenshotWidth_;
                float displayWidth = availWidth;
                float displayHeight = displayWidth * aspect;
                const float maxHeight = 300.0f;
                if (displayHeight > maxHeight) {
                    displayHeight = maxHeight;
                    displayWidth = displayHeight / aspect;
                }
                ImGui::Image(texRef_, ImVec2(displayWidth, displayHeight));

                ImGui::Spacing();
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##SaveName", screenshotNameBuf_, sizeof(screenshotNameBuf_));
                if (ImGui::Button("Save", ImVec2(-1, 28.0f * dpiScale_)))
                {
                    savedName_ = screenshotNameBuf_;
                    saveScreenShot_ = true;
                }
                ImGui::TextDisabled("Click Save to write to disk");
            }
            ImGui::Spacing();
        }

    } else if (activeTab_ == 3) {
        // 3D Library tab
        ImGui::Text("3D library");
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Model Repository", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##UrlInput", urlInputBuf_, sizeof(urlInputBuf_));

            std::string statusStr = getDownloadStatus();
            bool isRunning = isDownloadRunning();
            std::string idxStatus = getIndexStatus();
            bool isFetchingIdx = isIndexFetching();
            bool isIdxReady = isIndexReady();
            std::vector<std::pair<std::string, std::string>> modelsCopy = getWebModels();

            if (isRunning) ImGui::BeginDisabled();
            if (ImGui::Button("Load from URL", ImVec2(-1, 30.0f * dpiScale_))) {
                if (isIdxReady && !modelsCopy.empty()) {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0, (int)modelsCopy.size() - 1);
                    int idx = dis(gen);
                    selectedModelIndex_ = idx;
                    strncpy(urlInputBuf_, modelsCopy[idx].second.c_str(), sizeof(urlInputBuf_) - 1);
                    urlInputBuf_[sizeof(urlInputBuf_) - 1] = '\0';
                    startDownload(urlInputBuf_);
                } else {
                    pendingRandomLoad_ = true;
                    startFetchIndex();
                }
            }
            if (isRunning) { ImGui::EndDisabled(); ImGui::SameLine(); ImGui::Text("Loading..."); }

            ImGui::Spacing();
            if (isFetchingIdx) ImGui::BeginDisabled();
            const char* fetchBtnLabel = modelsCopy.empty() ? "Fetch List" : "Refetch list";
            if (ImGui::Button(fetchBtnLabel, ImVec2(-1, 30.0f * dpiScale_))) startFetchIndex();
            if (isFetchingIdx) ImGui::EndDisabled();

            if (!modelsCopy.empty()) {
                ImGui::Spacing();
                std::vector<const char*> items;
                for (size_t i = 0; i < modelsCopy.size(); ++i) items.push_back(modelsCopy[i].first.c_str());
                if (ImGui::Combo("Select Model", &selectedModelIndex_, items.data(), (int)items.size())) {
                    strncpy(urlInputBuf_, modelsCopy[selectedModelIndex_].second.c_str(), sizeof(urlInputBuf_) - 1);
                    urlInputBuf_[sizeof(urlInputBuf_) - 1] = '\0';
                    startDownload(urlInputBuf_);
                }
            }

            ImGui::Spacing();
            ImGui::Text("Status: %s", statusStr.c_str());
            ImGui::TextWrapped("Index Status: %s", idxStatus.c_str());
        }
    }

    ImGui::End();
}

void GUI::drawBottomPlayback(Model& model) {
    ImVec2 mainPos = ImGui::GetMainViewport()->Pos;
    ImVec2 mainSize = ImGui::GetMainViewport()->Size;

    ImGui::SetNextWindowPos(ImVec2(mainPos.x, mainPos.y + mainSize.y - bottomBarHeight_));
    ImGui::SetNextWindowSize(ImVec2(mainSize.x - sidebarWidth_, bottomBarHeight_));

    ImGuiWindowFlags bottomFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::Begin("BottomPlaybackPanel", nullptr, bottomFlags);

    if (ImGui::Button(isPlaying_ ? "Pause" : "Play", ImVec2(60.0f * dpiScale_, 32.0f * dpiScale_))) isPlaying_ = !isPlaying_;
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(50.0f * dpiScale_, 32.0f * dpiScale_))) animationTimeTicks_ = 0.0f;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 300.0f * dpiScale_);

    float tempTime = 0.0f;
    float maxTime = 100.0f;
    if (model.hasAnimation()) {
        unsigned int animIdx = (activeAnimationIndex_ > 0) ? (activeAnimationIndex_ - 1) : 0;
        float duration = model.getAnimationDurationInSeconds(animIdx);
        float ticksPerSecond = model.getAnimationTicksPerSecond();
        maxTime = duration;
        tempTime = animationTimeTicks_ / ticksPerSecond;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f * dpiScale_));
    if (ImGui::SliderFloat("##Timeline", &tempTime, 0.0f, maxTime, "Time: %.2fs")) {
        if (model.hasAnimation()) {
            float ticksPerSecond = model.getAnimationTicksPerSecond();
            animationTimeTicks_ = tempTime * ticksPerSecond;
        }
        isPlaying_ = false;
    }
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f * dpiScale_);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f * dpiScale_));

    if (model.hasAnimation()) {
        std::vector<std::string> animNames;
        animNames.push_back("None");
        for (unsigned int i = 0; i < model.getAnimationCount(); i++) {
            std::string name = model.getAnimationName(i);
            animNames.push_back(name.empty() ? "Animation " + std::to_string(i + 1) : name);
        }
        std::vector<const char*> animItems;
        for (const auto& name : animNames) animItems.push_back(name.c_str());
        if (activeAnimationIndex_ >= static_cast<int>(animItems.size())) activeAnimationIndex_ = 0;
        if (ImGui::Combo("##AnimType", &activeAnimationIndex_, animItems.data(), static_cast<int>(animItems.size()))) animationTimeTicks_ = 0.0f;
    } else {
        const char* items[] = { "None" };
        int dummyIdx = 0;
        ImGui::Combo("##AnimType", &dummyIdx, items, 1);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f * dpiScale_);
    const char* speeds[] = { "x 0.25", "x 0.5", "x 1.0", "x 1.5", "x 2.0" };
    static int speedIdx = 2;
    if (ImGui::Combo("##PlaybackSpeed", &speedIdx, speeds, IM_ARRAYSIZE(speeds))) {
        if (speedIdx == 0) playbackSpeed_ = 0.25f;
        else if (speedIdx == 1) playbackSpeed_ = 0.5f;
        else if (speedIdx == 2) playbackSpeed_ = 1.0f;
        else if (speedIdx == 3) playbackSpeed_ = 1.5f;
        else if (speedIdx == 4) playbackSpeed_ = 2.0f;
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void GUI::drawSettingsWindow() {
    if (!showSettingsWindow_) return;

    ImGui::SetNextWindowSize(ImVec2(450.0f * dpiScale_, 400.0f * dpiScale_), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGui::Begin("Settings", &showSettingsWindow_, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("UI Preferences & Font Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputText("Search Fonts", fontFilter_, sizeof(fontFilter_));
    ImGui::Spacing();

    ImGui::Text("Available Fonts:");
    if (ImGui::BeginChild("FontListChild", ImVec2(0, 220.0f * dpiScale_), true)) {
        std::string filterStr(fontFilter_);
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        for (const auto& font : availableFonts_) {
            if (!filterStr.empty()) {
                std::string nameLower = font.displayName;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                if (nameLower.find(filterStr) == std::string::npos) continue;
            }
            bool isSelected = (font.path == currentFontPath_);
            if (ImGui::Selectable(font.displayName.c_str(), isSelected)) {
                changeActiveFont(font.path);
                saveSettings();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(-1, 32.0f * dpiScale_))) showSettingsWindow_ = false;
    ImGui::End();
}

void GUI::drawLogWindow() {
    if (showLogWindow_) {
        appLog.Draw("Application Log", &showLogWindow_);
    }
}
