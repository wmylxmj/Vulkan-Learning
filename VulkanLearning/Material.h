#pragma once

#include "VulkanUtils.h"

class Material
{
public:
	Material();
	void Init();
	void Activate(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkBuffer uniformBuffer);

	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_descriptorPool;
};
