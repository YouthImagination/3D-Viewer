#pragma once

#include "Core/Base.h"

struct GLFWwindow;

namespace viewer
{

	class Application
	{
	public:
		Application();
		~Application();

		int Run();
	private:
		GLFWwindow* window{nullptr};
	};
}