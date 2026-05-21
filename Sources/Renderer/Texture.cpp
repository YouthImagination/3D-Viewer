#include "Renderer/Texture.h"
#include "Renderer/OpenGLUtils.h"
#include "stb_image.h"
#include "glad/gl.h"

namespace viewer
{

	uint32_t Texture::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Texture::GetHeight() const
	{
		return m_Height;
	}

	uint32_t Texture::GetRendererID() const
	{
		return m_RendererID;
	}

	void Texture::SetData(void* data, uint32_t size)
	{
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		LogA(size == m_Height * m_Width * bpp, "Data must be entire Texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	bool Texture::IsValid() const
	{
		if (m_Width < 0.0f || m_Height < 0.0f || m_Width > 4096.0f || m_Height > 4096.0f)
			return false;
		if (IsLoaded())
			return true;
		return false;
	}

	bool Texture::IsLoaded() const
	{
		return m_IsLoaded;
	}

	void Texture::Bind(uint32_t slot /*= 0*/) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	std::string Texture::GetPath() const
	{
		return m_path;
	}

	void Texture::Resize(uint32 w, uint32 h)
	{
		m_Info.Width = w;
		m_Info.Height = h;
		if (m_RendererID)
		{
			glDeleteTextures(1, &m_RendererID);
			m_RendererID = 0;
		}
		Create(m_Info);
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		m_InternalFormat = GL_RGBA8, m_DataFormat = GL_RGBA;
		m_Info.Format = TextureFormat::RGBA8;
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	Texture2D::Texture2D(const std::string& path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;
		{
			data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		}
		LogA(data, "Failed to load image!");
		if (data) {
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, format = 0;
			if (channels == 4) {
				internalFormat = GL_RGBA8;
				format = GL_RGBA;
				this->m_Info.Format = TextureFormat::RGBA8;
			}
			else if (channels == 3) {
				internalFormat = GL_RGB8;
				format = GL_RGB;
				this->m_Info.Format = TextureFormat::RGB8;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = format;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, data);	// glTexImage2D

			stbi_image_free(data);
		}
	}

	Texture2D::Texture2D(const TextureCreateInfo& createInfo)
	{
		Create(createInfo);
	}

	void Texture::Create(const TextureCreateInfo& createInfo)
	{
		this->m_Info = createInfo;
		m_Width = createInfo.Width;
		m_Height = createInfo.Height;
		if (!createInfo.Path.empty())
		{
			int width, height, channels;
			stbi_set_flip_vertically_on_load(1);
			stbi_uc* data = nullptr;
			{
				data = stbi_load(createInfo.Path.c_str(), &width, &height, &channels, 0);
			}
			LogA(data, "Failed to load image!");
			if (data) {
				m_IsLoaded = true;

				m_Width = width;
				m_Height = height;

				m_InternalFormat = GetInternalTextureFormat(createInfo.Format);
				m_DataFormat = GetTextureFormat(createInfo.Format);;

				glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
				glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

				auto Filter = GetTextureFilter(createInfo.Sampler.Filter);
				glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, Filter);
				glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, Filter);
				auto Wrap = GetTextureWrap(createInfo.Sampler.Wrap);
				glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, Wrap);
				glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, Wrap);

				glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);	// glTexImage2D

				stbi_image_free(data);
			}
		}
		else
		{
			m_InternalFormat = GetInternalTextureFormat(createInfo.Format);
			m_DataFormat = GetTextureFormat(createInfo.Format);;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

			auto Filter = GetTextureFilter(createInfo.Sampler.Filter);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, Filter);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, Filter);
			auto Wrap = GetTextureWrap(createInfo.Sampler.Wrap);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, Wrap);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, Wrap);
		}
	}

	TextureCube::TextureCube(uint32_t width, uint32_t height)
	{

	}

	TextureCube::TextureCube(const std::string& path)
	{

	}

	TextureCube::TextureCube(const TextureCreateInfo& createInfo)
	{

	}

}