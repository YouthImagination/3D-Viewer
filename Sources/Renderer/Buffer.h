#pragma once

#include "Core/Base.h"

#include <glm/glm.hpp>

namespace viewer
{

	enum class ShaderDataType {
		None, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type) {
		switch (type) {
		case ShaderDataType::Float:		return 4;
		case ShaderDataType::Float2:	return 4 * 2;
		case ShaderDataType::Float3:	return 4 * 3;
		case ShaderDataType::Float4:	return 4 * 4;
		case ShaderDataType::Mat3:		return 4 * 3 * 3;
		case ShaderDataType::Mat4:		return 4 * 4 * 4;
		case ShaderDataType::Int:		return 4;
		case ShaderDataType::Int2:		return 4 * 2;
		case ShaderDataType::Int3:		return 4 * 3;
		case ShaderDataType::Int4:		return 4 * 4;
		case ShaderDataType::Bool:		return 1;
		}

		LogA(false, "Unknown ShaderDataType!");
		return 0;
	}

	struct BufferElements {
		std::string Name;
		ShaderDataType Type;
		uint32_t Size;
		uint32_t Offset;
		bool Normalized;

		BufferElements() = default;
		BufferElements(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const {
			switch (Type) {
			case ShaderDataType::Float:		return 1;
			case ShaderDataType::Float2:	return 2;
			case ShaderDataType::Float3:	return 3;
			case ShaderDataType::Float4:	return 4;
			case ShaderDataType::Mat3:		return 3; // 3 * float3
			case ShaderDataType::Mat4:		return 4; // 4 * float4
			case ShaderDataType::Int:		return 1;
			case ShaderDataType::Int2:		return 2;
			case ShaderDataType::Int3:		return 3;
			case ShaderDataType::Int4:		return 4;
			case ShaderDataType::Bool:		return 1;
			}
			LogA(false, "Unknown ShaderDataType!");
			return 0;
		}

	};

	class BufferLayout {
	public:
		BufferLayout() {}
		BufferLayout(std::initializer_list<BufferElements> elements) ///> initialize_list to initialize vector
			:m_Element(elements)
		{
			CalculateOffset();
		}

		inline const std::vector<BufferElements>& GetElements() const { return m_Element; }
		inline const uint32_t GetStride() const { return m_stride; }

		std::vector<BufferElements>::iterator begin() { return m_Element.begin(); }
		std::vector<BufferElements>::iterator end() { return m_Element.end(); }
		std::vector<BufferElements>::const_iterator begin() const { return m_Element.begin(); }
		std::vector<BufferElements>::const_iterator end() const { return m_Element.end(); }
	private:
		void CalculateOffset() {
			size_t offset;
			m_stride;
			for (auto& element : m_Element) {
				element.Offset = offset;
				offset += element.Size;	// Big mistake: forgot +=
				m_stride += element.Size;
			}
		}
		uint32_t m_stride;
		std::vector<BufferElements> m_Element;
	};

	enum class VertexComponent
	{
		Position,
		Normal,
		UV,
		UV1,
		Color,
		Tangent,
		Bitangent,
		Joint0,
		Weight0
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 UV;
		glm::vec2 UV1;
		glm::vec4 Color;
		glm::vec4 Joint0;
		glm::vec4 Weight0;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
		glm::vec2 Padding;	// for align
	};

	class VertexBuffer {
	public:
		VertexBuffer(float* vertices, uint32_t size);
		VertexBuffer(uint32_t size);
		VertexBuffer(const std::vector<Vertex>& vertices);
		~VertexBuffer();

		void Bind() const;
		void Unbind() const;

		void SetData(const void* data, uint32_t size);
	private:
		uint32_t m_Renderer;
	};

	class IndexBuffer {
	public:
		IndexBuffer(uint32* indices, uint32 count);
		~IndexBuffer();

		void Bind() const;
		void Unbind() const;
		uint32_t GetCount() const;
	private:
		uint32_t m_Count;
		uint32_t m_Renderer;
	};

}