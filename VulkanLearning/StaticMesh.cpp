#include "StaticMesh.h"

std::vector<VkVertexInputBindingDescription> StaticMesh::sm_vertexInputBindingDescriptions;
std::vector<VkVertexInputAttributeDescription> StaticMesh::sm_vertexInputAttributeDescriptions;

void StaticMesh::Initialize()
{
	sm_vertexInputBindingDescriptions.resize(1);
	sm_vertexInputBindingDescriptions[0].binding = 0;
	sm_vertexInputBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per-vertex data
	sm_vertexInputBindingDescriptions[0].stride = sizeof(StaticMeshVertexData);

	sm_vertexInputAttributeDescriptions.resize(4);
	sm_vertexInputAttributeDescriptions[0].binding = 0; // slot 0
	sm_vertexInputAttributeDescriptions[0].location = 0; // location 0 in shader
	sm_vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	sm_vertexInputAttributeDescriptions[0].offset = 0; // position data starts at offset 0
	sm_vertexInputAttributeDescriptions[1].binding = 0; // slot 0
	sm_vertexInputAttributeDescriptions[1].location = 1; // location 1 in shader
	sm_vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	sm_vertexInputAttributeDescriptions[1].offset = sizeof(float) * 4; // color data starts after position data (vec4)
	sm_vertexInputAttributeDescriptions[2].binding = 0; // slot 0
	sm_vertexInputAttributeDescriptions[2].location = 2; // location 2 in shader
	sm_vertexInputAttributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	sm_vertexInputAttributeDescriptions[2].offset = sizeof(float) * 4 * 2; // normal data starts after position and color data (vec4 + vec4)
	sm_vertexInputAttributeDescriptions[3].binding = 0; // slot 0
	sm_vertexInputAttributeDescriptions[3].location = 3; // location 3 in shader
	sm_vertexInputAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; // vec4
	sm_vertexInputAttributeDescriptions[3].offset = sizeof(float) * 4 * 3; // uv data starts after position, color and normal data (vec4 + vec4 + vec4)
}

void StaticMesh::SetVertexCount(uint32_t vertexCount)
{
	m_vertexCount = vertexCount;
	m_vertexData = new StaticMeshVertexData[vertexCount];
}

void StaticMesh::SetPosition(uint32_t index, glm::vec4 position)
{
	m_vertexData[index].position = position;
}

void StaticMesh::SetTexCoord(uint32_t index, glm::vec4 texcoord)
{
	m_vertexData[index].texcoord = texcoord;
}

void StaticMesh::SetNormal(uint32_t index, glm::vec4 normal)
{
	m_vertexData[index].normal = normal;
}

void StaticMesh::SetTangent(uint32_t index, glm::vec4 tangent)
{
	m_vertexData[index].tangent = tangent;
}