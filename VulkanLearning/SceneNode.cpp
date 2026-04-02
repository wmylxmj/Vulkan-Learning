#include "SceneNode.h"

SceneNode::SceneNode() :
	m_scale(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
	m_needUpdate = true;
	m_isDrawCommandGenerated = false;
	m_pCachedDrawCommandBuffer = nullptr;
}

void SceneNode::SetPosition(glm::vec4 position)
{
	m_position = position;
	m_needUpdate = true;
}

void SceneNode::SetRotation(glm::quat rotation)
{
}

void SceneNode::SetScale(glm::vec4 scale)
{
	m_scale = scale;
	m_needUpdate = true;
}

static bool s_useCachedCommandBuffer = false;

void SceneNode::Draw(VkCommandBuffer commandBuffer, glm::mat4& viewMatrix, glm::mat4& projectionMatrix)
{
	if (m_needUpdate)
	{
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));
		glm::mat4 rotationMatrix = glm::mat4_cast(m_rotation);
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(m_position));
		m_modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
		m_normalMatrix = glm::transpose(glm::inverse(m_modelMatrix));
		if (m_uniformBuffer == nullptr)
		{
			m_uniformBuffer = GenBufferObject(
				sizeof(glm::mat4) * 1024,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			m_staticMesh->m_material.SetUniformBuffer(0, m_uniformBuffer->buffer, sizeof(glm::mat4) * 1024);
		}
		if (m_uniformBuffer1 == nullptr)
		{
			m_uniformBuffer1 = GenBufferObject(
				sizeof(glm::mat4) * 1024,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			m_staticMesh->m_material.SetUniformBuffer(1, m_uniformBuffer1->buffer, sizeof(glm::mat4) * 1024);
		}
		{
			VkDevice device = GetVulkanDevice();
			void* pMemory = nullptr;
			vkMapMemory(device, m_uniformBuffer->memory, 0, sizeof(glm::mat4) * 1024, 0, &pMemory);
			memcpy(pMemory, &m_modelMatrix, sizeof(glm::mat4));
			memcpy((glm::mat4*)pMemory + 1, &m_normalMatrix, sizeof(glm::mat4));
			vkUnmapMemory(device, m_uniformBuffer->memory);
		}
		{
			float uniformBuffer1Data[] = {
				1.0f, 0.0f, 0.0f, 0.0f,
				-1.0f, 0.0f, 0.0f, 0.0f,
				1.0f, 0.0f, 0.0f, 0.0f,
			};
			VkDevice device = GetVulkanDevice();
			void* pMemory = nullptr;
			vkMapMemory(device, m_uniformBuffer1->memory, 0, sizeof(glm::mat4) * 1024, 0, &pMemory);
			memcpy(pMemory, &uniformBuffer1Data, sizeof(uniformBuffer1Data));
			vkUnmapMemory(device, m_uniformBuffer1->memory);
		}

		m_needUpdate = false;
	}

	if (s_useCachedCommandBuffer) {
		GenerateDrawCommand(viewMatrix, projectionMatrix);
	}
	else {
		if (m_staticMesh != nullptr)
		{
			ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();

			m_staticMesh->m_material.Activate(commandBuffer, pShaderParameterDescription->pipelineLayout);
			m_staticMesh->Draw(commandBuffer);
		}
	}
}

void SceneNode::GenerateDrawCommand(glm::mat4& viewMatrix, glm::mat4& projectionMatrix)
{
	if (m_isDrawCommandGenerated) return;

	m_isDrawCommandGenerated = true;
	m_pCachedDrawCommandBuffer = new VkCommandBuffer[2];

	VkFramebuffer* pSwapChainFramebuffers = GetSwapChainFrameBuffers();
	ShaderParameterDescription* pShaderParameterDescription = GetUberPassShaderParameterDescription();

	for (int i = 0; i < 2; ++i) {
		m_pCachedDrawCommandBuffer[i] = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		VkCommandBufferInheritanceInfo commandBufferInheritanceInfo = {};
		commandBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		commandBufferInheritanceInfo.framebuffer = pSwapChainFramebuffers[i];
		commandBufferInheritanceInfo.renderPass = GetVulkanSwapChainRenderPass();
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;
		vkBeginCommandBuffer(m_pCachedDrawCommandBuffer[i], &commandBufferBeginInfo);

		vkCmdPushConstants(m_pCachedDrawCommandBuffer[i], pShaderParameterDescription->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0, sizeof(glm::mat4), &viewMatrix);
		vkCmdPushConstants(m_pCachedDrawCommandBuffer[i], pShaderParameterDescription->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(glm::mat4), sizeof(glm::mat4), &projectionMatrix);

		if (m_staticMesh != nullptr)
		{
			m_staticMesh->m_material.Activate(m_pCachedDrawCommandBuffer[i], pShaderParameterDescription->pipelineLayout);
			m_staticMesh->Draw(m_pCachedDrawCommandBuffer[i]);
		}

		vkEndCommandBuffer(m_pCachedDrawCommandBuffer[i]);
	}
}