#include "Scene.h"
#include "VulkanUtils.h"
#include "StaticMesh.h"
#include "SceneNode.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>

SceneNode* s_pSphereNode = nullptr;

glm::mat4 s_viewMatrix;
glm::mat4 s_projectionMatrix;

VkCommandBuffer s_pushConstantsCommandBuffers[2];

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
	s_viewMatrix = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	s_projectionMatrix = glm::perspective(glm::radians(60.0f), 1280.0f / 720.f, 0.1f, 100.0f);

	VkFramebuffer* pSwapChainFramebuffers = GetSwapChainFrameBuffers();

	for (int i = 0; i < 2; ++i) {
		s_pushConstantsCommandBuffers[i] = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		VkCommandBufferInheritanceInfo commandBufferInheritanceInfo = {};
		commandBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		commandBufferInheritanceInfo.framebuffer = pSwapChainFramebuffers[i];
		commandBufferInheritanceInfo.renderPass = GetVulkanSwapChainRenderPass();
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;
		vkBeginCommandBuffer(s_pushConstantsCommandBuffers[i], &commandBufferBeginInfo);

		ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();
		vkCmdPushConstants(s_pushConstantsCommandBuffers[i], pShaderParameterDescription->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0, sizeof(glm::mat4), &s_viewMatrix);
		vkCmdPushConstants(s_pushConstantsCommandBuffers[i], pShaderParameterDescription->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(glm::mat4), sizeof(glm::mat4), &s_projectionMatrix);
		vkEndCommandBuffer(s_pushConstantsCommandBuffers[i]);
	}

	StaticMesh::Initialize();
	VkDevice vulkanDevice = GetVulkanDevice();

	s_pSphereNode = new SceneNode();
	s_pSphereNode->m_staticMesh = new StaticMesh();
	s_pSphereNode->m_staticMesh->InitFromFile("Resource/UnitSphere.obj");
	s_pSphereNode->m_staticMesh->m_material.Init("Resource/test.vsb", "Resource/test.fsb");
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	uint32_t swapChainFrameIndex = BeginSwapChainRenderPass(vulkanCommandBuffer);

	s_pSphereNode->Draw(vulkanCommandBuffer, s_viewMatrix, s_projectionMatrix);
	vkCmdExecuteCommands(vulkanCommandBuffer, 1, &s_pSphereNode->m_pCachedDrawCommandBuffer[swapChainFrameIndex]);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}