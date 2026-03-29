#pragma once

#include "VulkanUtils.h"

class Material
{
public:
	Material();
	void Init();
	void SetUniformBuffer(uint32_t dstBinding, VkBuffer uniformBuffer, uint32_t uniformBufferSize);
	void Activate(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_descriptorPool;
};
