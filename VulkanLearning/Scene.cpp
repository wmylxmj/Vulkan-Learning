#include "Scene.h"
#include "VulkanUtils.h"
#include "StaticMesh.h"
#include "SceneNode.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>
#include <thread>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

SceneNode* s_pSphereNode = nullptr;

glm::mat4 s_viewMatrix;
glm::mat4 s_projectionMatrix;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
	s_viewMatrix = glm::lookAt(
		glm::vec3(200.0f, 200.0f, 200.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	s_projectionMatrix = glm::perspective(glm::radians(60.0f), 1280.0f / 720.f, 0.1f, 1000.0f);

	StaticMesh::Initialize();
	VkDevice vulkanDevice = GetVulkanDevice();

	stbi_set_flip_vertically_on_load(true);
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;
	void* pixelData = stbi_load(
		"Resource/Models/Planet/Texture/Planet_Diffuse.png",
		&imageWidth, &imageHeight, &imageChannels, 4
	);
	int imageSize = imageWidth * imageHeight * 4;

	Texture* pTexture = new Texture[1];
	pTexture->format = VK_FORMAT_R8G8B8A8_UNORM;
	pTexture->aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	GenImage(
		pTexture,
		imageWidth,
		imageHeight,
		pTexture->format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VkCommandBuffer commandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// 资源状态转换 -> 转换为传输目标状态
	TransferImageLayout(
		commandBuffer, pTexture->image,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);
	// 创建上传堆
	Buffer* pUploadBuffer = GenBufferObject(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);

	// 将数据拷贝到上传堆
	void* pMemoryData = nullptr;
	vkMapMemory(vulkanDevice, pUploadBuffer->memory, 0, imageSize, 0, &pMemoryData);
	memcpy(pMemoryData, pixelData, imageSize);
	vkUnmapMemory(vulkanDevice, pUploadBuffer->memory);

	// 将上传堆数据拷贝到显存
	SubmitBufferDataToImage(
		commandBuffer,
		pUploadBuffer->buffer, pTexture->image,
		imageWidth, imageHeight
	);

	// 资源状态转换 -> 转换为采样状态
	TransferImageLayout(
		commandBuffer, pTexture->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);

	vkEndCommandBuffer(commandBuffer);

	VkFence fence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence);
	vkResetFences(vulkanDevice, 1, &fence);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(GetGraphicsQueue(), 1, &submitInfo, fence);
	VkResult result = vkWaitForFences(vulkanDevice, 1, &fence, true, UINT64_MAX);
	if (result == VK_SUCCESS)
	{
		OutputDebugStringA("Upload image to texture success.\n");
	}
	vkDestroyBuffer(vulkanDevice, pUploadBuffer->buffer, nullptr);
	vkFreeMemory(vulkanDevice, pUploadBuffer->memory, nullptr);

	pTexture->imageView = GenImageView2D(
		pTexture->image,
		pTexture->format,
		pTexture->aspectFlags
	);

	VkSampler sampler = nullptr;
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.anisotropyEnable = false;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	vkCreateSampler(vulkanDevice, &samplerCreateInfo, nullptr, &sampler);

	s_pSphereNode = new SceneNode();
	s_pSphereNode->m_staticMesh = new StaticMesh();
	s_pSphereNode->m_staticMesh->InitFromFile("Resource/Models/Planet/Planet.obj");
	s_pSphereNode->m_staticMesh->m_material.Init("Resource/test.vsb", "Resource/test.fsb");
	s_pSphereNode->m_staticMesh->m_material.SetTexture(2, pTexture->imageView, sampler);
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	uint32_t swapChainFrameIndex = BeginSwapChainRenderPass(vulkanCommandBuffer);

	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();
	vkCmdPushConstants(vulkanCommandBuffer, pShaderParameterDescription->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0, sizeof(glm::mat4), &s_viewMatrix);
	vkCmdPushConstants(vulkanCommandBuffer, pShaderParameterDescription->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(glm::mat4), sizeof(glm::mat4), &s_projectionMatrix);

	s_pSphereNode->Draw(vulkanCommandBuffer, s_viewMatrix, s_projectionMatrix);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}