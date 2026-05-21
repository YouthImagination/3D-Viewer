#include "Core/Application.h"

int main()
{
	int result = EXIT_SUCCESS;
	viewer::Application* app = new viewer::Application();
	try {
		result = app->Run();
	}
	catch (std::exception& e)
	{
		delete app;
		LogE(e.what());
	}
	delete app;
	return result;
}