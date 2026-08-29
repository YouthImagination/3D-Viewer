#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "buffer.hpp"
#include <tiny_gltf.h>

// Vertex structure with skinning support
struct Vertex {
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
    glm::vec2 texCoords{ 0.0f };
    glm::vec3 tangent{ 0.0f }; // Tangent for normal mapping
    glm::ivec4 boneIds{ -1, -1, -1, -1 };
    glm::vec4 boneWeights{ 0.0f };

    void addBoneData(int boneId, float weight) {
        for (int i = 0; i < 4; ++i) {
            if (boneIds[i] == -1) {
                boneIds[i] = boneId;
                boneWeights[i] = weight;
                return;
            }
        }
    }
};

struct BboxVertex
{
	glm::vec3 position{ 0.0f };
	glm::vec3 color{ 0.0f };
};

// Bind-pose AABB of the vertices influenced by a joint.
// Transformed per frame by the joint's bone matrix and unioned to form the animated
// model AABB in O(joints), instead of skinning every vertex on the CPU.
struct JointBounds
{
    glm::vec3 pMin{1e9f};
    glm::vec3 pMax{-1e9f};
    void extend(const glm::vec3& p) { pMin = glm::min(pMin, p); pMax = glm::max(pMax, p); }
    bool valid() const { return pMin.x <= pMax.x; }
};

class BoundingBox
{
public:
    glm::vec3 pMax = glm::vec3(-1.0f);
    glm::vec3 pMin = glm::vec3(1.0f);

    bool isValid() const {
		return extent.x > 0.0f && extent.y > 0.0f && extent.z > 0.0f;
    }
private:
    glm::vec3 extent = glm::vec3(0.0f);
};

struct SubMesh {
    unsigned int indexCount = 0;
    unsigned int indexOffset = 0; // Byte offset in index buffer
    GLuint textureID = 0;
    GLuint normalMapID = 0;
    GLuint mrMapID = 0;
};

class Model {
public:
    Model();
    ~Model();

    // Disable copy semantics
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Load functions
    bool loadFromFile(const std::string& filepath);
    bool loadFromMemory(const std::vector<char>& data);
    // Update animations
    void updateAnimation(float timeInTicks);
    
    void setAnimationIndex(unsigned int index);

    void draw(bool usemipmap);

    void drawBoundingBox();

    void updateBoundingBox();

    // Getters / properties
    const std::vector<SubMesh>& getSubMeshes() const { return subMeshes_; }
    std::vector<SubMesh>& getSubMeshes() { return subMeshes_; }
    const std::vector<bool>& getMeshVisibility() const { return meshVisibility_; }
    std::vector<bool>& getMeshVisibility() { return meshVisibility_; }

    const VertexArray& getVAO() const { return vao_; }
    const VertexBuffer& getVBO() const { return vbo_; }
    const IndexBuffer& getIBO() const { return ibo_; }
    const StorageBuffer& getSSBO() const { return ssbo_; }

    glm::vec3 getCenter() const { return modelCenter_; }
    float getScale() const { return modelScale_; }
    glm::vec3 getExtent() const { return modelExtent_; }
    float getModelSize() const { return modelSize_; }

    // Animation info wrappers for main.cpp
    bool hasAnimation() const { return !gltfModel_.animations.empty(); }
    unsigned int getAnimationCount() const { return gltfModel_.animations.size(); }
    std::string getAnimationName(unsigned int index) const {
        if (index < gltfModel_.animations.size()) {
            return gltfModel_.animations[index].name;
        }
        return "";
    }
    
    float getAnimationDurationInSeconds(unsigned int index) const {
        if (index < gltfModel_.animations.size()) {
            const auto& animation = gltfModel_.animations[index];
            float duration = 0.0f;
            for (const auto& sampler : animation.samplers) {
                int inputAccessorIdx = sampler.input;
                if (inputAccessorIdx >= 0 && inputAccessorIdx < (int)gltfModel_.accessors.size()) {
                    const auto& accessor = gltfModel_.accessors[inputAccessorIdx];
                    float maxVal = accessor.maxValues.empty() ? 0.0f : (float)accessor.maxValues[0];
                    duration = std::max(duration, maxVal);
                }
            }
            return duration;
        }
        return 0.0f;
    }
    
    float getAnimationTicksPerSecond() const { return 25.0f; }

    void destroy();

private:
    void clear();
    bool parseScene();

    static bool LoadImageCallback(tinygltf::Image* image, const int image_idx, std::string* err,
                                  std::string* warn, int req_width, int req_height,
                                  const unsigned char* bytes, int size, void* user_data);

    static bool tryDecodeKTX(tinygltf::Image* image, const int image_idx, std::string* warn,
                             const unsigned char* bytes, int size, Model* model);

    static bool tryDecodeDDS(tinygltf::Image* image, const int image_idx, std::string* warn,
                             const unsigned char* bytes, int size, Model* model);
    
    tinygltf::Model gltfModel_;
    unsigned int activeAnimationIndex_{0};
    int meshNodeIdx_{ -1 }; // Node index that owns the first mesh (for skin inverse-compensation)
    bool hasSkin_{ false }; // Cached: !gltfModel_.skins.empty()

    // mesh index -> node index that owns it (skinless rigid-node animation routes
    // each vertex's boneId to its owning node's global transform).
    std::unordered_map<int, int> meshToNode_;

    // Per-node bind-pose AABB (from POSITION accessor min/max), indexed by node index.
    // For skinless models, updateBoundingBox() transforms these by the animated node
    // global matrix to get the animated scene AABB in O(nodes).
    std::vector<glm::vec3> nodePosMin_;
    std::vector<glm::vec3> nodePosMax_;
 
    // Parsed components
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<SubMesh> subMeshes_;
    std::vector<bool> meshVisibility_;
 
    std::vector<glm::mat4> inverseBindMatrices_;
    std::vector<GLuint> gltfTextures_;
    std::unordered_map<int, std::vector<unsigned char>> ktxImageBuffers_;  // image idx -> raw KTX bytes
    std::unordered_map<int, std::vector<unsigned char>> ddsImageBuffers_;  // image idx -> raw DDS bytes

    // OpenGL Buffers
    VertexBuffer vbo_;
    IndexBuffer ibo_;
    VertexArray vao_;
    StorageBuffer ssbo_;
    glm::mat4* mappedBones_{nullptr};
    size_t ssboMatrixCount_{ 0 }; // matrices actually written (joints for skinned, nodes for skinless)

    // Cached animated node global transforms from the last updateAnimation() call,
    // used by updateBoundingBox() for the skinless path.
    std::vector<glm::mat4> lastAbsoluteTransforms_;

    VertexArray bboxVao_;
    IndexBuffer bboxIbo_;
    VertexBuffer bboxVbo_;
    std::vector<unsigned int> bboxIndices_;
    std::vector<JointBounds> jointBounds_;

    // Bounding Box
    glm::vec3 modelCenter_{0.0f};
    float modelScale_{1.0f};
    glm::vec3 modelExtent_{0.0f};
    float modelSize_{0.0f};
    BoundingBox bounds_;
};
