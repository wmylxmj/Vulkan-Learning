#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

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

struct Buffer {
	Buffer();
	~Buffer();
	VkBuffer buffer;
	VkDeviceMemory memory;
};

bool InitVulkan(void* inUserData, int inWidth, int inHeight);

VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel inCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

void BeginCommandBuffer(VkCommandBuffer inCommandBuffer, VkCommandBufferUsageFlags inUsageFlags);
void BeginSwapChainRenderPass(VkCommandBuffer inCommandBuffer);
void EndSwapChainRenderPass(VkCommandBuffer inCommandBuffer);

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
