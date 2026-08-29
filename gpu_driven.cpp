#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <random>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Orbit Camera state
float cameraTheta = 0.0f;
float cameraPhi = 0.5f;
float cameraRadius = 8.0f;
glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);

bool mousePressed = false;
bool firstMouse = true;
double lastX = 0.0;
double lastY = 0.0;

bool cullingEnabled = true;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            firstMouse = true;
        } else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mousePressed) return;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.005f;
    cameraTheta -= (float)xoffset * sensitivity;
    cameraPhi   -= (float)yoffset * sensitivity;

    constexpr float limit = glm::radians(89.0f);
    if (cameraPhi > limit) cameraPhi = limit;
    if (cameraPhi < -limit) cameraPhi = -limit;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    cameraRadius -= (float)yoffset * 0.5f;
    if (cameraRadius < 2.0f) cameraRadius = 2.0f;
    if (cameraRadius > 40.0f) cameraRadius = 40.0f;
}

// Structures matching shaders
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct MeshletInfo {
    unsigned int indexOffset;
    unsigned int indexCount;
    int  vertexOffset;
    unsigned int pad;
};

struct MeshletBounds {
    glm::vec3 center;
    float radius;
};

struct DrawElementsIndirectCommand {
    unsigned int count;
    unsigned int instanceCount;
    unsigned int firstIndex;
    int  baseVertex;
    unsigned int baseInstance;
};

struct OBJFace {
    unsigned int v[3];
    unsigned int vn[3];
    bool hasNormals;
};

bool loadOBJCustomFast(const std::string& filepath, std::vector<glm::vec3>& out_positions, std::vector<glm::vec3>& out_normals, std::vector<OBJFace>& out_faces) {
    FILE* file = nullptr;
#ifdef _WIN32
    if (fopen_s(&file, filepath.c_str(), "r") != 0 || !file) {
#else
    file = fopen(filepath.c_str(), "r");
    if (!file) {
#endif
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    // Pre-reserve estimate based on 20M model sizes
    out_positions.reserve(12000000);
    out_faces.reserve(22000000);

    std::vector<glm::vec3> temp_normals;
    temp_normals.reserve(12000000);

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        if (*ptr == '\0' || *ptr == '#' || *ptr == '\n' || *ptr == '\r') continue;

        if (ptr[0] == 'v' && ptr[1] == ' ') {
            float x, y, z;
            if (sscanf(ptr + 2, "%f %f %f", &x, &y, &z) == 3) {
                out_positions.push_back(glm::vec3(x, y, z));
            }
        } else if (ptr[0] == 'v' && ptr[1] == 'n' && ptr[2] == ' ') {
            float x, y, z;
            if (sscanf(ptr + 3, "%f %f %f", &x, &y, &z) == 3) {
                temp_normals.push_back(glm::vec3(x, y, z));
            }
        } else if (ptr[0] == 'f' && ptr[1] == ' ') {
            OBJFace face;
            face.hasNormals = false;
            char* fPtr = ptr + 2;
            int vertexCount = 0;
            
            while (*fPtr && vertexCount < 3) {
                while (*fPtr == ' ' || *fPtr == '\t') fPtr++;
                if (*fPtr == '\0' || *fPtr == '\n' || *fPtr == '\r') break;

                int v = 0, vt = 0, vn = 0;
                // Parse v/vt/vn or v//vn or v/vt or v
                if (sscanf(fPtr, "%d/%d/%d", &v, &vt, &vn) == 3) {
                    face.v[vertexCount] = v - 1;
                    face.vn[vertexCount] = vn - 1;
                    face.hasNormals = true;
                } else if (sscanf(fPtr, "%d//%d", &v, &vn) == 2) {
                    face.v[vertexCount] = v - 1;
                    face.vn[vertexCount] = vn - 1;
                    face.hasNormals = true;
                } else if (sscanf(fPtr, "%d/%d", &v, &vt) == 2) {
                    face.v[vertexCount] = v - 1;
                    face.vn[vertexCount] = 0;
                } else if (sscanf(fPtr, "%d", &v) == 1) {
                    face.v[vertexCount] = v - 1;
                    face.vn[vertexCount] = 0;
                }
                
                vertexCount++;
                while (*fPtr && *fPtr != ' ' && *fPtr != '\t') fPtr++;
            }
            if (vertexCount == 3) {
                out_faces.push_back(face);
            }
        }
    }
    fclose(file);

    out_normals.resize(out_positions.size(), glm::vec3(0.0f));
    if (!temp_normals.empty()) {
        for (const auto& face : out_faces) {
            if (face.hasNormals) {
                for (int i = 0; i < 3; ++i) {
                    if (face.v[i] < out_normals.size() && face.vn[i] < temp_normals.size()) {
                        out_normals[face.v[i]] = temp_normals[face.vn[i]];
                    }
                }
            }
        }
    } else {
        std::cout << "OBJ has no normals. Generating smooth normals on the CPU..." << std::endl;
        // Accumulate face normals into vertices (weighted by face area via cross product magnitude)
        for (const auto& face : out_faces) {
            if (face.v[0] < out_positions.size() && face.v[1] < out_positions.size() && face.v[2] < out_positions.size()) {
                glm::vec3 p0 = out_positions[face.v[0]];
                glm::vec3 p1 = out_positions[face.v[1]];
                glm::vec3 p2 = out_positions[face.v[2]];
                
                glm::vec3 edge1 = p1 - p0;
                glm::vec3 edge2 = p2 - p0;
                glm::vec3 faceNormal = glm::cross(edge1, edge2);
                
                out_normals[face.v[0]] += faceNormal;
                out_normals[face.v[1]] += faceNormal;
                out_normals[face.v[2]] += faceNormal;
            }
        }
        
        // Normalize the accumulated normals
        for (auto& n : out_normals) {
            float len = glm::length(n);
            if (len > 0.0001f) {
                n /= len;
            } else {
                n = glm::vec3(0.0f, 1.0f, 0.0f); // Default normal
            }
        }
        std::cout << "Normal generation complete." << std::endl;
    }

    std::cout << "Successfully parsed custom OBJ file (fast mode)." << std::endl;
    std::cout << "Positions: " << out_positions.size() << ", Faces: " << out_faces.size() << std::endl;
    return true;
}

// Shader loading helpers (SPIR-V)
std::vector<char> readBinaryFile(const std::string& filename) {
    std::vector<std::string> searchPaths = {
        filename,
        "../" + filename,
        "../../" + filename,
        "build/" + filename,
        "../build/" + filename
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
                std::cout << "Successfully read SPIR-V shader: " << path << std::endl;
                return buffer;
            }
        }
    }
    throw std::runtime_error("failed to open SPIR-V file: " + filename);
}

GLuint loadSPIRVShader(GLenum shaderType, const std::string& filename) {
    std::vector<char> spirvCode = readBinaryFile(filename);
    GLuint shader = glCreateShader(shaderType);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, spirvCode.data(), (GLsizei)spirvCode.size());
    glSpecializeShader(shader, "main", 0, nullptr, nullptr);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "SPIR-V Shader (" << filename << ") specialization failed:\n" << infoLog << std::endl;
    }
    return shader;
}

// Compute Frustum planes from VP matrix
std::vector<glm::vec4> getFrustumPlanes(const glm::mat4& vp) {
    std::vector<glm::vec4> planes(6);
    // Left
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
    // Right
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
    // Bottom
    planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
    // Top
    planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
    // Near
    planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
    // Far
    planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(planes[i]));
        planes[i] /= length;
    }
    return planes;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    GLFWwindow* window = glfwCreateWindow(800, 600, "GPU-Driven Multi-Draw Indirect (MDI) Renderer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshletInfo> meshletInfos;
    std::vector<MeshletBounds> meshletBounds;
    unsigned int TOTAL_MESHLETS = 0;

    try {
        // 1. Load Triceratops Model using custom fast loader
        std::vector<glm::vec3> objPositions;
        std::vector<glm::vec3> objNormals;
        std::vector<OBJFace> objFaces;
        
        if (!loadOBJCustomFast("D:/trex_master_models/models/TRex_30M_part03.OBJ", objPositions, objNormals, objFaces)) {
            std::cerr << "Failed to load OBJ model via custom loader" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // Calculate scene bounding box for auto-centering and scaling
        glm::vec3 globalMin(1e9f), globalMax(-1e9f);
        for (const auto& p : objPositions) {
            globalMin = glm::min(globalMin, p);
            globalMax = glm::max(globalMax, p);
        }

        glm::vec3 center = (globalMin + globalMax) * 0.5f;
        glm::vec3 size = globalMax - globalMin;
        float maxDim = std::max({size.x, size.y, size.z});
        float normalizeScale = (maxDim > 0.0001f) ? (10.0f / maxDim) : 1.0f;

        std::cout << "Bounding Box Min: (" << globalMin.x << ", " << globalMin.y << ", " << globalMin.z << ")" << std::endl;
        std::cout << "Bounding Box Max: (" << globalMax.x << ", " << globalMax.y << ", " << globalMax.z << ")" << std::endl;
        std::cout << "Scale factor: " << normalizeScale << " | Centering at (0,0,0)" << std::endl;

        // Greedy Meshlet Builder
        constexpr unsigned int MAX_VERTICES = 64;
        constexpr unsigned int MAX_INDICES = 372; // 124 triangles

        std::unordered_map<unsigned int, unsigned int> localVertexMap;
        std::vector<Vertex> meshletVertices;
        std::vector<unsigned int> meshletIndices;

        for (const auto& face : objFaces) {
            // Check how many new vertices this face would introduce
            int newVerticesCount = 0;
            for (int i = 0; i < 3; ++i) {
                if (localVertexMap.find(face.v[i]) == localVertexMap.end()) {
                    newVerticesCount++;
                }
            }

            // If we exceed thresholds, finalize the current meshlet
            if (meshletVertices.size() + newVerticesCount > MAX_VERTICES || meshletIndices.size() + 3 > MAX_INDICES) {
                MeshletInfo info;
                info.vertexOffset = (int)vertices.size();
                info.indexOffset = (unsigned int)indices.size();
                info.indexCount = (unsigned int)meshletIndices.size();
                info.pad = 0;
                meshletInfos.push_back(info);

                MeshletBounds bounds;
                glm::vec3 minCoord(1e9f), maxCoord(-1e9f);
                for (const auto& v : meshletVertices) {
                    minCoord = glm::min(minCoord, v.position);
                    maxCoord = glm::max(maxCoord, v.position);
                    vertices.push_back(v);
                }
                bounds.center = (minCoord + maxCoord) * 0.5f;
                float maxDistSq = 0.0f;
                for (const auto& v : meshletVertices) {
                    float dist = glm::distance(v.position, bounds.center);
                    float distSq = dist * dist;
                    if (distSq > maxDistSq) maxDistSq = distSq;
                }
                bounds.radius = std::sqrt(maxDistSq);
                meshletBounds.push_back(bounds);

                for (unsigned int idx : meshletIndices) {
                    indices.push_back(idx);
                }

                localVertexMap.clear();
                meshletVertices.clear();
                meshletIndices.clear();
            }

            // Insert vertices and indices
            unsigned int localIndices[3];
            for (int i = 0; i < 3; ++i) {
                unsigned int globalIndex = face.v[i];
                auto it = localVertexMap.find(globalIndex);
                if (it == localVertexMap.end()) {
                    unsigned int localIndex = (unsigned int)meshletVertices.size();
                    localVertexMap[globalIndex] = localIndex;
                    localIndices[i] = localIndex;

                    Vertex v;
                    glm::vec3 p = objPositions[globalIndex];
                    v.position = (p - center) * normalizeScale;
                    v.normal = objNormals[globalIndex];
                    meshletVertices.push_back(v);
                } else {
                    localIndices[i] = it->second;
                }
            }

            meshletIndices.push_back(localIndices[0]);
            meshletIndices.push_back(localIndices[1]);
            meshletIndices.push_back(localIndices[2]);
        }

        // Finalize last meshlet
        if (!meshletVertices.empty()) {
            MeshletInfo info;
            info.vertexOffset = (int)vertices.size();
            info.indexOffset = (unsigned int)indices.size();
            info.indexCount = (unsigned int)meshletIndices.size();
            info.pad = 0;
            meshletInfos.push_back(info);

            MeshletBounds bounds;
            glm::vec3 minCoord(1e9f), maxCoord(-1e9f);
            for (const auto& v : meshletVertices) {
                minCoord = glm::min(minCoord, v.position);
                maxCoord = glm::max(maxCoord, v.position);
                vertices.push_back(v);
            }
            bounds.center = (minCoord + maxCoord) * 0.5f;
            float maxDistSq = 0.0f;
            for (const auto& v : meshletVertices) {
                float dist = glm::distance(v.position, bounds.center);
                float distSq = dist * dist;
                if (distSq > maxDistSq) maxDistSq = distSq;
            }
            bounds.radius = std::sqrt(maxDistSq);
            meshletBounds.push_back(bounds);

            for (unsigned int idx : meshletIndices) {
                indices.push_back(idx);
            }
        }

        TOTAL_MESHLETS = (unsigned int)meshletInfos.size();
        std::cout << "Model partitioned into meshlets successfully." << std::endl;
        std::cout << "Total Meshlets: " << TOTAL_MESHLETS << std::endl;
        std::cout << "Total Triangles: " << indices.size() / 3 << " (approx 20 Million)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred during model load/meshlet build: " << e.what() << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception occurred during model load/meshlet build!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 2. Load Shaders
    GLuint cullShader = loadSPIRVShader(GL_COMPUTE_SHADER, "cull_comp.spv");
    GLuint cullProgram = glCreateProgram();
    glAttachShader(cullProgram, cullShader);
    glLinkProgram(cullProgram);
    glDeleteShader(cullShader);

    GLuint vs = loadSPIRVShader(GL_VERTEX_SHADER, "meshlet_vert.spv");
    GLuint fs = loadSPIRVShader(GL_FRAGMENT_SHADER, "meshlet_frag.spv");
    GLuint drawProgram = glCreateProgram();
    glAttachShader(drawProgram, vs);
    glAttachShader(drawProgram, fs);
    glLinkProgram(drawProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 3. Create GPU Buffers using DSA
    GLuint vbo, ibo;
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(vbo, vertices.size() * sizeof(Vertex), vertices.data(), 0);
    glNamedBufferStorage(ibo, indices.size() * sizeof(unsigned int), indices.data(), 0);

    // VAO setup
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, ibo);

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(vao, 1, 0);

    // SSBO for Meshlet Info
    GLuint meshletInfoSSBO;
    glCreateBuffers(1, &meshletInfoSSBO);
    glNamedBufferStorage(meshletInfoSSBO, meshletInfos.size() * sizeof(MeshletInfo), meshletInfos.data(), 0);

    // SSBO for Meshlet Bounds
    GLuint meshletBoundsSSBO;
    glCreateBuffers(1, &meshletBoundsSSBO);
    glNamedBufferStorage(meshletBoundsSSBO, meshletBounds.size() * sizeof(MeshletBounds), meshletBounds.data(), 0);

    // SSBO for Output Draw Indirect Commands
    GLuint drawCommandBuffer;
    glCreateBuffers(1, &drawCommandBuffer);
    // Allocate space for max possible meshlets
    glNamedBufferStorage(drawCommandBuffer, TOTAL_MESHLETS * sizeof(DrawElementsIndirectCommand), nullptr, 0);

    // SSBO for Global Atomic Counter (uint)
    GLuint counterBuffer;
    glCreateBuffers(1, &counterBuffer);
    glNamedBufferStorage(counterBuffer, sizeof(unsigned int), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // Enable MSAA

    unsigned int zeroCounter = 0;

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Settings Window
        unsigned int drawnMeshletsCount = 0;
        {
            ImGui::Begin("GPU-Driven MDI Stats");
            ImGui::Checkbox("Enable GPU Frustum Culling", &cullingEnabled);
            ImGui::Text("Total Meshlets: %d", TOTAL_MESHLETS);
            ImGui::Text("Total Geometry Triangles: %zu", indices.size() / 3);
            ImGui::End();
        }

        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute Camera matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::vec3 camPos;
        camPos.x = cameraTarget.x + cameraRadius * std::cos(cameraPhi) * std::sin(cameraTheta);
        camPos.y = cameraTarget.y + cameraRadius * std::sin(cameraPhi);
        camPos.z = cameraTarget.z + cameraRadius * std::cos(cameraPhi) * std::cos(cameraTheta);
        glm::mat4 view = glm::lookAt(camPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 vp = projection * view;

        // Step 1: Clear the atomic counter buffer to 0 on the GPU
        glClearNamedBufferData(counterBuffer, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zeroCounter);
        // Ensure the clear completes before compute shader starts atomic additions
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // Step 2: Bind buffers and Dispatch Compute shader for Culling
        glUseProgram(cullProgram);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshletInfoSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshletBoundsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCommandBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, counterBuffer);

        glUniform1ui(0, TOTAL_MESHLETS);
        glUniform1ui(1, cullingEnabled ? 1 : 0);
        std::vector<glm::vec4> planes = getFrustumPlanes(vp);
        glUniform4fv(2, 6, glm::value_ptr(planes[0]));

        // Dispatch compute (group size 64)
        glDispatchCompute((TOTAL_MESHLETS + 63) / 64, 1, 1);

        // Ensure compute shader writes finish before indirect draw reads
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // Read back the visible meshlets count from the counter buffer for ImGui stats
        glGetNamedBufferSubData(counterBuffer, 0, sizeof(unsigned int), &drawnMeshletsCount);

        // Show live stats in the ImGui Window
        {
            ImGui::Begin("GPU-Driven MDI Stats");
            ImGui::Text("Visible Meshlets (Drawn): %d", drawnMeshletsCount);
            ImGui::Text("Culled Meshlets: %d", TOTAL_MESHLETS - drawnMeshletsCount);
            ImGui::Text("Estimated Rendered Triangles: ~%d", drawnMeshletsCount * 120);
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Step 3: Draw using Multi-Draw Indirect (MDI)
        glUseProgram(drawProgram);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(vp));
        glUniform3fv(1, 1, glm::value_ptr(camPos));
        glUniform3f(2, -0.5f, -1.0f, -0.5f);

        glBindVertexArray(vao);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer);
        
        // Multi-draw command directly using the GPU commands buffer count
        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            drawnMeshletsCount, // We use the read-back count to drive MDI
            sizeof(DrawElementsIndirectCommand)
        );

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &meshletInfoSSBO);
    glDeleteBuffers(1, &meshletBoundsSSBO);
    glDeleteBuffers(1, &drawCommandBuffer);
    glDeleteBuffers(1, &counterBuffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(cullProgram);
    glDeleteProgram(drawProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
