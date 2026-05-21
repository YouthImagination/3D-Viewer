#pragma once

#include "Core/Base.h"
#include "Renderer/Texture.h"

namespace viewer
{
	struct MaterialIndex
	{
		int AlbedoMapIndex = -1;
		int NormalMapIndex = -1;
		int SpecularMapIndex = -1;
		int HeightMapIndex = -1;
		int AmbientMapIndex = -1;
		int EmissiveMapIndex = -1;
	};

	struct Material
	{
		float shininess = 32.0f;
		float padding[3];
	};
}