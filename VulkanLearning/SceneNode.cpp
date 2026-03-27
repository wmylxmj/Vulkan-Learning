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
		}
		{
			VkDevice device = GetVulkanDevice();
			void* pMemory = nullptr;
			vkMapMemory(device, m_uniformBuffer->memory, 0, sizeof(glm::mat4) * 1024, 0, &pMemory);
			memcpy(pMemory, &m_modelMatrix, sizeof(glm::mat4));
			memcpy((glm::mat4*)pMemory + 1, &m_normalMatrix, sizeof(glm::mat4));
			vkUnmapMemory(device, m_uniformBuffer->memory);
		}

		m_needUpdate = false;
	}

	if (m_staticMesh != nullptr)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_uniformBuffer->buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(glm::mat4) * 1024;

		VkWriteDescriptorSet writeDescriptorSets[1] = {};

		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].pBufferInfo = &bufferInfo;
		writeDescriptorSets[0].dstArrayElement = 0;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].dstSet = m_staticMesh->m_material.m_descriptorSet;

		vkUpdateDescriptorSets(GetVulkanDevice(), 1, writeDescriptorSets, 0, nullptr);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&m_staticMesh->m_material.m_descriptorSet,
			0,
			nullptr
		);

		m_staticMesh->Draw(commandBuffer);
	}
}