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

bool InitVulkan(void* inUserData, int inWidth, int inHeight);
