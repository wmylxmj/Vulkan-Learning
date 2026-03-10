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
static VkDevice s_vulkanDevice = nullptr;
static VkQueue s_vulkanGraphicsQueue = nullptr;
static VkQueue s_vulkanPresentQueue = nullptr;
static VkSurfaceCapabilitiesKHR s_vulkanSurfaceCapabilities = {};
static VkSurfaceFormatKHR* s_vulkanSurfaceFormats = nullptr;
static uint32_t s_vulkanSurfaceFormatCount = 0;
static uint32_t s_vulkanPresentModeCount = 0;
static VkPresentModeKHR* s_vulkanPresentModes = nullptr;
static VkSwapchainKHR s_vulkanSwapchain = nullptr;

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

static bool InitVulkanLogicalDevice()
{
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.queueFamilyIndex = s_queueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 2;
	float queuePriority[] = { 1.0f, 1.0f };
	deviceQueueCreateInfo.pQueuePriorities = queuePriority;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = s_preferredEnabledLayers;

	deviceCreateInfo.enabledExtensionCount = 1;
	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	if (vkCreateDevice(s_vulkanPhysicalDevice, &deviceCreateInfo, nullptr, &s_vulkanDevice) != VK_SUCCESS)
	{
		return false;
	}

	vkGetDeviceQueue(s_vulkanDevice, s_queueFamilyIndex, 0, &s_vulkanGraphicsQueue);
	vkGetDeviceQueue(s_vulkanDevice, s_queueFamilyIndex, 1, &s_vulkanPresentQueue);

	return true;
}

static void InitSurfaceProperties()
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_vulkanPhysicalDevice, s_vulkanSurface, &s_vulkanSurfaceCapabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(s_vulkanPhysicalDevice, s_vulkanSurface, &s_vulkanSurfaceFormatCount, nullptr);
	s_vulkanSurfaceFormats = new VkSurfaceFormatKHR[s_vulkanSurfaceFormatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(s_vulkanPhysicalDevice, s_vulkanSurface, &s_vulkanSurfaceFormatCount, s_vulkanSurfaceFormats);

	vkGetPhysicalDeviceSurfacePresentModesKHR(s_vulkanPhysicalDevice, s_vulkanSurface, &s_vulkanPresentModeCount, nullptr);
	s_vulkanPresentModes = new VkPresentModeKHR[s_vulkanPresentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(s_vulkanPhysicalDevice, s_vulkanSurface, &s_vulkanPresentModeCount, s_vulkanPresentModes);
}

void InitSwapchain()
{
	VkSurfaceFormatKHR selectedSurfaceFormat = {};
	for (int i = 0; i < s_vulkanSurfaceFormatCount; ++i)
	{
		if (s_vulkanSurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
			s_vulkanSurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // Gamma color space
		{
			selectedSurfaceFormat = s_vulkanSurfaceFormats[i];
			break;
		}
	}
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
	swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
	swapchainCreateInfo.imageExtent.width = 1280u;
	swapchainCreateInfo.imageExtent.height = 720u;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 互斥访问
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // 颜色附件
	swapchainCreateInfo.minImageCount = s_vulkanSurfaceCapabilities.minImageCount + 1; // 最小图像数量加一
	uint32_t queueFamilyIndices[] = { (uint32_t)s_queueFamilyIndex };
	swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO 模式
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // 不进行预变换
	swapchainCreateInfo.queueFamilyIndexCount = 1;
	swapchainCreateInfo.surface = s_vulkanSurface;

	vkCreateSwapchainKHR(s_vulkanDevice, &swapchainCreateInfo, nullptr, &s_vulkanSwapchain);
}

bool InitVulkan(void* inUserData, int inWidth, int inHeight)
{
	// 创建 Vulkan 实例
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

	if (!InitVulkanLogicalDevice()) {
		return false;
	}

	InitSurfaceProperties();

	InitSwapchain();

	return true;
}