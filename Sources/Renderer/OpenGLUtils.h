#pragma once

#include <string>
#include <glad/gl.h>
#include "Core/Log.h"
#include "Renderer/Pipeline.h"

namespace viewer
{
	inline GLenum GetTextureFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8:
		case TextureFormat::RGBA8_SRGB:
		case TextureFormat::RGBA16F:
		case TextureFormat::RGBA32F:
			return GL_RGBA;
		case TextureFormat::RED_INTEGER:
			return GL_RED_INTEGER;
		case TextureFormat::RGB8:
		case TextureFormat::RGB8_SRGB:
		case TextureFormat::RGB16F:
		case TextureFormat::R11G11B10F:
		case TextureFormat::RGB32F:
			return GL_RGB;
		case TextureFormat::RG8_SRGB:
		case TextureFormat::RG8:
		case TextureFormat::RG16F:
			return GL_RG;
		case TextureFormat::R8:
		case TextureFormat::R8_SRGB:
		case TextureFormat::R32F:
			return GL_RED;
		case TextureFormat::DEPTH16:
		case TextureFormat::DEPTH32F:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DEPTH24STENCIL8:
		case TextureFormat::DEPTH32STENCIL8:
			return GL_DEPTH_STENCIL;
		default: LogA(false, "Unknown TextureFormat!"); return 0;
		}
		return GL_INVALID_ENUM;
	}

	inline GLenum GetInternalTextureFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8: return GL_RGBA8;
		case TextureFormat::RGB8: return GL_RGB8;
		case TextureFormat::RG8: return GL_RG8;
		case TextureFormat::R8: return GL_R8;
		case TextureFormat::RGBA8_SRGB: return GL_SRGB8_ALPHA8;
		case TextureFormat::RGB8_SRGB: return GL_SRGB8;
		case TextureFormat::RG8_SRGB: return GL_SRGB8;
		case TextureFormat::R8_SRGB: return GL_SRGB8;
		case TextureFormat::R32F: return GL_R32F;
		case TextureFormat::RG16F: return GL_RG16F;
		case TextureFormat::RGB16F: return GL_RGB16F;
		case TextureFormat::R11G11B10F: return GL_R11F_G11F_B10F;
		case TextureFormat::RGB32F: return GL_RGB32F;
		case TextureFormat::RGBA16F: return GL_RGBA16F;
		case TextureFormat::RGBA32F: return GL_RGBA32F;
		case TextureFormat::RED_INTEGER: return GL_RED_INTEGER;

		case TextureFormat::DEPTH16: return GL_DEPTH_COMPONENT16;
		case TextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		case TextureFormat::DEPTH32STENCIL8: return GL_DEPTH32F_STENCIL8;
		case TextureFormat::DEPTH32F: return GL_DEPTH_COMPONENT32F;
		default: LogA(false, "Unknown TextureFormat!"); return 0;
		}
		return GL_INVALID_ENUM;
	}

	inline GLenum GetTextureFilter(TextureFilter filter)
	{
		switch (filter)
		{
		case TextureFilter::LINEAR:
			return GL_LINEAR;
			break;
		case TextureFilter::NEAREST:
			return GL_NEAREST;
			break;
		default:
			break;
		}
		return GL_INVALID_ENUM;
	}

	inline GLenum GetTextureWrap(TextureWrap wrap)
	{
		switch (wrap)
		{
		case TextureWrap::REPEAT:
			return GL_REPEAT;
			break;
		case TextureWrap::CLAMP_TO_EDGE:
			return GL_CLAMP_TO_EDGE;
			break;
		case TextureWrap::CLAMP_TO_BORDER:
			return GL_CLAMP_TO_BORDER;
			break;
		case TextureWrap::MIRRORED_REPEAT:
			return GL_MIRRORED_REPEAT;
			break;
		case TextureWrap::MIRRORED_CLAMP_TO_EDGE:
			return GL_MIRROR_CLAMP_TO_EDGE;
			break;
		default:
			break;
		}
		return GL_INVALID_ENUM;
	}

	inline GLenum GetDepthCompareOp(const DepthCompareOperator& op)
	{
		switch (op)
		{
		case DepthCompareOperator::LESS:
			return GL_LESS;
			break;
		case DepthCompareOperator::LESS_OR_EQUAL:
			return GL_LEQUAL;
			break;
		case DepthCompareOperator::GREATER:
			return GL_GREATER;
			break;
		case DepthCompareOperator::GREATER_OR_EQUAL:
			return GL_GEQUAL;
			break;
		default:
			break;
		}
		return GL_ALWAYS;
	}

	inline GLenum GetBlendFactor(const BlendFactor& factor)
	{
		switch (factor)
		{
		case BlendFactor::ZERO: return GL_ZERO;
		case BlendFactor::ONE: return GL_ONE;
		case BlendFactor::SRC_COLOR: return GL_SRC_COLOR;
		case BlendFactor::ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DST_COLOR: return GL_DST_COLOR;
		case BlendFactor::ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
		case BlendFactor::SRC_ALPHA: return GL_SRC_ALPHA;
		case BlendFactor::ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DST_ALPHA: return GL_DST_ALPHA;
		case BlendFactor::ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
		default:
			LogA(false, "Unknown BlendFactor!");
			return 0;
		}
	}

	inline GLenum GetBlendOp(const BlendOp& op)
	{
		switch (op)
		{
		case BlendOp::ADD: return GL_FUNC_ADD;
		case BlendOp::SUBTRACT: return GL_FUNC_SUBTRACT;
		case BlendOp::REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
		case BlendOp::MIN: return GL_MIN;
		case BlendOp::MAX: return GL_MAX;
		default:
			LogA(false, "Unknown BlendOp!");
			return 0;
		}
	}

	inline GLenum GetCullMode(const CullMode& mode)
	{
		switch (mode)
		{
		case CullMode::NONE: return GL_NONE;
		case CullMode::BACK: return GL_BACK;
		case CullMode::FRONT: return GL_FRONT;
		case CullMode::FRONT_AND_BACK: return GL_FRONT_AND_BACK;
		default:
			LogA(false, "Unknown CullMode!");
			return 0;
		}
	}

	inline GLenum GetFillMode(const FillMode& mode)
	{
		switch (mode)
		{
		case FillMode::SOLID: return GL_FILL;
		case FillMode::WIREFRAME: return GL_LINE;
		case FillMode::POINT: return GL_POINT;
		default:
			LogA(false, "Unknown FillMode!");
			return 0;
		}
		return GL_INVALID_ENUM;
	}

	inline GLenum GetPrimitiveTopology(const Topology& topology)
	{
		switch (topology)
		{
		case Topology::TRIANGLE_LIST: return GL_TRIANGLES;
		case Topology::LINE_LIST: return GL_LINES;
		case Topology::POINT_LIST: return GL_POINTS;
		default:
			LogA(false, "Unknown Topology!");
			return 0;
		}
	}

#define CASE_CVT_GL(PRE, TOKEN) case PRE##TOKEN: return GL_##TOKEN

	inline GLint ConvertCubeFace(CubeMapFace face) {
		switch (face) {
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_X);
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_X);
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_Y);
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_Y);
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_Z);
			CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_Z);
		default:
			break;
		}
		return 0;
	}

	class OpenGLUtils {
	public:
		static void CheckGLError_(const char* stmt, const char* file, int line) {
			const char* str;
			GLenum err = glGetError();
			switch (err) {
			case GL_NO_ERROR:
				str = "GL_NO_ERROR";
				break;
			case GL_INVALID_ENUM:
				str = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				str = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION:
				str = "GL_INVALID_OPERATION";
				break;
			case GL_OUT_OF_MEMORY:
				str = "GL_OUT_OF_MEMORY";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				str = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			default:
				str = "(ERROR: Unknown Error Enum)";
				break;
			}

			if (err != GL_NO_ERROR) {
				LogE("GL_CHECK: {}, {}:{}, {}", str, file, line, stmt);
				abort();
			}
		}
	};

	inline std::string GetAssetDirs()
	{
		return std::string(ASSET_DIR);
	}

	inline std::string GetShaderDirs()
	{
		return std::string(ASSET_DIR) + "Shaders/GLSL/";
	}

	class FileUtils {
	public:
		static bool Exists(const std::string& path) {
			std::ifstream file(path);
			return file.good();
		}

		static std::vector<uint8_t> ReadBytes(const std::string& path) {
			std::vector<uint8_t> ret;
			std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
			if (!file.is_open()) {
				LogE("failed to open file: {}", path.c_str());
				return ret;
			}

			size_t size = file.tellg();
			if (size <= 0) {
				LogE("failed to read file, invalid size: %d", size);
				return ret;
			}

			ret.resize(size);

			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(ret.data()), (std::streamsize)size);

			return ret;
		}

		static std::string ReadText(const std::string& path) {
			auto data = ReadBytes(path);
			if (data.empty()) {
				return "";
			}

			return { (char*)data.data(), data.size() };
		}

		static bool WriteBytes(const std::string& path, const char* data, size_t length) {
			std::ofstream file(path, std::ios::out | std::ios::binary);
			if (!file.is_open()) {
				LogE("failed to open file: %s", path.c_str());
				return false;
			}

			file.write(data, length);
			return true;
		}

		static bool WriteText(const std::string& path, const std::string& str) {
			std::ofstream file(path, std::ios::out);
			if (!file.is_open()) {
				LogE("failed to open file: %s", path.c_str());
				return false;
			}

			file.write(str.c_str(), str.length());
			file.close();

			return true;
		}
	};

#ifdef _DEBUG
#define GL_CHECK(stmt) do { \
            stmt; \
            OpenGLUtils::CheckGLError_(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif
}