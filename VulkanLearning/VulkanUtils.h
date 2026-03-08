#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

void InitVulkan(void* inUserData, int inWidth, int inHeight);
