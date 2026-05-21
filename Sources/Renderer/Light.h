#pragma once

#include <Core/Base.h>
#include <glm/glm.hpp>

namespace viewer
{

	enum class LightType
	{
		Point = 0,
		Spot,
		Area,
		Directional,
	};

	class alignas(16) Light
	{
	public:
		glm::vec4 position = glm::vec4(1.0f);
		glm::vec4 direction = glm::vec4(1.0f, -1.0f, 1.0f, 1.0f);

		glm::vec4 ambient = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		glm::vec4 diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		float constant = 1.0f;
		float linear = 0.01f;
		float quadratic = 0.032f;
		float cutOff = glm::cos(glm::radians(12.5f));
		float outerCutOff = glm::cos(glm::radians(17.5f));
		int type = 0;

		// PCSS params
		float shadowBias = 0.005f;
		float shadowFarPlane = 25.0f;
		float pcssSearchRadius = 2.5f;  // search radius for blocker search
		float pcssFilterRadius = 1.5f;
	};

	// Max Light Num is 32
	struct alignas(16) LightBlock
	{
		Light lights[32];
		int numLights;
	};
}