#include "SceneNode.h"

SceneNode::SceneNode() :
	m_scale(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
	m_needUpdate = true;
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
}

void SceneNode::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
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
			float scaleX[] = { 0.5f, 0.0f, 0.0f, 0.0f };
			VkDevice device = GetVulkanDevice();
			void* pMemory = nullptr;
			vkMapMemory(device, m_uniformBuffer1->memory, 0, sizeof(glm::mat4) * 1024, 0, &pMemory);
			memcpy(pMemory, &scaleX, sizeof(glm::vec4));
			vkUnmapMemory(device, m_uniformBuffer1->memory);
		}

		m_needUpdate = false;
	}

	if (m_staticMesh != nullptr)
	{
		m_staticMesh->m_material.Activate(commandBuffer, pipelineLayout);
		m_staticMesh->Draw(commandBuffer);
	}
}