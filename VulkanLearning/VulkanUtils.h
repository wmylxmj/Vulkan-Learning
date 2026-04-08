#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#ifdef _WIN32
#include <Windows.h>
struct InitVulkanUserData
{
	HINSTANCE hInstance;
	HWND hWnd;
};
#endif

struct Texture
{
	Texture() :
		image(VK_NULL_HANDLE),
		memory(VK_NULL_HANDLE),
		imageView(VK_NULL_HANDLE),
		aspectFlags(VK_IMAGE_ASPECT_NONE),
		format(VK_FORMAT_UNDEFINED) {
	}

	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageView;
	VkImageAspectFlags aspectFlags;
	VkFormat format;
};

struct Texture2D : public Texture
{
	Texture2D() : Texture() {
		width = 0;
		height = 0;
		numChannels = 0;
	}

	uint32_t width;
	uint32_t height;
	uint32_t numChannels;
};

struct Buffer {
	Buffer();
	~Buffer();
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct ShaderParameterDescription {
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
};

bool InitVulkan(void* inUserData, int inWidth, int inHeight);

VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel inCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

void BeginCommandBuffer(VkCommandBuffer inCommandBuffer, VkCommandBufferUsageFlags inUsageFlags);
uint32_t BeginSwapChainRenderPass(VkCommandBuffer inCommandBuffer);
void EndSwapChainRenderPass(VkCommandBuffer inCommandBuffer);

VkQueue GetGraphicsQueue();
VkDevice GetVulkanDevice();
VkPhysicalDevice GetVulkanPhysicalDevice();
VkRenderPass GetVulkanSwapChainRenderPass();

Buffer* GenBufferObject(
	VkDeviceSize inBufferSize,
	VkBufferUsageFlags inUsageFlags,
	VkMemoryPropertyFlagBits inMemoryPropertyFlagBits,
	size_t inDataSize = 0,
	const void* inData = nullptr
);

ShaderParameterDescription* GetUberPassShaderParameterDescription();

VkPipeline CreatePipeline(
	const std::vector<VkVertexInputBindingDescription>& inVertexInputBindingDescriptions,
	const std::vector<VkVertexInputAttributeDescription>& inVertexInputAttributeDescriptions,
	const VkShaderModule inVertexShaderModule,
	const VkShaderModule inFragmentShaderModule
);

VkShaderModule CompileShader(const char* inFilePath);

VkFramebuffer* GetSwapChainFrameBuffers();

void GenImage(
	Texture* inOutTexture,
	uint32_t inWidth,
	uint32_t inHeight,
	VkFormat inFormat,
	VkImageUsageFlags inUsageFlags,
	VkMemoryPropertyFlagBits inMemoryPropertyFlagBits
);
VkImageView GenImageView2D(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inAspectFlags);

void TransferImageLayout(
	VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inOldPipelineStage,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inNewPipelineStage
);

void SubmitBufferDataToImage(
	VkCommandBuffer inCommandBuffer,
	VkBuffer inBuffer, VkImage inImage,
	uint32_t inImageWidth, uint32_t inImageHeight,
	uint32_t inFaceIndex
);

void SubmitTextureData(
	VkImage inTargetImage, const void* inPixelData,
	uint32_t inImageWidth, uint32_t inImageHeight, uint32_t inImageSizeInBytes
);

VkSampler GenSampler(
	VkFilter inMinFilter = VK_FILTER_LINEAR,
	VkFilter inMagFilter = VK_FILTER_LINEAR,
	VkSamplerAddressMode inWrapModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	VkSamplerAddressMode inWrapModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	VkSamplerAddressMode inWrapModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
);

Texture2D* LoadTexture2DFromFile(const char* inFilePath);

void GenImageCubeMap(
	Texture* inOutTexture,
	uint32_t inWidth,
	uint32_t inHeight,
	VkFormat inFormat,
	VkImageUsageFlags inUsageFlags,
	VkMemoryPropertyFlagBits inMemoryPropertyFlagBits
);

VkImageView GenImageViewCubeMap(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inAspectFlags);

void SubmitCubeMapData(
	VkImage inTargetImage, void** inPixelData,
	uint32_t inImageWidth, uint32_t inImageHeight, uint32_t inImageSizeInBytes
);
