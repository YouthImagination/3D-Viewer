#pragma once

#include "Core/Base.h"

#include "Renderer/Buffer.h"

namespace viewer
{

	class VertexArray
	{
	public:
		VertexArray();
		~VertexArray();

		void Bind() const;
		void Unbind() const;

		void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexbuffer, const std::vector<VertexComponent>& comps);
		void AddIndexBuffer(const std::shared_ptr<IndexBuffer>& indexbuffer);

		const std::vector<std::shared_ptr<VertexBuffer>> GetVertexBuffer() const { return m_VertexBuffer; }
		const std::shared_ptr<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffer;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;
		uint32_t m_VertexBufferIndex = 0;
		uint32_t m_RendererID;
	};

}