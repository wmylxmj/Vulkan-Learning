#pragma once

#include "VulkanUtils.h"

class Material
{
public:
	Material();
	void Init(const char* vertexShaderPath, const char* fragmentShaderPath);
	void InitVGF(const char* vertexShaderPath, const char* geometryShaderPath, const char* fragmentShaderPath);
	void InitVTF(const char* vertexShaderPath, const char* tessellationControlShaderPath, const char* tessellationEvaluationShaderPath, const char* fragmentShaderPath);
	void SetUniformBuffer(uint32_t dstBinding, VkBuffer uniformBuffer, uint32_t uniformBufferSize);
	void SetTexture(uint32_t dstBinding, VkImageView textureImageView, VkSampler textureSampler);
	void SetTexture2D(uint32_t dstBinding, uint32_t dstArreyIndex, VkImageView textureImageView, VkSampler textureSampler);
	void Activate(
		VkCommandBuffer commandBuffer,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout
	);

	VkDescriptorSet m_descriptorSet;
	VkDescriptorPool m_descriptorPool;
	VkPipeline m_pipeline;
	VkShaderModule m_vertexShaderModule;
	VkShaderModule m_tessellationControlShaderModule;
	VkShaderModule m_tessellationEvaluationShaderModule;
	VkShaderModule m_geometryShaderModule;
	VkShaderModule m_fragmentShaderModule;
	PipelineType m_pipelineType;
	VkPrimitiveTopology m_primitiveTopology;
};
