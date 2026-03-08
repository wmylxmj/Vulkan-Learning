#include "VulkanUtils.h"

#pragma comment(lib, "vulkan-1.lib")

static VkInstance s_vulkanInstance = nullptr;
static const char* s_enabledExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};
static const char* s_preferredEnabledLayers[] = {
	"VK_LAYER_KHRONOS_validation",
};
PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallbackEXT = nullptr;
PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR = nullptr;
VkDebugReportCallbackEXT s_vulkanDebugReportCallback = nullptr;
static VkSurfaceKHR s_vulkanSurface = nullptr;
static VkPhysicalDevice s_vulkanPhysicalDevice = nullptr;
static int s_queueFamilyIndex = -1;

static bool InitVulkanInstance()
{
	VkApplicationInfo vkApplicationInfo = {};
	vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkApplicationInfo.pApplicationName = "Vulkan Learning";
	vkApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkApplicationInfo.pEngineName = "No Engine";
	vkApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vkApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo vkInstanceCreateInfo = {};
	vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
	vkInstanceCreateInfo.enabledExtensionCount = 3;
	vkInstanceCreateInfo.ppEnabledExtensionNames = s_enabledExtensions;
	vkInstanceCreateInfo.enabledLayerCount = 1;
	vkInstanceCreateInfo.ppEnabledLayerNames = s_preferredEnabledLayers;

	if (vkCreateInstance(&vkInstanceCreateInfo, nullptr, &s_vulkanInstance) != VK_SUCCESS)
	{
		OutputDebugStringA("Failed to create Vulkan instance.\n");
		return false;
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData
) {
	OutputDebugStringA(pMessage);
	OutputDebugStringA("\n");
	return VK_FALSE;
}

static bool InitDebugger()
{
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
	debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugReportCallbackCreateInfo.pfnCallback = DebugCallback;

	if (__vkCreateDebugReportCallbackEXT(s_vulkanInstance, &debugReportCallbackCreateInfo, nullptr, &s_vulkanDebugReportCallback) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

static bool InitSurface(InitVulkanUserData* inUserData)
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = inUserData->hInstance;
	surfaceCreateInfo.hwnd = inUserData->hWnd;

	if (__vkCreateWin32SurfaceKHR(s_vulkanInstance, &surfaceCreateInfo, nullptr, &s_vulkanSurface) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

static bool InitVulkanPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(s_vulkanInstance, &physicalDeviceCount, nullptr);
	if (physicalDeviceCount == 0)
	{
		OutputDebugStringA("Failed to find GPUs with Vulkan support.\n");
		return false;
	}
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	vkEnumeratePhysicalDevices(s_vulkanInstance, &physicalDeviceCount, physicalDevices);

	int queueFamilyIndex = -1;
	for (uint32_t i = 0; i < physicalDeviceCount; i++)
	{
		VkPhysicalDevice physicalDevice = physicalDevices[i];
		uint32_t queueFamilyPropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
		VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyPropertyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties);

		for (uint32_t j = 0; j < queueFamilyPropertyCount; j++)
		{
			if (queueFamilyProperties[j].queueCount > 0 &&
				queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamilyIndex = j;
			}
			if (queueFamilyIndex != -1)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, j, s_vulkanSurface, &presentSupport);
				if (presentSupport)
				{
					break;
				}
			}
		}

		delete[] queueFamilyProperties;

		if (queueFamilyIndex != -1)
		{
			s_vulkanPhysicalDevice = physicalDevice;
			s_queueFamilyIndex = queueFamilyIndex;
			delete[] physicalDevices;
			return true;
		}
	}

	delete[] physicalDevices;
	return false;
}

bool InitVulkan(void* inUserData, int inWidth, int inHeight)
{
	// ´´½¨ Vulkan ÊµÀý
	if (!InitVulkanInstance()) {
		return false;
	}

	__vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_vulkanInstance, "vkCreateDebugReportCallbackEXT");
	__vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_vulkanInstance, "vkDestroyDebugReportCallbackEXT");
	__vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(s_vulkanInstance, "vkCreateWin32SurfaceKHR");

	if (!InitDebugger()) {
		return false;
	}

	if (!InitSurface((InitVulkanUserData*)inUserData)) {
		return false;
	}

	if (!InitVulkanPhysicalDevice()) {
		return false;
	}

	return true;
}