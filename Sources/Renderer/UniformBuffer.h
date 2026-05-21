#pragma once

#include "Core/Base.h"

namespace viewer
{
	class UniformBuffer {
	public:
		UniformBuffer(const String& name, uint32_t size, uint32_t binding);
		UniformBuffer(const String& name, uint32_t size);
		~UniformBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);

		bool MapAndUpdateData(const void* data, size_t size);

		int GetBinding(const std::string& name, uint programID);
		int GetLocation(const std::string& name, uint programID);
	private:
		uint32_t m_RendererID = 0;
		String m_Name;
	};
}