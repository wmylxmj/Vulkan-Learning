#include "Scene.h"
#include "VulkanUtils.h"
#include "StaticMesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>

Buffer* s_pUniformBuffer = nullptr;

VkPipeline s_trianglePipeline = nullptr;
VkPipelineLayout s_pipelineLayout = nullptr;

VkDescriptorSet s_descriptorSet = nullptr;
VkDescriptorPool s_descriptorPool = nullptr;

StaticMesh s_triangleMesh;

VkShaderModule CompileShader(const char* inFilePath)
{
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0)
	{
		fseek(pFile, 0, SEEK_END);
		long fileSize = ftell(pFile);
		rewind(pFile);
		unsigned char* fileContent = new unsigned char[fileSize];
		fread(fileContent, 1, fileSize, pFile);
		fclose(pFile);

		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = fileSize;
		shaderCreateInfo.pCode = (uint32_t*)fileContent;

		VkShaderModule shader;
		if (vkCreateShaderModule(GetVulkanDevice(), &shaderCreateInfo, nullptr, &shader) != VK_SUCCESS)
		{
			std::string errorString = "Failed to create shader " + std::string(inFilePath);
			OutputDebugStringA(errorString.c_str());
		}
		return shader;
	}
	return nullptr;
}

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
	StaticMesh::Initialize();
	VkDevice vulkanDevice = GetVulkanDevice();

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = StaticMesh::sm_vertexInputBindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = StaticMesh::sm_vertexInputBindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = StaticMesh::sm_vertexInputAttributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = StaticMesh::sm_vertexInputAttributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 0;
	dynamicStateCreateInfo.pDynamicStates = nullptr;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(inCanvasWidth);
	viewport.height = static_cast<float>(inCanvasHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(inCanvasWidth), static_cast<uint32_t>(inCanvasHeight) };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkShaderModule vertexShaderModule = CompileShader("Resource/test.vsb");
	VkShaderModule fragmentShaderModule = CompileShader("Resource/test.fsb");

	VkPipelineShaderStageCreateInfo shaderStages[2] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShaderModule;
	shaderStages[1].pName = "main";

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {};
	descriptorSetLayoutBindings[0].binding = 0;
	descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[0].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr; // for texture

	descriptorSetLayoutBindings[1].binding = 1;
	descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[1].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr; // for texture

	VkDescriptorSetLayout descriptorSetLayout;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

	vkCreateDescriptorSetLayout(vulkanDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &s_pipelineLayout);

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = 2;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

	vkCreateDescriptorPool(vulkanDevice, &descriptorPoolCreateInfo, nullptr, &s_descriptorPool);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = s_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
	if (vkAllocateDescriptorSets(vulkanDevice, &descriptorSetAllocateInfo, &s_descriptorSet) != VK_SUCCESS)
	{
		OutputDebugStringA("Failed to allocate descriptor sets!");
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.renderPass = GetVulkanSwapChainRenderPass();
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.layout = s_pipelineLayout;

	vkCreateGraphicsPipelines(vulkanDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &s_trianglePipeline);

	glm::mat4 modelViewProjection[3];
	modelViewProjection[0] = glm::mat4(1.0f);
	modelViewProjection[1] = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	modelViewProjection[2] = glm::perspective(glm::radians(60.0f), 1280.0f / 720.f, 0.1f, 100.0f);

	s_pUniformBuffer = GenBufferObject(
		sizeof(float) * 16 * 1024,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		sizeof(glm::mat4) * 3,
		modelViewProjection
	);

	s_triangleMesh.InitFromFile("Resource/UnitSphere.obj");
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	BeginSwapChainRenderPass(vulkanCommandBuffer);

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = s_pUniformBuffer->buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(float) * 16 * 1024;

	VkWriteDescriptorSet writeDescriptorSets[1] = {};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].pBufferInfo = &bufferInfo;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].dstSet = s_descriptorSet;

	vkUpdateDescriptorSets(GetVulkanDevice(), 1, writeDescriptorSets, 0, nullptr);

	vkCmdBindPipeline(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_trianglePipeline);

	VkDescriptorSet descriptorSets[1] = { s_descriptorSet };
	vkCmdBindDescriptorSets(
		vulkanCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		s_pipelineLayout,
		0,
		1,
		descriptorSets,
		0,
		nullptr
	);

	s_triangleMesh.Draw(vulkanCommandBuffer);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}