#pragma once

#include "camera.h"
#include "model.h"

struct GLFWwindow;
struct ImFont;

class Application
{
public:
	Application(uint32_t w = 1920, uint32_t h = 1080);
	virtual ~Application();

	virtual void onAttach();
	virtual void onDetach();
	virtual void onUpdate(float deltaTime);
	virtual void onUpdateUI(float deltaTime);

	void run();
	virtual void loadNewModel(const char* path);
	void setDpiScale(float scale) { dpiScale = scale; }

private:
	void updateTitle(float deltaTime);

public:
	ArcballCamera camera;
	// mouse event
	bool leftMouseDown = false;
	bool rightMouseDown = false;
	double lastX = 0.0;
	double lastY = 0.0;
	bool firstMouse = false;
	float dpiScale = 1.0f;

protected:
	float lastFrameTime_ = 0.0f;
	Model* activeModel_ = nullptr;
	GLFWwindow* windowHandle_{ nullptr };
	std::string title_ = "glTF 2.0 Viewer";

	uint32_t width_, height_;
};
