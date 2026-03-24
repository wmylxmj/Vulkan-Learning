#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

#include "VulkanUtils.h"

struct StaticMeshVertexData
{
	glm::vec4 position;
	glm::vec4 texcoord;
	glm::vec4 normal;
	glm::vec4 tangent;
};

struct SubMesh
{
	uint32_t* pIndices;
	uint32_t indexCount;
	Buffer* pIndexBuffer;
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

	void Draw(VkCommandBuffer commandBuffer);

	void InitFromFile(const char* inFilePath);

	StaticMeshVertexData* m_vertexData;
	uint32_t m_vertexCount;
	std::unordered_map<std::string, SubMesh*> m_subMeshes;

	Buffer* m_pVertexBuffer;
};
