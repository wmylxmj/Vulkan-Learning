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

	SubmitTextureData(pTexture->image, pixelData, imageWidth, imageHeight, imageSize);

	pTexture->imageView = GenImageView2D(
		pTexture->image,
		pTexture->format,
		pTexture->aspectFlags
	);

	VkSampler sampler = GenSampler();
	//
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