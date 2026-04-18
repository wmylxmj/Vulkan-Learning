#include "Scene.h"
#include "VulkanUtils.h"
#include "StaticMesh.h"
#include "SceneNode.h"
#include "Framebuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>
#include <thread>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

SceneNode* s_pSphereNode = nullptr;
SceneNode* s_pFullScreenQuadNode = nullptr;
Texture* s_pSkyboxTexture = nullptr;

glm::mat4 s_viewMatrix;
glm::mat4 s_projectionMatrix;

FrameBuffer* s_pFrameBuffer = nullptr;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
	s_viewMatrix = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	s_projectionMatrix = glm::perspective(glm::radians(60.0f), 1280.0f / 720.f, 0.1f, 1000.0f);

	StaticMesh::Initialize();
	VkDevice vulkanDevice = GetVulkanDevice();

	stbi_set_flip_vertically_on_load(true);
	Texture2D* pDiffuseTexture = LoadTexture2DFromFile("Resource/Models/Planet/Texture/Planet_Diffuse.png");

	const char* imagePaths[] = {
		"Resource/skybox/right.jpg",
		"Resource/skybox/left.jpg",
		"Resource/skybox/top.jpg",
		"Resource/skybox/bottom.jpg",
		"Resource/skybox/front.jpg",
		"Resource/skybox/back.jpg"
	};
	stbi_set_flip_vertically_on_load(false);
	s_pSkyboxTexture = LoadTextureCubeMapFromFile(imagePaths);

	VkSampler sampler = GenSampler();
	//
	s_pSphereNode = new SceneNode();
	s_pSphereNode->m_staticMesh = new StaticMesh();
	//s_pSphereNode->m_staticMesh->InitFromFile("Resource/Models/Planet/Planet.obj");
	s_pSphereNode->m_staticMesh->SetVertexCount(4);
	s_pSphereNode->m_staticMesh->SetPosition(0, glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));
	s_pSphereNode->m_staticMesh->SetNormal(0, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	s_pSphereNode->m_staticMesh->SetPosition(1, glm::vec4(0.5f, -0.5f, 0.0f, 1.0f));
	s_pSphereNode->m_staticMesh->SetNormal(1, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	s_pSphereNode->m_staticMesh->SetPosition(2, glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f));
	s_pSphereNode->m_staticMesh->SetNormal(2, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	s_pSphereNode->m_staticMesh->SetPosition(3, glm::vec4(0.5f, 0.5f, 0.0f, 1.0f));
	s_pSphereNode->m_staticMesh->SetNormal(3, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	s_pSphereNode->m_staticMesh->m_pVertexBuffer = GenBufferObject(
		s_pSphereNode->m_staticMesh->m_vertexCount * sizeof(StaticMeshVertexData),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		s_pSphereNode->m_staticMesh->m_vertexCount * sizeof(StaticMeshVertexData),
		s_pSphereNode->m_staticMesh->m_vertexData
	);

	s_pSphereNode->m_staticMesh->m_material.InitVTF(
		"Resource/tstest.vsb",
		"Resource/tstest.tcsb",
		"Resource/tstest.tesb",
		"Resource/tstest.fsb"
	);
	s_pSphereNode->m_staticMesh->m_material.SetTexture2D(2, 0, pDiffuseTexture->imageView, sampler);
	s_pSphereNode->m_staticMesh->m_material.SetTexture2D(3, 0, s_pSkyboxTexture->imageView, sampler);

	s_pFrameBuffer = new FrameBuffer();
	s_pFrameBuffer->InitWithSize(1280, 720);

	s_pFullScreenQuadNode = new SceneNode();
	s_pFullScreenQuadNode->m_staticMesh = new StaticMesh();
	s_pFullScreenQuadNode->m_staticMesh->SetVertexCount(4);
	s_pFullScreenQuadNode->m_staticMesh->SetPosition(0, glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetTexCoord(0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetPosition(1, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetTexCoord(1, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetPosition(2, glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetTexCoord(2, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetPosition(3, glm::vec4(1.0f, -1.0f, 0.0f, 1.0f));
	s_pFullScreenQuadNode->m_staticMesh->SetTexCoord(3, glm::vec4(1.0f, 1.0f, 0.0f, 0.0f));
	s_pFullScreenQuadNode->m_staticMesh->m_pVertexBuffer = GenBufferObject(
		s_pFullScreenQuadNode->m_staticMesh->m_vertexCount * sizeof(StaticMeshVertexData),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		s_pFullScreenQuadNode->m_staticMesh->m_vertexCount * sizeof(StaticMeshVertexData),
		s_pFullScreenQuadNode->m_staticMesh->m_vertexData
	);
	s_pFullScreenQuadNode->m_staticMesh->m_material.Init(
		"Resource/fsq.vsb",
		"Resource/fsq.fsb"
	);
	s_pFullScreenQuadNode->m_staticMesh->m_material.m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	s_pFullScreenQuadNode->m_staticMesh->m_material.SetTexture2D(2, 0, s_pFrameBuffer->m_colorRenderTarget->imageView, sampler);
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();
	vkCmdPushConstants(vulkanCommandBuffer, pShaderParameterDescription->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0, sizeof(glm::mat4), &s_viewMatrix);
	vkCmdPushConstants(vulkanCommandBuffer, pShaderParameterDescription->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(glm::mat4), sizeof(glm::mat4), &s_projectionMatrix);

	s_pFrameBuffer->BeginRender(vulkanCommandBuffer);

	s_pSphereNode->Draw(vulkanCommandBuffer, s_pFrameBuffer->m_renderPass, s_viewMatrix, s_projectionMatrix);

	vkCmdEndRenderPass(vulkanCommandBuffer);

	uint32_t swapChainFrameIndex = BeginSwapChainRenderPass(vulkanCommandBuffer);
	s_pFullScreenQuadNode->Draw(vulkanCommandBuffer, GetVulkanSwapChainRenderPass(), s_viewMatrix, s_projectionMatrix);
	EndSwapChainRenderPass(vulkanCommandBuffer);
}