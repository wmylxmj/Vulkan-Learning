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

bool InitVulkan(void* inUserData, int inWidth, int inHeight);
