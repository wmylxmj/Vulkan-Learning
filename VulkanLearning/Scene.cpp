#include "Scene.h"
#include "VulkanUtils.h"

VkPipeline s_trianglePipeline = nullptr;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
	VkDevice vulkanDevice = GetVulkanDevice();

	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0; // slot 0
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per-vertex data
	vertexInputBindingDescription.stride = sizeof(float) * 4 * 4;

	VkVertexInputAttributeDescription vertexInputAttributeDescriptions[4] = {};
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

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 4;
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
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // ±³ÃæÌÞ³ý

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.renderPass = GetVulkanSwapChainRenderPass();
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;

	vkCreateGraphicsPipelines(vulkanDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &s_trianglePipeline);
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	BeginSwapChainRenderPass(vulkanCommandBuffer);

	vkCmdBindPipeline(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_trianglePipeline);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}