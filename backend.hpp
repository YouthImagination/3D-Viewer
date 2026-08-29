#pragma once

enum class BackendType
{
	OpenGL,
	Vulkan
};

class Backend
{
public:
	Backend() = default;

	void setBackendType(BackendType type)
	{
		type_ = type;
	}

	BackendType getBackendType()
	{
		return type_;
	}

	bool isOpenGL() const {
		return type_ == BackendType::OpenGL;
	}

	bool isVulkan() const {
		return type_ == BackendType::Vulkan;
	}

	static Backend* instance()
	{
		static Backend* instance_ = nullptr;
		if (!instance_)
		{
			instance_ = new Backend;
		}
		return instance_;
	}

private:
	BackendType type_ = BackendType::OpenGL;
};

#define BackendInstance Backend::instance()