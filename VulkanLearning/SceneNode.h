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
	void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	StaticMesh* m_staticMesh;
	Buffer* m_uniformBuffer;

	glm::vec4 m_position;
	glm::quat m_rotation;
	glm::vec4 m_scale;
	bool m_needUpdate;
	glm::mat4 m_modelMatrix;
	glm::mat4 m_normalMatrix;
};
