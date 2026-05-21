#pragma once

#include "Core/Base.h"
#include "Renderer/Model.h"

#include <glm/glm.hpp>

namespace viewer
{

	struct SceneData {
		glm::mat4 View;
		glm::mat4 Proj;
		glm::vec4 ViewPos;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		RefVector<Model> Models;
	};

}