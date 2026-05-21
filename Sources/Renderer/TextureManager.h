#pragma once

#include "Renderer/Texture.h"

namespace viewer
{
	class TextureManager
	{
	public:
		TextureManager();
		~TextureManager();

		static void Init();
		static void LoadTexture(const FilePath& path);
		static int AddTexture(const Ref<Texture>& texture);
		static Ref<Texture> GetTexture(const String& texture);
		static Ref<Texture> GetTexture(int index);
		static Ref<Texture> GetWhiteTexture();
		static void AppendTextures(const RefVector<Texture>& texture);

	private:
		RefVector<Texture> m_Textures;
		Ref<Texture> m_WhiteTexture;
		static Ref<TextureManager> s_Instance;
	};
}