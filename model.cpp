#include "model.h"
#include "logger.h"
#include "utils.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <GL/glx.h>
#endif

// Define tinygltf implementation macros
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>
#include <stb_image.h>
#include <ktx.h>

namespace tinygltf {
    bool LoadImageData(Image *image, const int image_idx, std::string *err,
                       std::string *warn, int req_width, int req_height,
                       const unsigned char *bytes, int size, void *user_data) {
        return false;
    }
    
    bool WriteImageData(const std::string *basepath, const std::string *filename,
                        const Image *image, bool embedImages,
                        const FsCallbacks *fs, const URICallbacks *uri,
                        std::string *out_uri, void *user_data) {
        return false;
    }
}

struct NodeTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::mat4 matrix{1.0f};
    bool useMatrix{false};
};

namespace {
    // GL proc loader for ktxTexture_GLUpload.
    // wglGetProcAddress only resolves extension functions; core functions in
    // opengl32.dll must be resolved via GetProcAddress. ktxLoadOpenGL needs both.
    PFNVOIDFUNCTION KTX_APIENTRY ktxGlGetProc(const char* proc) {
#ifdef _WIN32
        void* p = (void*)wglGetProcAddress(proc);
        if (p == nullptr || p == (void*)1 || p == (void*)2 ||
            p == (void*)3 || p == (void*)-1) {
            HMODULE mod = GetModuleHandleA("opengl32.dll");
            if (!mod) mod = LoadLibraryA("opengl32.dll");
            if (mod) p = (void*)GetProcAddress(mod, proc);
        }
        return (PFNVOIDFUNCTION)p;
#else
        return (PFNVOIDFUNCTION)glXGetProcAddressARB((const GLubyte*)proc);
#endif
    }

    // Load OpenGL function pointers needed by ktxTexture_GLUpload (once).
    void ensureKtxGLLoaded() {
        static bool loaded = false;
        if (loaded) return;
        loaded = true;
        KTX_error_code kc = ktxLoadOpenGL(ktxGlGetProc);
        if (kc != KTX_SUCCESS) {
            LOG_ERROR << "ktxLoadOpenGL failed: " << ktxErrorString(kc) << std::endl;
        }
    }

    // Helper to get accessor data pointer
    template<typename T>
    const T* getAccessorData(const tinygltf::Model& model, int accessorIdx, int& stride) {
        if (accessorIdx < 0 || accessorIdx >= (int)model.accessors.size()) return nullptr;
        const auto& accessor = model.accessors[accessorIdx];
        if (accessor.bufferView < 0 || accessor.bufferView >= (int)model.bufferViews.size()) return nullptr;
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        stride = accessor.ByteStride(bufferView);
        return reinterpret_cast<const T*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    }

    struct DDS_PIXELFORMAT {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    };

    struct DDS_HEADER {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipMapCount;
        uint32_t dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t dwCaps;
        uint32_t dwCaps2;
        uint32_t dwCaps3;
        uint32_t dwCaps4;
        uint32_t dwReserved2;
    };

    // Upload a DDS container (DXT1/3/5 via FourCC, or BC4-BC7 via DX10 header).
    // Returns the number of mip levels uploaded, or 0 on failure.
    int uploadDDSTexture(const unsigned char* bytes, int size, GLuint& textureID, bool sRGB) {
        if (size < 128) return 0;
        if (*reinterpret_cast<const uint32_t*>(bytes) != 0x20534444) return 0;  // "DDS "

        const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>(bytes + 4);
        if (header->dwSize != 124 || header->ddspf.dwSize != 32) return 0;

        GLenum format = 0;
        int blockSize = 16;
        int offset = 128;  // magic (4) + DDS_HEADER (124)

        if (header->ddspf.dwFlags & 0x4) {  // DDPF_FOURCC
            uint32_t fourCC = header->ddspf.dwFourCC;
            if (fourCC == 0x31545844) {  // "DXT1"
                format = sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
                              : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                blockSize = 8;
            } else if (fourCC == 0x33545844) {  // "DXT3"
                format = sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
                              : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                blockSize = 16;
            } else if (fourCC == 0x35545844) {  // "DXT5"
                format = sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
                              : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                blockSize = 16;
            } else if (fourCC == 0x30315844) {  // "DX10"
                if (size < 148) return 0;
                // DDS_HEADER_DXT10: dxgiFormat, resourceDimension, miscFlag, arraySize, miscFlags2
                const uint32_t* dx10 = reinterpret_cast<const uint32_t*>(bytes + 128);
                uint32_t dxgiFormat = dx10[0];
                if (dx10[3] > 1) {
                    LOG_ERROR << "DDS array textures are not supported" << std::endl;
                    return 0;
                }
                offset = 148;
                switch (dxgiFormat) {
                    case 71: format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;         blockSize = 8;  break;  // BC1_UNORM
                    case 72: format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;   blockSize = 8;  break;  // BC1_UNORM_SRGB
                    case 74: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;                            break;  // BC2_UNORM
                    case 75: format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;                      break;  // BC2_UNORM_SRGB
                    case 77: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;                            break;  // BC3_UNORM
                    case 78: format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;                      break;  // BC3_UNORM_SRGB
                    case 80: format = GL_COMPRESSED_RED_RGTC1;                  blockSize = 8;  break;  // BC4_UNORM
                    case 81: format = GL_COMPRESSED_SIGNED_RED_RGTC1;           blockSize = 8;  break;  // BC4_SNORM
                    case 83: format = GL_COMPRESSED_RG_RGTC2;                                       break;  // BC5_UNORM
                    case 84: format = GL_COMPRESSED_SIGNED_RG_RGTC2;                                break;  // BC5_SNORM
                    case 95: format = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;                        break;  // BC6H_UF16
                    case 96: format = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;                          break;  // BC6H_SF16
                    case 98: format = GL_COMPRESSED_RGBA_BPTC_UNORM;                                break;  // BC7_UNORM
                    case 99: format = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;                          break;  // BC7_UNORM_SRGB
                    default:
                        LOG_ERROR << "Unsupported DXGI format: " << dxgiFormat << std::endl;
                        return 0;
                }
            } else {
                LOG_ERROR << "Unsupported DDS FourCC format: 0x" << std::hex << fourCC << std::dec << std::endl;
                return 0;
            }
        } else {
            LOG_ERROR << "Uncompressed DDS formats are not supported" << std::endl;
            return 0;
        }

        uint32_t width = header->dwWidth;
        uint32_t height = header->dwHeight;
        uint32_t mipmaps = (header->dwFlags & 0x20000) ? header->dwMipMapCount : 1;  // DDSD_MIPMAPCOUNT
        if (mipmaps == 0) mipmaps = 1;

        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
        glTextureStorage2D(textureID, mipmaps, format, width, height);

        for (uint32_t level = 0; level < mipmaps; ++level) {
            uint32_t mipWidth = std::max(1u, width >> level);
            uint32_t mipHeight = std::max(1u, height >> level);
            uint32_t mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;

            if (offset + (int)mipSize > size) {
                LOG_ERROR << "DDS file truncated at level " << level << std::endl;
                glDeleteTextures(1, &textureID);
                textureID = 0;
                return 0;
            }

            glCompressedTextureSubImage2D(textureID, level, 0, 0, mipWidth, mipHeight,
                                          format, mipSize, bytes + offset);
            offset += mipSize;
        }

        return (int)mipmaps;
    }
} // namespace

// Try decoding image data as KTX/KTX2 (incl. BasisU supercompression).
// Save raw KTX bytes for later GLUpload in parseScene().
// Returns true if the data is a KTX container (we stash the raw bytes and
// set placeholder metadata; actual GL texture creation happens in parseScene).
bool Model::tryDecodeKTX(tinygltf::Image *image, const int image_idx, std::string *warn,
                         const unsigned char *bytes, int size, Model* model) {
        ktxTexture* ktxTex = nullptr;
        KTX_error_code kc = ktxTexture_CreateFromMemory(
            bytes, size, KTX_TEXTURE_CREATE_NO_FLAGS, &ktxTex);
        if (kc != KTX_SUCCESS || !ktxTex) {
            return false;  // Not a KTX container; let stb_image try.
        }

        // Stash the raw KTX bytes; parseScene() will call ktxTexture_GLUpload.
        if (model) {
            model->ktxImageBuffers_[image_idx].assign(bytes, bytes + size);
        }
        image->width = ktxTex->baseWidth;
        image->height = ktxTex->baseHeight;
        image->component = 4;
        image->bits = -1;  // sentinel: this is a KTX image, not decoded pixels
        image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
        LOG_INFO << "Image " << image_idx << " identified as KTX ("
                 << ktxTex->baseWidth << "x" << ktxTex->baseHeight << "); will GLUpload" << std::endl;
        ktxTexture_Destroy(ktxTex);
        return true;
    }

bool Model::tryDecodeDDS(tinygltf::Image *image, const int image_idx, std::string *warn,
                         const unsigned char *bytes, int size, Model* model) {
    if (size < 128) return false;
    if (*reinterpret_cast<const uint32_t*>(bytes) != 0x20534444) return false;
    
    const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>(bytes + 4);
    if (header->dwSize != 124 || header->ddspf.dwSize != 32) return false;
    
    if (model) {
        model->ddsImageBuffers_[image_idx].assign(bytes, bytes + size);
    }
    image->width = header->dwWidth;
    image->height = header->dwHeight;
    image->component = 4;
    image->bits = -2;  // sentinel: this is a DDS image, not decoded pixels
    image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    LOG_INFO << "Image " << image_idx << " identified as DDS ("
             << header->dwWidth << "x" << header->dwHeight << "); will GLUpload" << std::endl;
    return true;
}

// Custom image loader callback: KTX/KTX2 first, then stb_image, then placeholder.
bool Model::LoadImageCallback(tinygltf::Image *image, const int image_idx, std::string *err,
                              std::string *warn, int req_width, int req_height,
                              const unsigned char *bytes, int size, void *user_data) {
        Model* model = static_cast<Model*>(user_data);

        // 1. Try KTX/KTX2 (handles BasisU, UASTC, ETC, BCn in KTX containers)
        if (tryDecodeKTX(image, image_idx, warn, bytes, size, model)) {
            return true;
        }

        // 1.5 Try DDS (handles DXT1, DXT3, DXT5)
        if (tryDecodeDDS(image, image_idx, warn, bytes, size, model)) {
            return true;
        }

        // 2. Fall back to stb_image for PNG/JPEG/etc.
        int width, height, component;
        unsigned char *data = stbi_load_from_memory(bytes, size, &width, &height, &component, 4);
        if (data) {
            image->width = width;
            image->height = height;
            image->component = 4;
            image->bits = 8;
            image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
            image->image.resize(width * height * 4);
            std::copy(data, data + width * height * 4, image->image.begin());
            stbi_image_free(data);
            return true;
        }

        // 3. Both failed: use a 1x1 white placeholder so the model still renders.
        const char* reason = stbi_failure_reason();
        if (warn) {
            *warn = std::string("Image ") + std::to_string(image_idx) +
                    " decode failed (" + (reason ? reason : "unknown") +
                    "); using placeholder.";
        }
        image->width = 1;
        image->height = 1;
        image->component = 4;
        image->bits = 8;
        image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
        image->image.assign(4, 255);
        return true;
}

namespace {
    // Animation interpolation helper functions
    glm::vec3 interpolateTranslation(const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler, float time) {
        int inputStride = 0, outputStride = 0;
        const float* times = getAccessorData<float>(model, sampler.input, inputStride);
        const float* values = getAccessorData<float>(model, sampler.output, outputStride);
        int count = model.accessors[sampler.input].count;
        
        if (count == 0 || !times || !values) return glm::vec3(0.0f);
        if (count == 1 || time <= times[0]) {
            return glm::vec3(values[0], values[1], values[2]);
        }
        if (time >= times[count - 1]) {
            int idx = count - 1;
            return glm::vec3(values[idx * 3], values[idx * 3 + 1], values[idx * 3 + 2]);
        }
        
        int idx = 0;
        for (int i = 0; i < count - 1; ++i) {
            if (time < times[i + 1]) {
                idx = i;
                break;
            }
        }
        
        float t1 = times[idx];
        float t2 = times[idx + 1];
        float factor = glm::clamp((time - t1) / (t2 - t1), 0.0f, 1.0f);
        
        glm::vec3 v1(values[idx * 3], values[idx * 3 + 1], values[idx * 3 + 2]);
        glm::vec3 v2(values[(idx + 1) * 3], values[(idx + 1) * 3 + 1], values[(idx + 1) * 3 + 2]);
        return glm::mix(v1, v2, factor);
    }
    
    glm::vec3 interpolateScale(const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler, float time) {
        int inputStride = 0, outputStride = 0;
        const float* times = getAccessorData<float>(model, sampler.input, inputStride);
        const float* values = getAccessorData<float>(model, sampler.output, outputStride);
        int count = model.accessors[sampler.input].count;
        
        if (count == 0 || !times || !values) return glm::vec3(1.0f);
        if (count == 1 || time <= times[0]) {
            return glm::vec3(values[0], values[1], values[2]);
        }
        if (time >= times[count - 1]) {
            int idx = count - 1;
            return glm::vec3(values[idx * 3], values[idx * 3 + 1], values[idx * 3 + 2]);
        }
        
        int idx = 0;
        for (int i = 0; i < count - 1; ++i) {
            if (time < times[i + 1]) {
                idx = i;
                break;
            }
        }
        
        float t1 = times[idx];
        float t2 = times[idx + 1];
        float factor = glm::clamp((time - t1) / (t2 - t1), 0.0f, 1.0f);
        
        glm::vec3 v1(values[idx * 3], values[idx * 3 + 1], values[idx * 3 + 2]);
        glm::vec3 v2(values[(idx + 1) * 3], values[(idx + 1) * 3 + 1], values[(idx + 1) * 3 + 2]);
        return glm::mix(v1, v2, factor);
    }
    
    glm::quat interpolateRotation(const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler, float time) {
        int inputStride = 0, outputStride = 0;
        const float* times = getAccessorData<float>(model, sampler.input, inputStride);
        const float* values = getAccessorData<float>(model, sampler.output, outputStride);
        int count = model.accessors[sampler.input].count;
        
        if (count == 0 || !times || !values) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (count == 1 || time <= times[0]) {
            return glm::quat(values[3], values[0], values[1], values[2]); // glTF: x,y,z,w -> glm: w,x,y,z
        }
        if (time >= times[count - 1]) {
            int idx = count - 1;
            return glm::quat(values[idx * 4 + 3], values[idx * 4], values[idx * 4 + 1], values[idx * 4 + 2]);
        }
        
        int idx = 0;
        for (int i = 0; i < count - 1; ++i) {
            if (time < times[i + 1]) {
                idx = i;
                break;
            }
        }
        
        float t1 = times[idx];
        float t2 = times[idx + 1];
        float factor = glm::clamp((time - t1) / (t2 - t1), 0.0f, 1.0f);
        
        glm::quat q1(values[idx * 4 + 3], values[idx * 4], values[idx * 4 + 1], values[idx * 4 + 2]);
        glm::quat q2(values[(idx + 1) * 4 + 3], values[(idx + 1) * 4], values[(idx + 1) * 4 + 1], values[(idx + 1) * 4 + 2]);
        return glm::slerp(q1, q2, factor);
    }
}

Model::Model() {}

Model::~Model() {
    destroy();
}

void Model::clear() {
    for (auto tex : gltfTextures_) {
        if (tex > 0) glDeleteTextures(1, &tex);
    }
    gltfTextures_.clear();
    ktxImageBuffers_.clear();
    ddsImageBuffers_.clear();

    subMeshes_.clear();
    meshVisibility_.clear();
    vertices_.clear();
    indices_.clear();
    inverseBindMatrices_.clear();

    vbo_.destroy();
    ibo_.destroy();
    vao_.destroy();
    ssbo_.destroy();
    mappedBones_ = nullptr;

    bboxVao_.destroy();
    bboxVbo_.destroy();
    bboxIbo_.destroy();
}

void Model::destroy() {
    clear();
}

bool Model::loadFromFile(const std::string& filepath) {
    clear();
    ensureKtxGLLoaded();
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(Model::LoadImageCallback, this);
    std::string err;
    std::string warn;

    LOG_INFO << "Loading model: " << filepath << std::endl;

    if (!std::filesystem::exists(filepath)) {
        LOG_ERROR << "File does not exist: " << filepath << std::endl;
        return false;
    }

    bool ret = false;
    // Check extension (case-insensitive)
    std::string ext = filepath.substr(filepath.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == "glb") {
        ret = loader.LoadBinaryFromFile(&gltfModel_, &err, &warn, filepath);
    } else {
        ret = loader.LoadASCIIFromFile(&gltfModel_, &err, &warn, filepath);
    }

    if (!warn.empty()) {
        LOG_WARN << "tinygltf warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        LOG_ERROR << "tinygltf error: " << err << std::endl;
    }
    if (!ret) {
        LOG_ERROR << "Failed to parse glTF file: " << filepath << std::endl;
        return false;
    }

    bool parseOk = parseScene();
    if (parseOk) {
        LOG_INFO << "Model loaded OK: " << filepath
                 << " (" << vertices_.size() << " verts, " << indices_.size() << " indices, "
                 << subMeshes_.size() << " submeshes)" << std::endl;
    } else {
        LOG_ERROR << "parseScene() failed for: " << filepath << std::endl;
    }
    return parseOk;
}

bool Model::loadFromMemory(const std::vector<char>& data) {
    clear();
    ensureKtxGLLoaded();
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(Model::LoadImageCallback, this);
    std::string err;
    std::string warn;
    
    bool ret = loader.LoadBinaryFromMemory(&gltfModel_, &err, &warn, 
        reinterpret_cast<const unsigned char*>(data.data()), data.size());
        
    if (!warn.empty()) {
        LOG_INFO << "tinygltf warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        LOG_ERROR << "tinygltf error: " << err << std::endl;
    }
    if (!ret) {
        return false;
    }
    return parseScene();
}

bool Model::parseScene() {
    // 1. Load textures
    std::vector<bool> isSRGB(gltfModel_.textures.size(), false);
    for (const auto& mat : gltfModel_.materials) {
        int baseColorTexIdx = mat.pbrMetallicRoughness.baseColorTexture.index;
        if (baseColorTexIdx < 0) {
            auto extIt = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (extIt != mat.extensions.end() && extIt->second.IsObject()) {
                const auto& diffuseInfo = extIt->second.Get("diffuseTexture");
                if (diffuseInfo.IsObject()) {
                    const auto& idxVal = diffuseInfo.Get("index");
                    if (idxVal.IsNumber()) baseColorTexIdx = idxVal.GetNumberAsInt();
                }
            }
        }
        if (baseColorTexIdx >= 0 && baseColorTexIdx < (int)isSRGB.size()) {
            isSRGB[baseColorTexIdx] = true;
        }
        int emissiveTexIdx = mat.emissiveTexture.index;
        if (emissiveTexIdx >= 0 && emissiveTexIdx < (int)isSRGB.size()) {
            isSRGB[emissiveTexIdx] = true;
        }
    }

    gltfTextures_.resize(gltfModel_.textures.size(), 0);
    for (size_t i = 0; i < gltfModel_.textures.size(); ++i) {
        const auto& gltfTex = gltfModel_.textures[i];
        
        int sourceImageIdx = gltfTex.source;

        // Prefer KHR_texture_basisu (KTX2) if present.
        auto extIt = gltfTex.extensions.find("KHR_texture_basisu");
        if (extIt != gltfTex.extensions.end() && extIt->second.IsObject() &&
            extIt->second.Has("source")) {
            sourceImageIdx = extIt->second.Get("source").GetNumberAsInt();
        }
        // MSFT_texture_dds (DDS) — many exporters store the PNG in `source` and
        // the compressed DDS variant in this extension; prefer the DDS image.
        auto ddsExtIt = gltfTex.extensions.find("MSFT_texture_dds");
        if (ddsExtIt != gltfTex.extensions.end() && ddsExtIt->second.IsObject() &&
            ddsExtIt->second.Has("source")) {
            sourceImageIdx = ddsExtIt->second.Get("source").GetNumberAsInt();
        }

        if (sourceImageIdx < 0 || sourceImageIdx >= (int)gltfModel_.images.size()) continue;
        const auto& gltfImg = gltfModel_.images[sourceImageIdx];

        GLuint textureID = 0;

        // KTX images: let the KTX library create & upload the GL texture directly.
        // It handles format detection, BasisU/UASTC transcoding, mip chains, and row order.
        auto ktxIt = ktxImageBuffers_.find(sourceImageIdx);
        if (ktxIt != ktxImageBuffers_.end()) {
            ktxTexture* ktxTex = nullptr;
            KTX_error_code kc = ktxTexture_CreateFromMemory(
                ktxIt->second.data(), ktxIt->second.size(),
                KTX_TEXTURE_CREATE_NO_FLAGS, &ktxTex);
            if (kc == KTX_SUCCESS && ktxTex) {
                if (ktxTex->classId == ktxTexture2_c) {
                    ktxTexture2* ktxTex2 = reinterpret_cast<ktxTexture2*>(ktxTex);
                    if (ktxTexture2_NeedsTranscoding(ktxTex2)) {
                        ktx_transcode_fmt_e targetFormat = KTX_TTF_RGBA32;
                        if (GLEW_ARB_texture_compression_bptc) {
                            targetFormat = KTX_TTF_BC7_RGBA;
                        } else if (GLEW_EXT_texture_compression_s3tc) {
                            targetFormat = KTX_TTF_BC3_RGBA;
                        }
                        
                        KTX_error_code tc = ktxTexture2_TranscodeBasis(ktxTex2, targetFormat, 0);
                        if (tc != KTX_SUCCESS) {
                            LOG_ERROR << "Failed to transcode KTX2 texture: " << ktxErrorString(tc) << std::endl;
                        }
                    }
                }
                GLenum target = 0, glerror = 0;
                kc = ktxTexture_GLUpload(ktxTex, &textureID, &target, &glerror);
                if (kc == KTX_SUCCESS && textureID != 0) {
                    // Apply sRGB to the base color / emissive textures for correct gamma.
                    // (ktxTexture_GLUpload picks the internal format from the KTX DFD;
                    //  for BasisU RGBA32 it will be GL_RGBA8. We cannot easily re-create
                    //  with GL_SRGB8_ALPHA8 after upload, so accept linear for now.)
                    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    LOG_INFO << "Texture " << i << " uploaded via ktxTexture_GLUpload" << std::endl;
                } else {
                    LOG_ERROR << "Texture " << i << " KTX GLUpload failed: "
                              << ktxErrorString(kc) << " (glerror=" << glerror << ")" << std::endl;
                    textureID = 0;
                }
                ktxTexture_Destroy(ktxTex);
            }
            // Done with the raw buffer; free it.
            ktxImageBuffers_.erase(ktxIt);
        }

        // DDS images: upload compressed blocks directly.
        auto ddsIt = ddsImageBuffers_.find(sourceImageIdx);
        if (ddsIt != ddsImageBuffers_.end()) {
            int ddsMips = uploadDDSTexture(ddsIt->second.data(), (int)ddsIt->second.size(),
                                           textureID, isSRGB[i]);
            if (ddsMips > 0 && textureID != 0) {
                glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER,
                                    ddsMips > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
                glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
                LOG_INFO << "Texture " << i << " uploaded via DDS (" << ddsMips << " mips)" << std::endl;
            } else {
                LOG_ERROR << "Texture " << i << " DDS upload failed" << std::endl;
                textureID = 0;
            }
            ddsImageBuffers_.erase(ddsIt);
        }

        // Non-KTX images: upload decoded RGBA pixels as before.
        if (textureID == 0 && gltfImg.bits > 0 && !gltfImg.image.empty()) {
            glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
            GLsizei levels = static_cast<GLsizei>(std::floor(std::log2(std::max(gltfImg.width, gltfImg.height)))) + 1;
            GLenum internalFormat = isSRGB[i] ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            glTextureStorage2D(textureID, levels, internalFormat, gltfImg.width, gltfImg.height);
            glTextureSubImage2D(textureID, 0, 0, 0, gltfImg.width, gltfImg.height,
                                GL_RGBA, GL_UNSIGNED_BYTE, gltfImg.image.data());
            glGenerateTextureMipmap(textureID);
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        if (textureID == 0) {
            // Last-resort 1x1 white placeholder
            glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
            glTextureStorage2D(textureID, 1, GL_RGBA8, 1, 1);
            unsigned char white[4] = {255, 255, 255, 255};
            glTextureSubImage2D(textureID, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        gltfTextures_[i] = textureID;
    }

    // 2. Load inverse bind matrices for skins
    if (!gltfModel_.skins.empty()) {
        const auto& skin = gltfModel_.skins[0];
        inverseBindMatrices_.resize(skin.joints.size(), glm::mat4(1.0f));
        if (skin.inverseBindMatrices >= 0) {
            const auto& accessor = gltfModel_.accessors[skin.inverseBindMatrices];
            const auto& bufferView = gltfModel_.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel_.buffers[bufferView.buffer];
            const float* matrixData = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            for (size_t m = 0; m < accessor.count; ++m) {
                inverseBindMatrices_[m] = glm::make_mat4(&matrixData[m * 16]);
            }
        }
    }

    // 3. Parse all meshes and primitives
    // Remember the node that owns the mesh: skinning must compensate for its
    // global transform so a root scale (e.g. mm->m 0.01) is not baked into the joint matrices.
    hasSkin_ = !gltfModel_.skins.empty();
    meshNodeIdx_ = -1;
    meshToNode_.clear();
    for (int i = 0; i < (int)gltfModel_.nodes.size(); ++i) {
        int meshIdx = gltfModel_.nodes[i].mesh;
        if (meshIdx >= 0) {
            if (meshNodeIdx_ < 0) {
                meshNodeIdx_ = i;
            }
            // Each mesh is owned by exactly one node in our supported assets; if a mesh
            // is instanced across nodes, keep the first owner so the vertex binding is stable.
            meshToNode_.emplace(meshIdx, i);
        }
    }
    // Per-node bind-pose AABBs (skinless bbox path). Sized to nodes; invalid until filled.
    nodePosMin_.assign(gltfModel_.nodes.size(), glm::vec3(1e9f));
    nodePosMax_.assign(gltfModel_.nodes.size(), glm::vec3(-1e9f));

    int vertexOffset = 0;
    for (size_t m = 0; m < gltfModel_.meshes.size(); ++m) {
        const auto& gltfMesh = gltfModel_.meshes[m];
        // Resolve the node that owns this mesh (skinless rigid-node animation binds
        // each vertex of this mesh to its owner's global transform).
        auto m2n = meshToNode_.find((int)m);
        int ownerNodeIdx = (m2n != meshToNode_.end()) ? m2n->second : -1;
        for (size_t p = 0; p < gltfMesh.primitives.size(); ++p) {
            const auto& primitive = gltfMesh.primitives[p];
            
            SubMesh sm;
            sm.indexOffset = indices_.size() * sizeof(unsigned int);
            
            // Read indices
            int indexAccessorIdx = primitive.indices;
            int numIndices = 0;
            if (indexAccessorIdx >= 0) {
                const auto& accessor = gltfModel_.accessors[indexAccessorIdx];
                numIndices = accessor.count;
                sm.indexCount = numIndices;
                
                int stride = 0;
                if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
                    const uint32_t* data = getAccessorData<uint32_t>(gltfModel_, indexAccessorIdx, stride);
                    if (data) {
                        for (size_t i = 0; i < accessor.count; ++i) {
                            indices_.push_back(data[i] + vertexOffset);
                        }
                    }
                } else if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* data = getAccessorData<uint16_t>(gltfModel_, indexAccessorIdx, stride);
                    if (data) {
                        for (size_t i = 0; i < accessor.count; ++i) {
                            indices_.push_back(data[i] + vertexOffset);
                        }
                    }
                } else if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* data = getAccessorData<uint8_t>(gltfModel_, indexAccessorIdx, stride);
                    if (data) {
                        for (size_t i = 0; i < accessor.count; ++i) {
                            indices_.push_back(data[i] + vertexOffset);
                        }
                    }
                }
            }
            
            // Read attributes
            int posAccessorIdx = primitive.attributes.count("POSITION") ? primitive.attributes.at("POSITION") : -1;
            int normAccessorIdx = primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1;
            int uvAccessorIdx = primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1;
            int tangAccessorIdx = primitive.attributes.count("TANGENT") ? primitive.attributes.at("TANGENT") : -1;
            int jointAccessorIdx = primitive.attributes.count("JOINTS_0") ? primitive.attributes.at("JOINTS_0") : -1;
            int weightAccessorIdx = primitive.attributes.count("WEIGHTS_0") ? primitive.attributes.at("WEIGHTS_0") : -1;
            
            int numVertices = 0;
            if (posAccessorIdx >= 0) {
                numVertices = gltfModel_.accessors[posAccessorIdx].count;
            }

            // Accumulate this primitive's POSITION accessor min/max into the owning
            // node's bind-pose AABB (used by the skinless animated bounding box).
            if (!hasSkin_ && ownerNodeIdx >= 0 && posAccessorIdx >= 0) {
                const auto& posAcc = gltfModel_.accessors[posAccessorIdx];
                if (posAcc.minValues.size() >= 3 && posAcc.maxValues.size() >= 3) {
                    glm::vec3 pmin(posAcc.minValues[0], posAcc.minValues[1], posAcc.minValues[2]);
                    glm::vec3 pmax(posAcc.maxValues[0], posAcc.maxValues[1], posAcc.maxValues[2]);
                    nodePosMin_[ownerNodeIdx] = glm::min(nodePosMin_[ownerNodeIdx], pmin);
                    nodePosMax_[ownerNodeIdx] = glm::max(nodePosMax_[ownerNodeIdx], pmax);
                }
            }
            
            int posStride = 0, normStride = 0, uvStride = 0, tangStride = 0, jointStride = 0, weightStride = 0;
            const float* posData = getAccessorData<float>(gltfModel_, posAccessorIdx, posStride);
            const float* normData = getAccessorData<float>(gltfModel_, normAccessorIdx, normStride);
            const float* uvData = getAccessorData<float>(gltfModel_, uvAccessorIdx, uvStride);
            const float* tangData = getAccessorData<float>(gltfModel_, tangAccessorIdx, tangStride);
            
            const void* jointDataRaw = nullptr;
            int jointComponentType = 0;
            if (jointAccessorIdx >= 0) {
                const auto& accessor = gltfModel_.accessors[jointAccessorIdx];
                jointComponentType = accessor.componentType;
                const auto& bufferView = gltfModel_.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel_.buffers[bufferView.buffer];
                jointStride = accessor.ByteStride(bufferView);
                jointDataRaw = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
            }
            
            const float* weightData = getAccessorData<float>(gltfModel_, weightAccessorIdx, weightStride);
            
            for (int v = 0; v < numVertices; ++v) {
                Vertex vert;
                
                // Position
                if (posData) {
                    const float* p = reinterpret_cast<const float*>(reinterpret_cast<const char*>(posData) + v * posStride);
                    vert.position = glm::vec3(p[0], p[1], p[2]);
                }
                
                // Normal
                if (normData) {
                    const float* n = reinterpret_cast<const float*>(reinterpret_cast<const char*>(normData) + v * normStride);
                    vert.normal = glm::vec3(n[0], n[1], n[2]);
                }
                
                // UV
                if (uvData) {
                    const float* u = reinterpret_cast<const float*>(reinterpret_cast<const char*>(uvData) + v * uvStride);
                    vert.texCoords = glm::vec2(u[0], 1.0f - u[1]);
                }
                
                // Tangent
                if (tangData) {
                    const float* t = reinterpret_cast<const float*>(reinterpret_cast<const char*>(tangData) + v * tangStride);
                    vert.tangent = glm::vec3(t[0], t[1], t[2]);
                }
                
                // Joints and Weights
                if (jointDataRaw && weightData) {
                    const float* w = reinterpret_cast<const float*>(reinterpret_cast<const char*>(weightData) + v * weightStride);
                    vert.boneWeights = glm::vec4(w[0], w[1], w[2], w[3]);

                    if (jointComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
                        const uint8_t* j = reinterpret_cast<const uint8_t*>(reinterpret_cast<const char*>(jointDataRaw) + v * jointStride);
                        vert.boneIds = glm::ivec4(j[0], j[1], j[2], j[3]);
                    } else if (jointComponentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* j = reinterpret_cast<const uint16_t*>(reinterpret_cast<const char*>(jointDataRaw) + v * jointStride);
                        vert.boneIds = glm::ivec4(j[0], j[1], j[2], j[3]);
                    }
                } else if (!hasSkin_ && ownerNodeIdx >= 0) {
                    // No skin: bind the vertex rigidly to its owning node's global
                    // transform. The shader's skinning path then applies that node's
                    // animated TRS to the (node-local) position.
                    vert.boneIds = glm::ivec4(ownerNodeIdx, -1, -1, -1);
                    vert.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                }
                
                vertices_.push_back(vert);
            }
            
            // Setup texture bindings for the submesh
            if (primitive.material >= 0 && primitive.material < (int)gltfModel_.materials.size()) {
                const auto& material = gltfModel_.materials[primitive.material];
                
                // Diffuse. Microsoft GLTF exporter emits an empty pbrMetallicRoughness block
                // and stores the base-color texture under KHR_materials_pbrSpecularGlossiness.
                int baseColorTexIdx = material.pbrMetallicRoughness.baseColorTexture.index;
                if (baseColorTexIdx < 0) {
                    auto extIt = material.extensions.find("KHR_materials_pbrSpecularGlossiness");
                    if (extIt != material.extensions.end() && extIt->second.IsObject()) {
                        const auto& diffuseInfo = extIt->second.Get("diffuseTexture");
                        if (diffuseInfo.IsObject()) {
                            const auto& idxVal = diffuseInfo.Get("index");
                            if (idxVal.IsNumber()) baseColorTexIdx = idxVal.GetNumberAsInt();
                        }
                    }
                }
                if (baseColorTexIdx >= 0 && baseColorTexIdx < (int)gltfTextures_.size()) {
                    sm.textureID = gltfTextures_[baseColorTexIdx];
                }
                
                // Normal
                int normalTexIdx = material.normalTexture.index;
                if (normalTexIdx >= 0) {
                    sm.normalMapID = gltfTextures_[normalTexIdx];
                }
                
                // MetallicRoughness
                int mrTexIdx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
                if (mrTexIdx >= 0) {
                    sm.mrMapID = gltfTextures_[mrTexIdx];
                }
            }
            
            subMeshes_.push_back(sm);
            vertexOffset += numVertices;
        }
    }

    meshVisibility_.assign(subMeshes_.size(), true);

    // Normalize bone weights
    for (auto& v : vertices_) {
        float totalWeight = v.boneWeights.x + v.boneWeights.y + v.boneWeights.z + v.boneWeights.w;
        if (totalWeight > 0.0f) {
            v.boneWeights /= totalWeight;
        } else {
            v.boneIds[0] = 0;
            v.boneWeights[0] = 1.0f;
        }
    }

    // 4. Calculate bounding box properties
    glm::vec3 boxMin(1e9f);
    glm::vec3 boxMax(-1e9f);
    bool hasVertices = !vertices_.empty();
    
    for (const auto& v : vertices_) {
        boxMin = glm::min(boxMin, v.position);
        boxMax = glm::max(boxMax, v.position);
    }
    
    if (hasVertices) {
        modelCenter_ = (boxMin + boxMax) * 0.5f;
        modelExtent_ = boxMax - boxMin;
        float maxDim = std::max(modelExtent_.x, std::max(modelExtent_.y, modelExtent_.z));
        modelScale_ = (maxDim > 0.0f) ? (1.5f / maxDim) : 1.0f;
        modelSize_ = glm::length(modelExtent_);
        bounds_.pMin = boxMin;
        bounds_.pMax = boxMax;
    } else {
        modelCenter_ = glm::vec3(0.0f);
        modelExtent_ = glm::vec3(0.0f);
        modelScale_ = 1.0f;
        modelSize_ = 0.0f;
    }

    // 5. Build GPU buffers
    vbo_ = VertexBuffer(vertices_.size() * sizeof(Vertex), vertices_.data());
    ibo_ = IndexBuffer(indices_.size() * sizeof(unsigned int), indices_.data());

    vao_ = VertexArray();
    vao_.setVertexBuffer(0, vbo_, 0, sizeof(Vertex));
    vao_.setIndexBuffer(ibo_);

    // Attributes setup
    vao_.enableAttrib(0);
    vao_.attribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    vao_.attribBinding(0, 0);

    vao_.enableAttrib(1);
    vao_.attribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    vao_.attribBinding(1, 0);

    vao_.enableAttrib(2);
    vao_.attribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoords));
    vao_.attribBinding(2, 0);

    vao_.enableAttrib(3);
    vao_.attribIFormat(3, 4, GL_INT, offsetof(Vertex, boneIds));
    vao_.attribBinding(3, 0);

    vao_.enableAttrib(4);
    vao_.attribFormat(4, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, boneWeights));
    vao_.attribBinding(4, 0);

    vao_.enableAttrib(5);
    vao_.attribFormat(5, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
    vao_.attribBinding(5, 0);

    // 6. Setup SSBO for Bone matrices
    if (!gltfModel_.skins.empty()) {
        const auto& skin = gltfModel_.skins[0];
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        ssboMatrixCount_ = std::max((size_t)1, skin.joints.size());
        GLsizeiptr ssboSize = ssboMatrixCount_ * sizeof(glm::mat4);
        ssbo_ = StorageBuffer(ssboSize, nullptr, flags);
        mappedBones_ = (glm::mat4*)ssbo_.mapRange(0, ssboSize, flags);

        for (size_t i = 0; i < ssboMatrixCount_; i++) {
            mappedBones_[i] = glm::mat4(1.0f);
        }
    } else {
        // Skinless: one matrix per node, indexed by node index. Each vertex is bound
        // to its owning node, so the shader applies that node's animated global transform.
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        ssboMatrixCount_ = std::max((size_t)1, gltfModel_.nodes.size());
        GLsizeiptr ssboSize = ssboMatrixCount_ * sizeof(glm::mat4);
        ssbo_ = StorageBuffer(ssboSize, nullptr, flags);
        mappedBones_ = (glm::mat4*)ssbo_.mapRange(0, ssboSize, flags);
        for (size_t i = 0; i < ssboMatrixCount_; i++) {
            mappedBones_[i] = glm::mat4(1.0f);
        }
    }

    // build boundingbox
    std::vector<BboxVertex> bboxVertices;
	bboxVertices.resize(8);
	bboxVertices[0] = { {bounds_.pMin.x, bounds_.pMin.y, bounds_.pMin.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[1] = { {bounds_.pMax.x, bounds_.pMin.y, bounds_.pMin.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[2] = { {bounds_.pMax.x, bounds_.pMax.y, bounds_.pMin.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[3] = { {bounds_.pMin.x, bounds_.pMax.y, bounds_.pMin.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[4] = { {bounds_.pMin.x, bounds_.pMin.y, bounds_.pMax.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[5] = { {bounds_.pMax.x, bounds_.pMin.y, bounds_.pMax.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[6] = { {bounds_.pMax.x, bounds_.pMax.y, bounds_.pMax.z}, {0.0f, 0.0f, 0.0f} };
	bboxVertices[7] = { {bounds_.pMin.x, bounds_.pMax.y, bounds_.pMax.z}, {0.0f, 0.0f, 0.0f} };
	unsigned int edgeIndices[][2] = {
		{0,1}, {1,2}, {2,3}, {3,0},
		{4,5}, {5,6}, {6,7}, {7,4},
		{0,4}, {1,5}, {2,6}, {3,7} 
	};
    bboxIndices_.clear();
	for (auto& e : edgeIndices) {
		bboxIndices_.push_back(e[0]);
		bboxIndices_.push_back(e[1]);
	}
    bboxVbo_ = VertexBuffer(bboxVertices.size() * sizeof(BboxVertex), bboxVertices.data(), GL_DYNAMIC_STORAGE_BIT);
    bboxIbo_ = IndexBuffer(bboxIndices_.size() * sizeof(unsigned int), bboxIndices_.data());

    bboxVao_ = VertexArray();
	bboxVao_.setVertexBuffer(0, bboxVbo_, 0, sizeof(BboxVertex));
	bboxVao_.setIndexBuffer(bboxIbo_);

    bboxVao_.enableAttrib(0);
	bboxVao_.attribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(BboxVertex, position));
	bboxVao_.attribBinding(0, 0);

	bboxVao_.enableAttrib(1);
	bboxVao_.attribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(BboxVertex, color));
	bboxVao_.attribBinding(1, 0);

    // Precompute per-joint bind-pose bounds. One-time O(V) pass; updateBoundingBox()
    // then animates the box in O(joints) per frame.
    //
    // A deformed vertex is p = Σ_j w_j * (M_j * p), a convex combination of the points
    // M_j * p, each of which lies in AABB(M_j * S_j) since p is in S_j for every joint
    // that influences it. The AABB of the union of those per-joint AABBs is convex and
    // therefore contains every deformed vertex — exact, and tight at rest (union of the
    // local cluster boxes ≈ mesh bounds).
    jointBounds_.clear();
    if (!gltfModel_.skins.empty()) {
        jointBounds_.resize(inverseBindMatrices_.size());
        for (const Vertex& v : vertices_) {
            for (int i = 0; i < 4; ++i) {
                int bone = v.boneIds[i];
                if (bone >= 0 && bone < (int)jointBounds_.size() && v.boneWeights[i] > 0.0f) {
                    jointBounds_[bone].extend(v.position);
                }
            }
        }
    }

    return true;
}

void Model::setAnimationIndex(unsigned int index) {
    if (index < gltfModel_.animations.size()) {
        activeAnimationIndex_ = index;
    } else {
        activeAnimationIndex_ = 0;
    }
}

void Model::draw(bool useMipmap)
{
	getVAO().bind();
	int subMeshIdx = 0;
	for (const auto& sm : subMeshes_) {
		if (subMeshIdx < (int)meshVisibility_.size() && !meshVisibility_[subMeshIdx]) {
			subMeshIdx++;
			continue;
		}
		subMeshIdx++;

		if (sm.textureID > 0) {
			glBindTextureUnit(0, sm.textureID);
			GLint width = 0;
			glGetTextureLevelParameteriv(sm.textureID, 1, GL_TEXTURE_WIDTH, &width);
			glTextureParameteri(sm.textureID, GL_TEXTURE_MIN_FILTER, (useMipmap && width > 0) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		}
		if (sm.normalMapID > 0) {
			glBindTextureUnit(1, sm.normalMapID);
			GLint width = 0;
			glGetTextureLevelParameteriv(sm.normalMapID, 1, GL_TEXTURE_WIDTH, &width);
			glTextureParameteri(sm.normalMapID, GL_TEXTURE_MIN_FILTER, (useMipmap && width > 0) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		}
		if (sm.mrMapID > 0) {
			glBindTextureUnit(2, sm.mrMapID);
			GLint width = 0;
			glGetTextureLevelParameteriv(sm.mrMapID, 1, GL_TEXTURE_WIDTH, &width);
			glTextureParameteri(sm.mrMapID, GL_TEXTURE_MIN_FILTER, (useMipmap && width > 0) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		}

		glUniform1i(6, sm.textureID > 0 ? 1 : 0);
		glUniform1i(7, sm.normalMapID > 0 ? 1 : 0);
		glUniform1i(8, sm.mrMapID > 0 ? 1 : 0);

		glDrawElements(GL_TRIANGLES, sm.indexCount, GL_UNSIGNED_INT, (void*)(uintptr_t)sm.indexOffset);
	}
}


void Model::drawBoundingBox()
{
	updateBoundingBox();
	bboxVao_.bind();
    glDrawElements(GL_LINES, (GLsizei)bboxIndices_.size(), GL_UNSIGNED_INT, nullptr);
}

// Animate the AABB in O(joints) instead of skinning every vertex on the CPU.
//
// Every deformed vertex is a convex combination of the points M_j * p (joint matrix
// times the bind pose), so it lies inside the AABB of the union of the transformed
// per-joint boxes. That union is exact and tight at rest (see precompute comment).
void Model::updateBoundingBox()
{
    if (!mappedBones_ || bboxVbo_.id() == 0)
        return;

    glm::vec3 mn(1e9f), mx(-1e9f);

    if (!jointBounds_.empty()) {
        // Skinned path: union of per-joint transformed bind-pose boxes.
        for (size_t j = 0; j < jointBounds_.size(); ++j) {
            const JointBounds& b = jointBounds_[j];
            if (!b.valid()) continue;

            const glm::mat4& m = mappedBones_[j];
            glm::vec3 corners[8] = {
                glm::vec3(m * glm::vec4(b.pMin.x, b.pMin.y, b.pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMax.x, b.pMin.y, b.pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMax.x, b.pMax.y, b.pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMin.x, b.pMax.y, b.pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMin.x, b.pMin.y, b.pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMax.x, b.pMin.y, b.pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMax.x, b.pMax.y, b.pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(b.pMin.x, b.pMax.y, b.pMax.z, 1.0f)),
            };
            for (int c = 0; c < 8; ++c) {
                mn = glm::min(mn, corners[c]);
                mx = glm::max(mx, corners[c]);
            }
        }
    } else if (!lastAbsoluteTransforms_.empty()) {
        // Skinless path: union of each mesh node's bind-pose AABB transformed by
        // its animated global matrix. Uses the transforms cached by updateAnimation().
        for (size_t i = 0; i < nodePosMin_.size() && i < lastAbsoluteTransforms_.size(); ++i) {
            if (nodePosMin_[i].x > nodePosMax_[i].x) continue;  // not a mesh node
            const glm::mat4& m = lastAbsoluteTransforms_[i];
            const glm::vec3& pMin = nodePosMin_[i];
            const glm::vec3& pMax = nodePosMax_[i];
            glm::vec3 corners[8] = {
                glm::vec3(m * glm::vec4(pMin.x, pMin.y, pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMax.x, pMin.y, pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMax.x, pMax.y, pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMin.x, pMax.y, pMin.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMin.x, pMin.y, pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMax.x, pMin.y, pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMax.x, pMax.y, pMax.z, 1.0f)),
                glm::vec3(m * glm::vec4(pMin.x, pMax.y, pMax.z, 1.0f)),
            };
            for (int c = 0; c < 8; ++c) {
                mn = glm::min(mn, corners[c]);
                mx = glm::max(mx, corners[c]);
            }
        }
    }

    if (mn.x > mx.x) return;  // nothing contributed

    BboxVertex boxCorners[8] = {
        { {mn.x, mn.y, mn.z}, {0.0f, 0.0f, 0.0f} },
        { {mx.x, mn.y, mn.z}, {0.0f, 0.0f, 0.0f} },
        { {mx.x, mx.y, mn.z}, {0.0f, 0.0f, 0.0f} },
        { {mn.x, mx.y, mn.z}, {0.0f, 0.0f, 0.0f} },
        { {mn.x, mn.y, mx.z}, {0.0f, 0.0f, 0.0f} },
        { {mx.x, mn.y, mx.z}, {0.0f, 0.0f, 0.0f} },
        { {mx.x, mx.y, mx.z}, {0.0f, 0.0f, 0.0f} },
        { {mn.x, mx.y, mx.z}, {0.0f, 0.0f, 0.0f} },
    };
    glNamedBufferSubData(bboxVbo_.id(), 0, sizeof(boxCorners), boxCorners);
}

void Model::updateAnimation(float timeInTicks) {
    if (!mappedBones_) return;

    const tinygltf::Skin* skin = hasSkin_ ? &gltfModel_.skins[0] : nullptr;

    // Calculate time in seconds
    float timeInSeconds = 0.0f;
    if (activeAnimationIndex_ < gltfModel_.animations.size()) {
        const auto& animation = gltfModel_.animations[activeAnimationIndex_];
        float duration = 0.0f;
        for (const auto& sampler : animation.samplers) {
            int inputAccessorIdx = sampler.input;
            if (inputAccessorIdx >= 0 && inputAccessorIdx < (int)gltfModel_.accessors.size()) {
                const auto& accessor = gltfModel_.accessors[inputAccessorIdx];
                float maxVal = accessor.maxValues.empty() ? 0.0f : (float)accessor.maxValues[0];
                duration = std::max(duration, maxVal);
            }
        }
        if (duration > 0.0f) {
            timeInSeconds = std::fmod(timeInTicks / 25.0f, duration);
        }
    }
    
    // Initialize node local transforms
    std::vector<NodeTransform> localTransforms(gltfModel_.nodes.size());
    for (size_t i = 0; i < gltfModel_.nodes.size(); ++i) {
        const auto& node = gltfModel_.nodes[i];
        if (node.matrix.size() == 16) {
            localTransforms[i].matrix = glm::make_mat4(node.matrix.data());
            localTransforms[i].useMatrix = true;
        } else {
            if (node.translation.size() == 3) {
                localTransforms[i].translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
            }
            if (node.rotation.size() == 4) {
                localTransforms[i].rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            }
            if (node.scale.size() == 3) {
                localTransforms[i].scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
            }
        }
    }
    
    // Apply animation updates
    if (activeAnimationIndex_ < gltfModel_.animations.size()) {
        const auto& animation = gltfModel_.animations[activeAnimationIndex_];
        for (const auto& channel : animation.channels) {
            int nodeIdx = channel.target_node;
            if (nodeIdx < 0 || nodeIdx >= (int)localTransforms.size()) continue;
            
            if (channel.target_path == "translation") {
                localTransforms[nodeIdx].translation = interpolateTranslation(gltfModel_, animation.samplers[channel.sampler], timeInSeconds);
            } else if (channel.target_path == "rotation") {
                localTransforms[nodeIdx].rotation = interpolateRotation(gltfModel_, animation.samplers[channel.sampler], timeInSeconds);
            } else if (channel.target_path == "scale") {
                localTransforms[nodeIdx].scale = interpolateScale(gltfModel_, animation.samplers[channel.sampler], timeInSeconds);
            }
        }
    }
    
    // Compute absolute transforms recursively
    std::vector<glm::mat4> absoluteTransforms(gltfModel_.nodes.size(), glm::mat4(1.0f));
    
    std::vector<int> parentMap(gltfModel_.nodes.size(), -1);
    for (size_t i = 0; i < gltfModel_.nodes.size(); ++i) {
        for (int child : gltfModel_.nodes[i].children) {
            if (child >= 0 && child < (int)parentMap.size()) {
                parentMap[child] = i;
            }
        }
    }
    
    std::vector<int> roots;
    for (size_t i = 0; i < gltfModel_.nodes.size(); ++i) {
        if (parentMap[i] == -1) {
            roots.push_back(i);
        }
    }
    
    auto computeAbsolute = [&](auto& self, int nodeIdx, const glm::mat4& parentTransform) -> void {
        if (nodeIdx < 0 || nodeIdx >= (int)gltfModel_.nodes.size()) return;
        
        glm::mat4 localMat;
        if (localTransforms[nodeIdx].useMatrix) {
            localMat = localTransforms[nodeIdx].matrix;
        } else {
            localMat = glm::translate(glm::mat4(1.0f), localTransforms[nodeIdx].translation) *
                       glm::mat4_cast(localTransforms[nodeIdx].rotation) *
                       glm::scale(glm::mat4(1.0f), localTransforms[nodeIdx].scale);
        }
        
        absoluteTransforms[nodeIdx] = parentTransform * localMat;
        
        for (int child : gltfModel_.nodes[nodeIdx].children) {
            self(self, child, absoluteTransforms[nodeIdx]);
        }
    };
    
    for (int root : roots) {
        computeAbsolute(computeAbsolute, root, glm::mat4(1.0f));
    }
    
    if (skin) {
        // Compute joint matrices, compensating for the mesh node's global transform.
        // glTF spec: jointMatrix(j) = inverse(globalTransformOfMeshNode) * globalTransformOfJoint(j) * inverseBindMatrix(j)
        // Without the mesh-node inverse, a root scale (e.g. mm->m 0.01) in the scene graph
        // would shrink every skinned vertex 100x while modelScale/modelCenter were computed
        // from the raw (unskinned) geometry, making the whole model a sub-pixel speck.
        glm::mat4 inverseMeshTransform(1.0f);
        if (meshNodeIdx_ >= 0 && meshNodeIdx_ < (int)absoluteTransforms.size()) {
            inverseMeshTransform = glm::inverse(absoluteTransforms[meshNodeIdx_]);
        }

        for (size_t i = 0; i < skin->joints.size(); ++i) {
            int jointNodeIdx = skin->joints[i];
            if (jointNodeIdx >= 0 && jointNodeIdx < (int)absoluteTransforms.size()) {
                mappedBones_[i] = inverseMeshTransform * absoluteTransforms[jointNodeIdx] * inverseBindMatrices_[i];
            }
        }
    } else {
        // Skinless rigid-node animation: each vertex is bound to its owning node's
        // global transform. Vertex positions are node-local, so writing the node's
        // absolute matrix places the geometry at its animated world pose; the `model`
        // uniform (center/scale) then frames the whole scene.
        for (size_t i = 0; i < absoluteTransforms.size() && i < ssboMatrixCount_; ++i) {
            mappedBones_[i] = absoluteTransforms[i];
        }
    }

    lastAbsoluteTransforms_ = std::move(absoluteTransforms);
}

