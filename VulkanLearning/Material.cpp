#include "Material.h"
#include "StaticMesh.h"

Material::Material()
{
	m_descriptorSet = nullptr;
	m_pipeline = nullptr;
}

void Material::Init(const char* vertexShaderPath, const char* fragmentShaderPath)
{
	m_vertexShaderModule = CompileShader(vertexShaderPath);
	m_fragmentShaderModule = CompileShader(fragmentShaderPath);

	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();

	VkDescriptorPoolSize descriptorPoolSize[2] = {};
	descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize[0].descriptorCount = 32; // descriptor -> ubo, texture, sampler
	descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize[1].descriptorCount = 32;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSize) / sizeof(descriptorPoolSize[0]);
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;

	vkCreateDescriptorPool(GetVulkanDevice(), &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &pShaderParameterDescription->descriptorSetLayout;
	if (vkAllocateDescriptorSets(
		GetVulkanDevice(),
		&descriptorSetAllocateInfo,
		&m_descriptorSet
	) != VK_SUCCESS)
	{
		OutputDebugStringA("Failed to allocate descriptor sets!");
	}
}

void Material::InitVGF(const char* vertexShaderPath, const char* geometryShaderPath, const char* fragmentShaderPath)
{
	m_vertexShaderModule = CompileShader(vertexShaderPath);
	m_geometryShaderModule = CompileShader(geometryShaderPath);
	m_fragmentShaderModule = CompileShader(fragmentShaderPath);

	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();

	VkDescriptorPoolSize descriptorPoolSize[2] = {};
	descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize[0].descriptorCount = 32; // descriptor -> ubo, texture, sampler
	descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize[1].descriptorCount = 32;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSize) / sizeof(descriptorPoolSize[0]);
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;

	vkCreateDescriptorPool(GetVulkanDevice(), &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &pShaderParameterDescription->descriptorSetLayout;
	if (vkAllocateDescriptorSets(
		GetVulkanDevice(),
		&descriptorSetAllocateInfo,
		&m_descriptorSet
	) != VK_SUCCESS)
	{
		OutputDebugStringA("Failed to allocate descriptor sets!");
	}
}

void Material::InitVTF(const char* vertexShaderPath, const char* tessellationControlShaderPath, const char* tessellationEvaluationShaderPath, const char* fragmentShaderPath)
{
	m_vertexShaderModule = CompileShader(vertexShaderPath);
	m_tessellationControlShaderModule = CompileShader(tessellationControlShaderPath);
	m_tessellationEvaluationShaderModule = CompileShader(tessellationEvaluationShaderPath);
	m_fragmentShaderModule = CompileShader(fragmentShaderPath);

	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();

	VkDescriptorPoolSize descriptorPoolSize[2] = {};
	descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize[0].descriptorCount = 32; // descriptor -> ubo, texture, sampler
	descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSize[1].descriptorCount = 32;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSize) / sizeof(descriptorPoolSize[0]);
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;

	vkCreateDescriptorPool(GetVulkanDevice(), &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &pShaderParameterDescription->descriptorSetLayout;
	if (vkAllocateDescriptorSets(
		GetVulkanDevice(),
		&descriptorSetAllocateInfo,
		&m_descriptorSet
	) != VK_SUCCESS)
	{
		OutputDebugStringA("Failed to allocate descriptor sets!");
	}
}

void Material::SetUniformBuffer(uint32_t dstBinding, VkBuffer uniformBuffer, uint32_t uniformBufferSize)
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = uniformBufferSize;

	VkWriteDescriptorSet writeDescriptorSets[1] = {};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].pBufferInfo = &bufferInfo;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].dstBinding = dstBinding;
	writeDescriptorSets[0].dstSet = m_descriptorSet;

	vkUpdateDescriptorSets(GetVulkanDevice(), 1, writeDescriptorSets, 0, nullptr);
}

void Material::SetTexture(uint32_t dstBinding, VkImageView textureImageView, VkSampler textureSampler)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

	VkWriteDescriptorSet writeDescriptorSets[1] = {};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].pImageInfo = &imageInfo;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].dstBinding = dstBinding;
	writeDescriptorSets[0].dstSet = m_descriptorSet;

	vkUpdateDescriptorSets(GetVulkanDevice(), 1, writeDescriptorSets, 0, nullptr);
}

void Material::SetTexture2D(uint32_t dstBinding, uint32_t dstArreyIndex, VkImageView textureImageView, VkSampler textureSampler)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

	VkWriteDescriptorSet writeDescriptorSets[1] = {};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].pImageInfo = &imageInfo;
	writeDescriptorSets[0].dstArrayElement = dstArreyIndex;
	writeDescriptorSets[0].dstBinding = dstBinding;
	writeDescriptorSets[0].dstSet = m_descriptorSet;

	vkUpdateDescriptorSets(GetVulkanDevice(), 1, writeDescriptorSets, 0, nullptr);
}

void Material::Activate(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	if (m_pipeline == nullptr) {
		m_pipeline = CreateVTFPipeline(
			StaticMesh::sm_vertexInputBindingDescriptions,
			StaticMesh::sm_vertexInputAttributeDescriptions,
			m_vertexShaderModule,
			m_tessellationControlShaderModule,
			m_tessellationEvaluationShaderModule,
			m_fragmentShaderModule
		);
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&m_descriptorSet,
		0,
		nullptr
	);
}