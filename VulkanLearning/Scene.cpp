#include "Scene.h"
#include "VulkanUtils.h"
#include "StaticMesh.h"
#include "SceneNode.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>
#include <thread>
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

	s_pSphereNode = new SceneNode();
	s_pSphereNode->m_staticMesh = new StaticMesh();
	s_pSphereNode->m_staticMesh->InitFromFile("Resource/Models/Planet/Planet.obj");
	s_pSphereNode->m_staticMesh->m_material.Init("Resource/test.vsb", "Resource/test.fsb");

	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;
	void* pixelData = stbi_load(
		"Resource/Models/Planet/Texture/Planet_Diffuse.png",
		&imageWidth, &imageHeight, &imageChannels, 4
	);
	VkDeviceSize imageSize = imageWidth * imageHeight * 4;

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
	{
		VkBufferImageCopy bufferImageCopy = {};
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageSubresource.mipLevel = 0;

		bufferImageCopy.imageOffset = { 0, 0, 0 };
		bufferImageCopy.imageExtent = { (uint32_t)imageWidth, (uint32_t)imageHeight, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer, pUploadBuffer->buffer, pTexture->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy
		);
	}

	// 资源状态转换 -> 转换为采样状态

	vkEndCommandBuffer(commandBuffer);
	/*
	// Submit Command Buffer
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &inCommandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &s_readyToRenderSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &s_readyToPresentSemaphore;
	vkQueueSubmit(s_vulkanGraphicsQueue, 1, &submitInfo, nullptr);

	// Present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &s_readyToPresentSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &s_vulkanSwapchain;
	presentInfo.pImageIndices = &s_currentFrameBufferToRenderIndex;
	vkQueuePresentKHR(s_vulkanPresentQueue, &presentInfo);
	vkQueueWaitIdle(s_vulkanPresentQueue);

	s_currentFrameBufferToRenderIndex = (s_currentFrameBufferToRenderIndex + 1) % s_vulkanSwapchainImageCount;

	vkFreeCommandBuffers(s_vulkanDevice, s_vulkanCommandPool, 1, &inCommandBuffer);
	*/

	pTexture->imageView = GenImageView2D(
		pTexture->image,
		pTexture->format,
		pTexture->aspectFlags
	);
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