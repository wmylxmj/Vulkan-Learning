#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "StaticMesh.h"

class SceneNode
{
public:
	SceneNode();
	void SetPosition(glm::vec4 position);
	void SetRotation(glm::quat rotation);
	void SetScale(glm::vec4 scale);
	void Draw(VkCommandBuffer commandBuffer, glm::mat4& viewMatrix, glm::mat4& projectionMatrix);
	void GenerateDrawCommand(glm::mat4& viewMatrix, glm::mat4& projectionMatrix);

	StaticMesh* m_staticMesh;
	Buffer* m_uniformBuffer;
	Buffer* m_uniformBuffer1;

	glm::vec4 m_position;
	glm::quat m_rotation;
	glm::vec4 m_scale;
	bool m_needUpdate;
	glm::mat4 m_modelMatrix;
	glm::mat4 m_normalMatrix;

	bool m_isDrawCommandGenerated;
	VkCommandBuffer* m_pCachedDrawCommandBuffer;
};
