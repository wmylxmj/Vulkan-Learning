#include "Scene.h"
#include "VulkanUtils.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <cstdio>
#include <string>

VkBuffer s_vertexBuffer = nullptr;
VkDeviceMemory s_vertexBufferMemory = nullptr;

VkBuffer s_colorVertexBuffer = nullptr;
VkDeviceMemory s_colorVertexBufferMemory = nullptr;

VkBuffer s_uniformBuffer = nullptr;
VkDeviceMemory s_uniformBufferMemory = nullptr;

VkPipeline s_trianglePipeline = nullptr;
VkPipelineLayout s_pipelineLayout = nullptr;

VkDescriptorSet s_descriptorSet = nullptr;
VkDescriptorPool s_descriptorPool = nullptr;

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
	VkDevice vulkanDevice = GetVulkanDevice();

	VkVertexInputBindingDescription vertexInputBindingDescription[2] = {};
	vertexInputBindingDescription[0].binding = 0; // slot 0
	vertexInputBindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per-vertex data
	vertexInputBindingDescription[0].stride = sizeof(float) * 4 * 4;
	// color vbo
	vertexInputBindingDescription[1].binding = 1; // slot 1
	vertexInputBindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per-vertex data
	vertexInputBindingDescription[1].stride = sizeof(float) * 4 * 1;

	VkVertexInputAttributeDescription vertexInputAttributeDescriptions[5] = {};
	vertexInputAttributeDescriptions[0].binding = 0; // slot 0
	vertexInputAttributeDescriptions[0].location = 0; // location 0 in shader
	vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	vertexInputAttributeDescriptions[0].offset = 0; // position data starts at offset 0
	vertexInputAttributeDescriptions[1].binding = 0; // slot 0
	vertexInputAttributeDescriptions[1].location = 1; // location 1 in shader
	vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	vertexInputAttributeDescriptions[1].offset = sizeof(float) * 4; // color data starts after position data (vec4)
	vertexInputAttributeDescriptions[2].binding = 0; // slot 0
	vertexInputAttributeDescriptions[2].location = 2; // location 2 in shader
	vertexInputAttributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	vertexInputAttributeDescriptions[2].offset = sizeof(float) * 4 * 2; // normal data starts after position and color data (vec4 + vec4)
	vertexInputAttributeDescriptions[3].binding = 0; // slot 0
	vertexInputAttributeDescriptions[3].location = 3; // location 3 in shader
	vertexInputAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	vertexInputAttributeDescriptions[3].offset = sizeof(float) * 4 * 3; // uv data starts after position, color and normal data (vec4 + vec4 + vec4)

	vertexInputAttributeDescriptions[4].binding = 1; // slot 1
	vertexInputAttributeDescriptions[4].location = 4; // location 4 in shader
	vertexInputAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	vertexInputAttributeDescriptions[4].offset = 0; // position data starts at offset 0

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 2;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;

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

	float position[] = {
		-0.5f, -0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,

		0.0f, 0.5f, 0.0f, 1.0f,
		0.0f, 0.5f, 0.0f, 1.0f,
		0.0f, 0.5f, 0.0f, 1.0f,
		0.0f, 0.5f, 0.0f, 1.0f,

		0.5f, -0.5f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f
	};

	float colors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
	};

	{
		VkBufferCreateInfo vertexBufferCreateInfo = {};
		vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCreateInfo.size = sizeof(position);
		vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(vulkanDevice, &vertexBufferCreateInfo, nullptr, &s_vertexBuffer) != VK_SUCCESS) {
			OutputDebugStringA("Failed to create vertex buffer!\n");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(vulkanDevice, s_vertexBuffer, &memoryRequirements);
		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(GetVulkanPhysicalDevice(), &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
				(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) // 上传堆
			{
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		vkAllocateMemory(vulkanDevice, &memoryAllocateInfo, nullptr, &s_vertexBufferMemory);
		vkBindBufferMemory(vulkanDevice, s_vertexBuffer, s_vertexBufferMemory, 0);

		void* pMemory;
		vkMapMemory(vulkanDevice, s_vertexBufferMemory, 0, sizeof(position), 0, &pMemory);
		memcpy(pMemory, position, sizeof(position));
		vkUnmapMemory(vulkanDevice, s_vertexBufferMemory);
	}
	{
		VkBufferCreateInfo vertexBufferCreateInfo = {};
		vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCreateInfo.size = sizeof(colors);
		vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(vulkanDevice, &vertexBufferCreateInfo, nullptr, &s_colorVertexBuffer) != VK_SUCCESS) {
			OutputDebugStringA("Failed to create color vertex buffer!\n");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(vulkanDevice, s_colorVertexBuffer, &memoryRequirements);
		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(GetVulkanPhysicalDevice(), &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
				(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) // 上传堆
			{
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		vkAllocateMemory(vulkanDevice, &memoryAllocateInfo, nullptr, &s_colorVertexBufferMemory);
		vkBindBufferMemory(vulkanDevice, s_colorVertexBuffer, s_colorVertexBufferMemory, 0);

		void* pMemory;
		vkMapMemory(vulkanDevice, s_colorVertexBufferMemory, 0, sizeof(colors), 0, &pMemory);
		memcpy(pMemory, colors, sizeof(colors));
		vkUnmapMemory(vulkanDevice, s_colorVertexBufferMemory);
	}

	{
		VkBufferCreateInfo uniformBufferCreateInfo = {};
		uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		uniformBufferCreateInfo.size = sizeof(float) * 16 * 1024;
		uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (vkCreateBuffer(vulkanDevice, &uniformBufferCreateInfo, nullptr, &s_uniformBuffer) != VK_SUCCESS) {
			OutputDebugStringA("Failed to create uniform buffer!\n");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(vulkanDevice, s_uniformBuffer, &memoryRequirements);
		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(GetVulkanPhysicalDevice(), &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
				(memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) // 上传堆
			{
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		vkAllocateMemory(vulkanDevice, &memoryAllocateInfo, nullptr, &s_uniformBufferMemory);
		vkBindBufferMemory(vulkanDevice, s_uniformBuffer, s_uniformBufferMemory, 0);

		void* pMemory;
		vkMapMemory(vulkanDevice, s_uniformBufferMemory, 0, sizeof(float) * 16 * 3, 0, &pMemory);
		glm::mat4 modelViewProjection[3];
		modelViewProjection[0] = glm::mat4(1.0f);
		modelViewProjection[1] = glm::lookAt(
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		modelViewProjection[2] = glm::perspective(glm::radians(60.0f), 1280.0f / 720.f, 0.1f, 100.0f);
		memcpy(pMemory, modelViewProjection, sizeof(glm::mat4) * 3);
		vkUnmapMemory(vulkanDevice, s_uniformBufferMemory);
	}
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	BeginSwapChainRenderPass(vulkanCommandBuffer);

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = s_uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(float) * 16 * 1024;

	VkWriteDescriptorSet writeDescriptorSets[2] = {};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].pBufferInfo = &bufferInfo;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].dstSet = s_descriptorSet;

	writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[1].pBufferInfo = &bufferInfo;
	writeDescriptorSets[1].dstArrayElement = 0;
	writeDescriptorSets[1].dstBinding = 1;
	writeDescriptorSets[1].dstSet = s_descriptorSet;

	vkUpdateDescriptorSets(GetVulkanDevice(), 2, writeDescriptorSets, 0, nullptr);

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

	VkBuffer vertexBuffers[] = {
		s_vertexBuffer, s_colorVertexBuffer
	};

	VkDeviceSize vertexOffsets[] = { 0, 0 };
	vkCmdBindVertexBuffers(vulkanCommandBuffer, 0, 2, vertexBuffers, vertexOffsets);

	vkCmdDraw(vulkanCommandBuffer, 3, 1, 0, 0);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}