#include "Renderer/TextureManager.h"

namespace viewer
{
	Ref<TextureManager> TextureManager::s_Instance = nullptr;

	TextureManager::TextureManager()
	{

	}

	TextureManager::~TextureManager()
	{

	}

	void TextureManager::Init()
	{
		s_Instance = CreateRef<TextureManager>();

		TextureCreateInfo whiteTextureCI{};
		//whiteTextureCI.Path = ASSET_DIR + std::string("/textures/white.png");
		whiteTextureCI.Name = "WhiteTexture";
		whiteTextureCI.Format = TextureFormat::RGBA8;
		whiteTextureCI.Width = 1;
		whiteTextureCI.Height = 1;
		whiteTextureCI.Layers = 1;
		s_Instance->m_WhiteTexture = CreateRef<Texture2D>(whiteTextureCI);
		unsigned char rgba[4] = { 255.0, 255.0, 255.0, 255.0 };
		s_Instance->m_WhiteTexture->SetData(rgba, sizeof(unsigned char) * 4);
	}

	void TextureManager::LoadTexture(const FilePath& path)
	{
		s_Instance->m_Textures.push_back(CreateRef<Texture2D>(path.string()));
	}

	int TextureManager::AddTexture(const Ref<Texture>& texture)
	{
		s_Instance->m_Textures.push_back(texture);
		return s_Instance->m_Textures.size() - 1;
	}

	Ref<Texture> TextureManager::GetTexture(const String& name)
	{
		auto it = std::find_if(s_Instance->m_Textures.begin(), s_Instance->m_Textures.end(),
			[&](Ref<Texture> t) { return t->GetName() == name; });
		if (it != s_Instance->m_Textures.end())
		{
			auto index = std::distance(s_Instance->m_Textures.begin(), it);
			return s_Instance->m_Textures[index];
		}
		return nullptr;
	}

	Ref<Texture> TextureManager::GetTexture(int index)
	{
		if (index < 0 && index >= s_Instance->m_Textures.size())
			return nullptr;
		return s_Instance->m_Textures[index];
	}

	Ref<Texture> TextureManager::GetWhiteTexture()
	{
		return s_Instance->m_WhiteTexture;
	}

	void TextureManager::AppendTextures(const RefVector<Texture>& texture)
	{
		s_Instance->m_Textures.insert(s_Instance->m_Textures.end(), texture.begin(), texture.end());
	}

}