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

void SceneNode::Draw(VkCommandBuffer commandBuffer)
{
	if (m_needUpdate)
	{
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));
		glm::mat4 rotationMatrix = glm::mat4_cast(m_rotation);
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(m_position));
		m_modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
		m_normalMatrix = glm::transpose(glm::inverse(m_modelMatrix));
	}
	if (m_staticMesh != nullptr)
	{
		m_staticMesh->Draw(commandBuffer);
	}
}