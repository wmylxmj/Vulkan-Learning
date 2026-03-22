#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct StaticMeshVertexData
{
	glm::vec4 position;
	glm::vec4 texcoord;
	glm::vec4 normal;
	glm::vec4 tangent;
};

class StaticMesh
{
public:
	static void Initialize();
	static std::vector<VkVertexInputBindingDescription> sm_vertexInputBindingDescriptions;
	static std::vector<VkVertexInputAttributeDescription> sm_vertexInputAttributeDescriptions;

	void SetVertexCount(uint32_t vertexCount);
	void SetPosition(uint32_t index, glm::vec4 position);
	void SetTexCoord(uint32_t index, glm::vec4 texcoord);
	void SetNormal(uint32_t index, glm::vec4 normal);
	void SetTangent(uint32_t index, glm::vec4 tangent);

	StaticMeshVertexData* m_vertexData;
	uint32_t m_vertexCount;
};
