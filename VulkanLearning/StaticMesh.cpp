#include "StaticMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

void StaticMesh::Draw(VkCommandBuffer commandBuffer)
{
	VkBuffer vertexBuffers[] = {
		m_pVertexBuffer->buffer
	};

	VkDeviceSize vertexOffsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);

	if (!m_subMeshes.empty()) {
		for (auto& subMesh : m_subMeshes) {
			vkCmdBindIndexBuffer(commandBuffer, subMesh.second->pIndexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, subMesh.second->indexCount, 1, 0, 0, 0);
		}
	}
	else
	{
		vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
	}
}

void StaticMesh::InitFromFile(const char* inFilePath)
{
	Assimp::Importer importer;
	constexpr unsigned int flags = aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace;
	const aiScene* pScene = importer.ReadFile(inFilePath, flags);
	if (pScene == nullptr) {
		OutputDebugStringA(importer.GetErrorString());
		OutputDebugStringA("\n");
		return;
	}
	std::vector<StaticMeshVertexData> vertices;
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		const aiMesh* pMesh = pScene->mMeshes[i];

		uint32_t vertexBase = vertices.size();
		for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
			StaticMeshVertexData vertex;
			vertex.position.x = pMesh->mVertices[i].x;
			vertex.position.y = pMesh->mVertices[i].y;
			vertex.position.z = pMesh->mVertices[i].z;
			vertex.position.w = 1.0f;
			if (pMesh->mTextureCoords[0]) {
				vertex.texcoord.x = pMesh->mTextureCoords[0][i].x;
				vertex.texcoord.y = pMesh->mTextureCoords[0][i].y;
				vertex.texcoord.z = 0.0f;
				vertex.texcoord.w = 0.0f;
			}
			vertex.normal.x = pMesh->mNormals[i].x;
			vertex.normal.y = pMesh->mNormals[i].y;
			vertex.normal.z = pMesh->mNormals[i].z;
			vertex.normal.w = 0.0f;

			if (pMesh->mTangents) {
				vertex.tangent.x = pMesh->mTangents[i].x;
				vertex.tangent.y = pMesh->mTangents[i].y;
				vertex.tangent.z = pMesh->mTangents[i].z;
				vertex.tangent.w = 0.0f;
			}
			vertices.push_back(vertex);
		}

		SubMesh* subMesh = new SubMesh();
		std::vector<uint32_t> indices;
		for (unsigned int i = 0; i < pMesh->mNumFaces; ++i) {
			if (const aiFace face = pMesh->mFaces[i]; face.mNumIndices == 3)
			{
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					indices.push_back(face.mIndices[j] + vertexBase);
				}
			}
		}
		subMesh->indexCount = indices.size();
		subMesh->pIndices = new uint32_t[subMesh->indexCount];
		memcpy(subMesh->pIndices, indices.data(), sizeof(uint32_t) * subMesh->indexCount);
		subMesh->pIndexBuffer = GenBufferObject(
			subMesh->indexCount * sizeof(uint32_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			subMesh->indexCount * sizeof(uint32_t),
			subMesh->pIndices
		);
		m_subMeshes[pMesh->mName.C_Str()] = subMesh;
	}
	SetVertexCount(vertices.size());
	memcpy(m_vertexData, vertices.data(), sizeof(StaticMeshVertexData) * vertices.size());
	m_pVertexBuffer = GenBufferObject(
		m_vertexCount * sizeof(StaticMeshVertexData),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		m_vertexCount * sizeof(StaticMeshVertexData),
		m_vertexData
	);
}