#pragma once

#include "Engine/Renderer/UniformBuffer.h"

namespace Syndra {

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		~VulkanUniformBuffer() override = default;

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
	private:
		std::vector<uint8_t> m_Data;
		uint32_t m_Binding = 0;
	};

}
