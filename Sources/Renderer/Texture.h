#pragma once

#include "Core/Base.h"

namespace viewer
{
	enum class TextureFormat
	{
		NONE = 0,
		// Color
		R8,
		R8_SRGB,
		RG8,
		RG8_SRGB,
		RGB8,
		RGB8_SRGB,
		RGBA8,
		RGBA8_SRGB,
		RED_INTEGER,

		R32F,
		RG16F,
		RGB16F,
		R11G11B10F,
		RGB32F,
		RGBA16F,
		RGBA32F,

		//Depth/Stencil
		DEPTH16,
		DEPTH24STENCIL8,
		DEPTH32STENCIL8,
		DEPTH32F
	};

	enum TextureUsage
	{
		NONE = BIT(0),
		SAMPLED = BIT(1),
		STORAGE = BIT(2),
		ATTACHMENT = BIT(3),

		DEFAULT = SAMPLED
	};

	enum class TextureType
	{
		TEXTURE_2D,
		TEXTURE_CUBE
	};

	enum class TextureFilter
	{
		LINEAR = 1,
		NEAREST = 2,
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	enum class TextureCompareOperator
	{
		NONE = 0,
		LESS_OR_EQUAL,
		GREATER_OR_EQUAL
	};

	enum class TextureMapType
	{
		None = 0,
		Albedo,
		Specular,
		Normal,
		Metallic,
		Roughness,
		AO,
		Emissive,
	};

	const uint TEXTURE_UNIT_ALBEDO = 0;
	const uint TEXTURE_UNIT_NORMAL = 1;
	const uint TEXTURE_UNIT_METALLIC = 2;
	const uint TEXTURE_UNIT_ROUGHNESS = 3;
	const uint TEXTURE_UNIT_AO = 4;
	const uint TEXTURE_UNIT_EMISSIVE = 5;
	const uint TEXTURE_UNIT_OTHER_START = 6;

	struct TextureSamplerCreateInfo
	{
		bool operator==(const TextureSamplerCreateInfo& other)
		{
			Filter = other.Filter;
			Wrap = other.Wrap;
			Compare = other.Compare;
		}

		TextureFilter Filter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;
		TextureCompareOperator Compare = TextureCompareOperator::NONE;
	};

	struct TextureCreateInfo
	{
		String Path;
		String Name;
		TextureFormat Format = TextureFormat::RGBA8;
		TextureUsage Usage = TextureUsage::DEFAULT;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		bool GenerateMipMap = false;
		bool MultiSample = false;
		TextureMapType MapType = TextureMapType::None;
		TextureSamplerCreateInfo Sampler;
	};

	class Texture {
	public:
		~Texture() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		uint32_t GetRendererID() const;

		void SetData(void* data, uint32_t size);
		bool IsValid() const;
		bool IsLoaded() const;
		void Bind(uint32_t slot = 0) const;
		std::string GetPath() const;
		TextureFormat GetFormat() { return m_Info.Format; }
		String GetName() { return m_Info.Name; }
		void SetName(const String& name) { m_Info.Name = name; }
		TextureMapType GetMapType() { return m_Info.MapType; }
		void SetMapType(TextureMapType type) { m_Info.MapType = type; }
		bool IsMultiSample() const { return m_Info.MultiSample; }
		virtual TextureType GetType() const = 0;
		void Resize(uint32 w, uint32 h);

		static bool IsDepthFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::DEPTH16:		 return true;
			case TextureFormat::DEPTH24STENCIL8: return true;
			case TextureFormat::DEPTH32F:		 return true;
			}

			return false;
		}

		static bool IsStencilFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::DEPTH24STENCIL8: return true;
			}

			return false;
		}

		static bool IsColorFormat(TextureFormat format)
		{
			return !IsDepthFormat(format) && !IsStencilFormat(format);
		}

		bool operator==(const Texture& other)
		{
			return m_RendererID == ((Texture&)other).m_RendererID;
		}
	protected:
		void Create(const TextureCreateInfo& createInfo);
	protected:
		TextureCreateInfo m_Info;
		uint32_t m_Width;
		uint32_t m_Height;
		std::string m_path;
		uint m_DataFormat, m_InternalFormat;
		uint32_t m_RendererID;
		bool m_IsLoaded = false;
	};

	class Texture2D : public Texture {
	public:
		Texture2D(uint32_t width, uint32_t height);
		Texture2D(const std::string& path);
		Texture2D(const TextureCreateInfo& createInfo);

		virtual TextureType GetType() const override { return TextureType::TEXTURE_2D; }
	};

	enum CubeMapFace {
		TEXTURE_CUBE_MAP_POSITIVE_X = 0,
		TEXTURE_CUBE_MAP_NEGATIVE_X = 1,
		TEXTURE_CUBE_MAP_POSITIVE_Y = 2,
		TEXTURE_CUBE_MAP_NEGATIVE_Y = 3,
		TEXTURE_CUBE_MAP_POSITIVE_Z = 4,
		TEXTURE_CUBE_MAP_NEGATIVE_Z = 5,
	};

	class TextureCube : public Texture {
	public:
		TextureCube(uint32_t width, uint32_t height);
		TextureCube(const std::string& path);
		TextureCube(const TextureCreateInfo& createInfo);

		virtual TextureType GetType() const override { return TextureType::TEXTURE_CUBE; }
	};
}