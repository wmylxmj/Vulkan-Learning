#include "VulkanUtils.h"

#include <string>
#include <vector>
#include "Scene.h"

#include "stb/stb_image.h"

#pragma comment(lib, "vulkan-1.lib")

static VkInstance s_vulkanInstance = nullptr;
static const char* s_enabledExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};
static char** s_ppPreferredEnabledLayers = nullptr;
static int s_preferredEnabledLayerCount = 0;

PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallbackEXT = nullptr;
PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR = nullptr;
VkDebugReportCallbackEXT s_vulkanDebugReportCallback = nullptr;
static VkSurfaceKHR s_vulkanSurface = nullptr;
static VkPhysicalDevice s_vulkanPhysicalDevice = nullptr;
static uint32_t s_graphicsQueueFamilyIndex = 0;
static uint32_t s_presentQueueFamilyIndex = 0;
static VkDevice s_vulkanDevice = nullptr;
static VkQueue s_vulkanGraphicsQueue = nullptr;
static VkQueue s_vulkanPresentQueue = nullptr;
static VkSurfaceCapabilitiesKHR s_vulkanSurfaceCapabilities = {};
static VkSurfaceFormatKHR* s_vulkanSurfaceFormats = nullptr;
static uint32_t s_vulkanSurfaceFormatCount = 0;
static uint32_t s_vulkanPresentModeCount = 0;
static VkPresentModeKHR* s_vulkanPresentModes = nullptr;
static VkSwapchainKHR s_vulkanSwapchain = nullptr;
static VkImage* s_vulkanSwapchainImages = nullptr;
static uint32_t s_vulkanSwapchainImageCount = 0;
static VkImageView* s_vulkanSwapchainImageViews = nullptr;
static Texture* s_vulkanSwapchainDSRTs = nullptr;
static VkRenderPass s_vulkanSwapchainRenderPass = nullptr;
static VkFramebuffer s_vulkanSwapchainFramebuffers[2] = { nullptr };
static VkCommandPool s_vulkanCommandPool = nullptr;
static VkSemaphore s_readyToRenderSemaphore = nullptr;
static VkSemaphore s_readyToPresentSemaphore = nullptr;
static uint32_t s_currentFrameBufferToRenderIndex = 0;

static ShaderParameterDescription s_uberShaderParameterDescription;

Buffer::Buffer()
{
	buffer = nullptr;
	memory = nullptr;
}

Buffer::~Buffer() {
}

static void InitUberPipelineLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[4] = {};
	descriptorSetLayoutBindings[0].binding = 0;
	descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[0].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr; // for texture

	descriptorSetLayoutBindings[1].binding = 1;
	descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[1].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr; // for texture

	descriptorSetLayoutBindings[2].binding = 2;
	descriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings[2].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings[2].pImmutableSamplers = nullptr; // for texture

	descriptorSetLayoutBindings[3].binding = 3;
	descriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings[3].descriptorCount = 1; // ubo -> descriptor <- texture
	descriptorSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings[3].pImmutableSamplers = nullptr; // for texture

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = _countof(descriptorSetLayoutBindings);
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

	vkCreateDescriptorSetLayout(s_vulkanDevice, &descriptorSetLayoutCreateInfo, nullptr, &s_uberShaderParameterDescription.descriptorSetLayout);
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4) * 2;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &s_uberShaderParameterDescription.descriptorSetLayout;
	vkCreatePipelineLayout(s_vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &s_uberShaderParameterDescription.pipelineLayout);
}

Buffer* GenBufferObject(VkDeviceSize inBufferSize, VkBufferUsageFlags inUsageFlags, VkMemoryPropertyFlagBits inMemoryPropertyFlagBits, size_t inDataSize, const void* inData)
{
	Buffer* pBuffer = new Buffer();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = inBufferSize;
	bufferCreateInfo.usage = inUsageFlags;
	if (vkCreateBuffer(s_vulkanDevice, &bufferCreateInfo, nullptr, &pBuffer->buffer) != VK_SUCCESS) {
		OutputDebugStringA("Failed to create buffer!\n");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(s_vulkanDevice, pBuffer->buffer, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(GetVulkanPhysicalDevice(), &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & inMemoryPropertyFlagBits))
		{
			memoryAllocateInfo.memoryTypeIndex = i;
			break;
		}
	}

	vkAllocateMemory(s_vulkanDevice, &memoryAllocateInfo, nullptr, &pBuffer->memory);
	vkBindBufferMemory(s_vulkanDevice, pBuffer->buffer, pBuffer->memory, 0);

	if (inMemoryPropertyFlagBits & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		if (inDataSize > 0 && inData != nullptr) {
			void* pMemory;
			vkMapMemory(s_vulkanDevice, pBuffer->memory, 0, inDataSize, 0, &pMemory);
			memcpy(pMemory, inData, inDataSize);
			vkUnmapMemory(s_vulkanDevice, pBuffer->memory);
		}
	}

	return pBuffer;
}

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

	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	VkLayerProperties* layerProperties = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

	s_ppPreferredEnabledLayers = new char* [layerCount];
	for (int i = 0; i < layerCount; ++i)
	{
		if (strstr(layerProperties[i].layerName, "validation"))
		{
			s_ppPreferredEnabledLayers[s_preferredEnabledLayerCount] = new char[strlen(layerProperties[i].layerName) + 1];
			strcpy_s(s_ppPreferredEnabledLayers[s_preferredEnabledLayerCount], strlen(layerProperties[i].layerName) + 1, layerProperties[i].layerName);
			std::string debugString = "Found Validation Layer: " + std::string(layerProperties[i].layerName) + "\n";
			OutputDebugStringA(debugString.c_str());
			++s_preferredEnabledLayerCount;
		}
	}

#ifdef _DEBUG
	vkInstanceCreateInfo.enabledLayerCount = s_preferredEnabledLayerCount;
	vkInstanceCreateInfo.ppEnabledLayerNames = s_ppPreferredEnabledLayers;
	OutputDebugStringA("Vulkan Validation Layers Enabled.\n");
#endif

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

	int graphicsQueueFamilyIndex = -1;
	int presentQueueFamilyIndex = -1;
	for (uint32_t i = 0; i < physicalDeviceCount; ++i)
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
				graphicsQueueFamilyIndex = j;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, j, s_vulkanSurface, &presentSupport);
			if (queueFamilyProperties[j].queueCount > 0 && presentSupport)
			{
				presentQueueFamilyIndex = j;
			}
			if (graphicsQueueFamilyIndex != -1 && presentQueueFamilyIndex != -1)
			{
				s_vulkanPhysicalDevice = physicalDevice;
				s_graphicsQueueFamilyIndex = uint32_t(graphicsQueueFamilyIndex);
				s_presentQueueFamilyIndex = uint32_t(presentQueueFamilyIndex);

				delete[] queueFamilyProperties;
				delete[] physicalDevices;
				return true;
			}
		}
	}

	delete[] physicalDevices;
	return false;
}

static bool InitVulkanLogicalDevice()
{
	float queuePriority = 1.0f;
	int queueCreateInfoCount = 2;
	VkDeviceQueueCreateInfo deviceQueueCreateInfos[2] = {};
	if (s_graphicsQueueFamilyIndex == s_presentQueueFamilyIndex) {
		deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[0].queueFamilyIndex = s_graphicsQueueFamilyIndex;
		deviceQueueCreateInfos[0].queueCount = 1;
		deviceQueueCreateInfos[0].pQueuePriorities = &queuePriority;
		queueCreateInfoCount = 1;
	}
	else {
		deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[0].queueFamilyIndex = s_graphicsQueueFamilyIndex;
		deviceQueueCreateInfos[0].queueCount = 1;
		deviceQueueCreateInfos[0].pQueuePriorities = &queuePriority;
		deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[1].queueFamilyIndex = s_presentQueueFamilyIndex;
		deviceQueueCreateInfos[1].queueCount = 1;
		deviceQueueCreateInfos[1].pQueuePriorities = &queuePriority;
		queueCreateInfoCount = 2;
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;

#ifdef _DEBUG
	deviceCreateInfo.enabledLayerCount = s_preferredEnabledLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = s_ppPreferredEnabledLayers;
#endif

	deviceCreateInfo.enabledExtensionCount = 1;
	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
	physicalDeviceFeatures.geometryShader = VK_TRUE;
	physicalDeviceFeatures.tessellationShader = VK_TRUE;
	physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
	physicalDeviceFeatures.wideLines = VK_TRUE;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	if (vkCreateDevice(s_vulkanPhysicalDevice, &deviceCreateInfo, nullptr, &s_vulkanDevice) != VK_SUCCESS)
	{
		return false;
	}

	vkGetDeviceQueue(s_vulkanDevice, s_graphicsQueueFamilyIndex, 0, &s_vulkanGraphicsQueue);
	vkGetDeviceQueue(s_vulkanDevice, s_presentQueueFamilyIndex, 0, &s_vulkanPresentQueue);

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
	uint32_t queueFamilyIndices[2] = { 0 };
	uint32_t queueFamilyIndexCount = 2;
	if (s_graphicsQueueFamilyIndex == s_presentQueueFamilyIndex) {
		queueFamilyIndices[0] = s_graphicsQueueFamilyIndex;
		queueFamilyIndexCount = 1;
	}
	else {
		queueFamilyIndices[0] = s_graphicsQueueFamilyIndex;
		queueFamilyIndices[1] = s_presentQueueFamilyIndex;
	}
	swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO 模式
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // 不进行预变换
	swapchainCreateInfo.surface = s_vulkanSurface;

	vkCreateSwapchainKHR(s_vulkanDevice, &swapchainCreateInfo, nullptr, &s_vulkanSwapchain);
}

VkImageView GenImageView2D(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inAspectFlags)
{
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.format = inFormat;
	imageViewCreateInfo.image = inImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.subresourceRange.aspectMask = inAspectFlags;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	VkImageView imageView;
	vkCreateImageView(s_vulkanDevice, &imageViewCreateInfo, nullptr, &imageView);
	return imageView;
}

void InitSwapChainRenderTarget()
{
	vkGetSwapchainImagesKHR(s_vulkanDevice, s_vulkanSwapchain, &s_vulkanSwapchainImageCount, nullptr);
	s_vulkanSwapchainImages = new VkImage[s_vulkanSwapchainImageCount];
	vkGetSwapchainImagesKHR(s_vulkanDevice, s_vulkanSwapchain, &s_vulkanSwapchainImageCount, s_vulkanSwapchainImages);

	s_vulkanSwapchainImageViews = new VkImageView[s_vulkanSwapchainImageCount];
	for (uint32_t i = 0; i < s_vulkanSwapchainImageCount; ++i)
	{
		s_vulkanSwapchainImageViews[i] = GenImageView2D(
			s_vulkanSwapchainImages[i],
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
}

void GenImage(
	Texture* inOutTexture,
	uint32_t inWidth,
	uint32_t inHeight,
	VkFormat inFormat,
	VkImageUsageFlags inUsageFlags,
	VkMemoryPropertyFlagBits inMemoryPropertyFlagBits
)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.format = inFormat;
	imageCreateInfo.extent.width = inWidth;
	imageCreateInfo.extent.height = inHeight;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = inUsageFlags;
	vkCreateImage(s_vulkanDevice, &imageCreateInfo, nullptr, &inOutTexture->image);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(s_vulkanDevice, inOutTexture->image, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(s_vulkanPhysicalDevice, &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & inMemoryPropertyFlagBits)) // 显存
		{
			memoryAllocateInfo.memoryTypeIndex = i;
			break;
		}
	}
	vkAllocateMemory(s_vulkanDevice, &memoryAllocateInfo, nullptr, &inOutTexture->memory);
	vkBindImageMemory(s_vulkanDevice, inOutTexture->image, inOutTexture->memory, 0);
}

void InitSwapChainDSRT()
{
	s_vulkanSwapchainDSRTs = new Texture[1];
	s_vulkanSwapchainDSRTs->format = VK_FORMAT_D24_UNORM_S8_UINT;
	s_vulkanSwapchainDSRTs->aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	GenImage(
		s_vulkanSwapchainDSRTs,
		s_vulkanSurfaceCapabilities.currentExtent.width,
		s_vulkanSurfaceCapabilities.currentExtent.height,
		s_vulkanSwapchainDSRTs->format,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	s_vulkanSwapchainDSRTs->imageView = GenImageView2D(
		s_vulkanSwapchainDSRTs->image,
		s_vulkanSwapchainDSRTs->format,
		s_vulkanSwapchainDSRTs->aspectFlags
	);
}

void InitSwapChainRenderPass()
{
	VkAttachmentDescription attachmentDescriptions[2] = {};
	// Color Buffer
	attachmentDescriptions[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Depth-Stencil Buffer
	attachmentDescriptions[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentReference = {};
	depthStencilAttachmentReference.attachment = 1;
	depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;

	vkCreateRenderPass(s_vulkanDevice, &renderPassCreateInfo, nullptr, &s_vulkanSwapchainRenderPass);
}

static void InitSwapchainFBO()
{
	for (int i = 0; i < 2; ++i) {
		VkImageView attachments[2] = {};
		attachments[0] = s_vulkanSwapchainImageViews[i];
		attachments[1] = s_vulkanSwapchainDSRTs[0].imageView;

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = s_vulkanSwapchainRenderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = s_vulkanSurfaceCapabilities.currentExtent.width;
		framebufferCreateInfo.height = s_vulkanSurfaceCapabilities.currentExtent.height;
		framebufferCreateInfo.layers = 1;

		vkCreateFramebuffer(s_vulkanDevice, &framebufferCreateInfo, nullptr, &s_vulkanSwapchainFramebuffers[i]);
	}
}

static void InitCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = s_graphicsQueueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // 允许重置命令缓冲区
	vkCreateCommandPool(s_vulkanDevice, &commandPoolCreateInfo, nullptr, &s_vulkanCommandPool);
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

	InitSwapChainRenderTarget();

	InitSwapChainDSRT();

	InitSwapChainRenderPass();

	InitSwapchainFBO();

	InitCommandPool();

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	vkCreateSemaphore(s_vulkanDevice, &semaphoreCreateInfo, nullptr, &s_readyToRenderSemaphore);
	vkCreateSemaphore(s_vulkanDevice, &semaphoreCreateInfo, nullptr, &s_readyToPresentSemaphore);

	InitUberPipelineLayout();

	return true;
}

VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel inCommandBufferLevel)
{
	VkCommandBuffer commandBuffer = nullptr;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = s_vulkanCommandPool;
	commandBufferAllocateInfo.level = inCommandBufferLevel;
	vkAllocateCommandBuffers(s_vulkanDevice, &commandBufferAllocateInfo, &commandBuffer);
	return commandBuffer;
}

void BeginCommandBuffer(VkCommandBuffer inCommandBuffer, VkCommandBufferUsageFlags inUsageFlags)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = inUsageFlags;
	vkBeginCommandBuffer(inCommandBuffer, &commandBufferBeginInfo);
}

uint32_t BeginSwapChainRenderPass(VkCommandBuffer inCommandBuffer)
{
	vkAcquireNextImageKHR(s_vulkanDevice, s_vulkanSwapchain, 1000000, s_readyToRenderSemaphore, nullptr, &s_currentFrameBufferToRenderIndex);

	VkClearValue clearValues[2] = {};
	clearValues[0].color = { 0.1f, 0.4f, 0.6f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0u };
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.framebuffer = s_vulkanSwapchainFramebuffers[s_currentFrameBufferToRenderIndex];
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = s_vulkanSurfaceCapabilities.currentExtent;
	renderPassBeginInfo.renderPass = s_vulkanSwapchainRenderPass;
	vkCmdBeginRenderPass(inCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return s_currentFrameBufferToRenderIndex;
}

void EndSwapChainRenderPass(VkCommandBuffer inCommandBuffer)
{
	// End Render Pass
	vkCmdEndRenderPass(inCommandBuffer);
	// End Command Buffer
	vkEndCommandBuffer(inCommandBuffer);

	// Submit Command Buffer
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &inCommandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &s_readyToRenderSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &s_readyToPresentSemaphore;
	vkQueueSubmit(s_vulkanGraphicsQueue, 1, &submitInfo, nullptr);

	// Present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &s_readyToPresentSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &s_vulkanSwapchain;
	presentInfo.pImageIndices = &s_currentFrameBufferToRenderIndex;
	vkQueuePresentKHR(s_vulkanPresentQueue, &presentInfo);
	vkQueueWaitIdle(s_vulkanPresentQueue);

	s_currentFrameBufferToRenderIndex = (s_currentFrameBufferToRenderIndex + 1) % s_vulkanSwapchainImageCount;

	vkFreeCommandBuffers(s_vulkanDevice, s_vulkanCommandPool, 1, &inCommandBuffer);
}

VkQueue GetGraphicsQueue()
{
	return s_vulkanGraphicsQueue;
}

VkDevice GetVulkanDevice()
{
	return s_vulkanDevice;
}

VkPhysicalDevice GetVulkanPhysicalDevice()
{
	return s_vulkanPhysicalDevice;
}

VkRenderPass GetVulkanSwapChainRenderPass()
{
	return s_vulkanSwapchainRenderPass;
}

ShaderParameterDescription* GetUberPassShaderParameterDescription()
{
	return &s_uberShaderParameterDescription;
}

VkPipeline CreatePipeline(
	const std::vector<VkVertexInputBindingDescription>& inVertexInputBindingDescriptions,
	const std::vector<VkVertexInputAttributeDescription>& inVertexInputAttributeDescriptions,
	const VkShaderModule inVertexShaderModule,
	const VkShaderModule inFragmentShaderModule
)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = inVertexInputBindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = inVertexInputBindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = inVertexInputAttributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = inVertexInputAttributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 0;
	dynamicStateCreateInfo.pDynamicStates = nullptr;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(1280);
	viewport.height = static_cast<float>(720);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizationStateCreateInfo.lineWidth = 2.0f;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // 背面剔除

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineShaderStageCreateInfo shaderStages[2] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = inVertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = inFragmentShaderModule;
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.renderPass = GetVulkanSwapChainRenderPass();
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.layout = s_uberShaderParameterDescription.pipelineLayout;

	VkPipeline pipeline;
	vkCreateGraphicsPipelines(s_vulkanDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
	return pipeline;
}

VkPipeline CreateVGFPipeline(
	const std::vector<VkVertexInputBindingDescription>& inVertexInputBindingDescriptions,
	const std::vector<VkVertexInputAttributeDescription>& inVertexInputAttributeDescriptions,
	const VkShaderModule inVertexShaderModule,
	const VkShaderModule inGeometryShaderModule,
	const VkShaderModule inFragmentShaderModule
)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = inVertexInputBindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = inVertexInputBindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = inVertexInputAttributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = inVertexInputAttributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 0;
	dynamicStateCreateInfo.pDynamicStates = nullptr;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(1280);
	viewport.height = static_cast<float>(720);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineShaderStageCreateInfo shaderStages[3] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = inVertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	shaderStages[1].module = inGeometryShaderModule;
	shaderStages[1].pName = "main";
	shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[2].module = inFragmentShaderModule;
	shaderStages[2].pName = "main";

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.renderPass = GetVulkanSwapChainRenderPass();
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.stageCount = _countof(shaderStages);
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.layout = s_uberShaderParameterDescription.pipelineLayout;

	VkPipeline pipeline;
	vkCreateGraphicsPipelines(s_vulkanDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
	return pipeline;
}

VkPipeline CreateVTFPipeline(
	VkRenderPass inRenderPass,
	const std::vector<VkVertexInputBindingDescription>& inVertexInputBindingDescriptions,
	const std::vector<VkVertexInputAttributeDescription>& inVertexInputAttributeDescriptions,
	const VkShaderModule inVertexShaderModule,
	const VkShaderModule inTessellationControlShaderModule,
	const VkShaderModule inTessellationEvaluationShaderModule,
	const VkShaderModule inFragmentShaderModule
)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = inVertexInputBindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = inVertexInputBindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = inVertexInputAttributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = inVertexInputAttributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 0;
	dynamicStateCreateInfo.pDynamicStates = nullptr;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(1280);
	viewport.height = static_cast<float>(720);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizationStateCreateInfo.lineWidth = 2.0f;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineShaderStageCreateInfo shaderStages[4] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = inVertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	shaderStages[1].module = inTessellationControlShaderModule;
	shaderStages[1].pName = "main";
	shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[2].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	shaderStages[2].module = inTessellationEvaluationShaderModule;
	shaderStages[2].pName = "main";
	shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[3].module = inFragmentShaderModule;
	shaderStages[3].pName = "main";

	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo = {};
	tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationStateCreateInfo.patchControlPoints = 4;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.renderPass = inRenderPass;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.stageCount = _countof(shaderStages);
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.layout = s_uberShaderParameterDescription.pipelineLayout;
	graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;

	VkPipeline pipeline;
	vkCreateGraphicsPipelines(s_vulkanDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
	return pipeline;
}

VkShaderModule CompileShader(const char* inFilePath)
{
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0)
	{
		fseek(pFile, 0, SEEK_END);
		long fileSize = ftell(pFile);
		rewind(pFile);
		unsigned char* fileContent = new unsigned char[fileSize];
		fread(fileContent, 1, fileSize, pFile);
		fclose(pFile);

		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = fileSize;
		shaderCreateInfo.pCode = (uint32_t*)fileContent;

		VkShaderModule shader;
		if (vkCreateShaderModule(GetVulkanDevice(), &shaderCreateInfo, nullptr, &shader) != VK_SUCCESS)
		{
			std::string errorString = "Failed to create shader " + std::string(inFilePath);
			OutputDebugStringA(errorString.c_str());
		}
		return shader;
	}
	return nullptr;
}

VkFramebuffer* GetSwapChainFrameBuffers()
{
	return s_vulkanSwapchainFramebuffers;
}

void TransferImageLayout(
	VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inOldPipelineStage,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inNewPipelineStage
)
{
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.oldLayout = inOldLayout;
	imageMemoryBarrier.newLayout = inNewLayout;
	imageMemoryBarrier.srcAccessMask = inOldAccessFlags;
	imageMemoryBarrier.dstAccessMask = inNewAccessFlags;
	imageMemoryBarrier.image = inImage;
	imageMemoryBarrier.subresourceRange = inSubresourceRange;
	vkCmdPipelineBarrier(
		inCommandBuffer,
		inOldPipelineStage,
		inNewPipelineStage,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
	);
}

void SubmitBufferDataToImage(
	VkCommandBuffer inCommandBuffer,
	VkBuffer inBuffer, VkImage inImage,
	uint32_t inImageWidth, uint32_t inImageHeight,
	uint32_t inFaceIndex
)
{
	VkBufferImageCopy bufferImageCopy = {};
	bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferImageCopy.imageSubresource.baseArrayLayer = inFaceIndex;
	bufferImageCopy.imageSubresource.layerCount = 1;
	bufferImageCopy.imageSubresource.mipLevel = 0;

	bufferImageCopy.imageOffset = { 0, 0, 0 };
	bufferImageCopy.imageExtent = { inImageWidth, inImageHeight, 1 };

	vkCmdCopyBufferToImage(
		inCommandBuffer, inBuffer, inImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy
	);
}

void SubmitTextureData(
	VkImage inTargetImage, const void* inPixelData,
	uint32_t inImageWidth, uint32_t inImageHeight, uint32_t inImageSizeInBytes
)
{
	VkCommandBuffer commandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// 资源状态转换 -> 转换为传输目标状态
	VkImageSubresourceRange imageSubresourceRange = {};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.layerCount = 1;

	TransferImageLayout(
		commandBuffer, inTargetImage, imageSubresourceRange,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);

	// 创建上传堆
	Buffer* pUploadBuffer = GenBufferObject(
		inImageSizeInBytes,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);

	// 将数据拷贝到上传堆
	void* pMemoryData = nullptr;
	vkMapMemory(s_vulkanDevice, pUploadBuffer->memory, 0, inImageSizeInBytes, 0, &pMemoryData);
	memcpy(pMemoryData, inPixelData, inImageSizeInBytes);
	vkUnmapMemory(s_vulkanDevice, pUploadBuffer->memory);

	// 将上传堆数据拷贝到显存
	SubmitBufferDataToImage(
		commandBuffer,
		pUploadBuffer->buffer, inTargetImage,
		inImageWidth, inImageHeight,
		0
	);

	// 资源状态转换 -> 转换为采样状态
	TransferImageLayout(
		commandBuffer, inTargetImage, imageSubresourceRange,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);

	vkEndCommandBuffer(commandBuffer);

	VkFence fence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(s_vulkanDevice, &fenceCreateInfo, nullptr, &fence);
	vkResetFences(s_vulkanDevice, 1, &fence);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(GetGraphicsQueue(), 1, &submitInfo, fence);
	VkResult result = vkWaitForFences(s_vulkanDevice, 1, &fence, true, UINT64_MAX);
	if (result == VK_SUCCESS)
	{
		OutputDebugStringA("Upload image to texture success.\n");
	}
	vkDestroyBuffer(s_vulkanDevice, pUploadBuffer->buffer, nullptr);
	vkFreeMemory(s_vulkanDevice, pUploadBuffer->memory, nullptr);
	vkDestroyFence(s_vulkanDevice, fence, nullptr);
	vkFreeCommandBuffers(s_vulkanDevice, s_vulkanCommandPool, 1, &commandBuffer);
}

VkSampler GenSampler(
	VkFilter inMinFilter, VkFilter inMagFilter,
	VkSamplerAddressMode inWrapModeU, VkSamplerAddressMode inWrapModeV, VkSamplerAddressMode inWrapModeW
)
{
	VkSampler sampler = nullptr;
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.minFilter = inMinFilter;
	samplerCreateInfo.magFilter = inMagFilter;
	samplerCreateInfo.anisotropyEnable = false;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.addressModeU = inWrapModeU;
	samplerCreateInfo.addressModeV = inWrapModeV;
	samplerCreateInfo.addressModeW = inWrapModeW;
	vkCreateSampler(s_vulkanDevice, &samplerCreateInfo, nullptr, &sampler);
	return sampler;
}

Texture2D* LoadTexture2DFromFile(const char* inFilePath)
{
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;
	void* pixelData = stbi_load(
		inFilePath,
		&imageWidth, &imageHeight, &imageChannels, 4
	);
	int imageSize = imageWidth * imageHeight * 4;

	Texture2D* pTexture = new Texture2D[1];
	pTexture->format = VK_FORMAT_R8G8B8A8_UNORM;
	pTexture->aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	GenImage(
		pTexture,
		imageWidth,
		imageHeight,
		pTexture->format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	SubmitTextureData(pTexture->image, pixelData, imageWidth, imageHeight, imageSize);

	pTexture->imageView = GenImageView2D(
		pTexture->image,
		pTexture->format,
		pTexture->aspectFlags
	);

	pTexture->width = imageWidth;
	pTexture->height = imageHeight;
	pTexture->numChannels = imageChannels;

	return pTexture;
}

Texture2D* LoadTextureCubeMapFromFile(const char** inFilePaths)
{
	void* imageDatas[6] = { nullptr };
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannels = 0;
	for (int i = 0; i < 6; ++i) {
		imageDatas[i] = stbi_load(
			inFilePaths[i],
			&imageWidth, &imageHeight, &imageChannels, 4
		);
	}
	int imageSize = imageWidth * imageHeight * 4;

	Texture2D* pCubeMapTexture = new Texture2D[1];
	pCubeMapTexture->format = VK_FORMAT_R8G8B8A8_UNORM;
	pCubeMapTexture->aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	GenImageCubeMap(
		pCubeMapTexture,
		imageWidth,
		imageHeight,
		pCubeMapTexture->format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	SubmitCubeMapData(pCubeMapTexture->image, imageDatas, imageWidth, imageHeight, imageSize);

	pCubeMapTexture->imageView = GenImageViewCubeMap(
		pCubeMapTexture->image,
		pCubeMapTexture->format,
		pCubeMapTexture->aspectFlags
	);

	return pCubeMapTexture;
}

void GenImageCubeMap(
	Texture* inOutTexture,
	uint32_t inWidth,
	uint32_t inHeight,
	VkFormat inFormat,
	VkImageUsageFlags inUsageFlags,
	VkMemoryPropertyFlagBits inMemoryPropertyFlagBits
)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.format = inFormat;
	imageCreateInfo.extent.width = inWidth;
	imageCreateInfo.extent.height = inHeight;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1; // Mipmap Chain
	imageCreateInfo.arrayLayers = 6; // Texture Array
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = inUsageFlags;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	vkCreateImage(s_vulkanDevice, &imageCreateInfo, nullptr, &inOutTexture->image);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(s_vulkanDevice, inOutTexture->image, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(s_vulkanPhysicalDevice, &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
			(memoryProperties.memoryTypes[i].propertyFlags & inMemoryPropertyFlagBits)) // 显存
		{
			memoryAllocateInfo.memoryTypeIndex = i;
			break;
		}
	}
	vkAllocateMemory(s_vulkanDevice, &memoryAllocateInfo, nullptr, &inOutTexture->memory);
	vkBindImageMemory(s_vulkanDevice, inOutTexture->image, inOutTexture->memory, 0);
}

VkImageView GenImageViewCubeMap(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inAspectFlags)
{
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCreateInfo.image = inImage;
	imageViewCreateInfo.format = inFormat;
	imageViewCreateInfo.subresourceRange.aspectMask = inAspectFlags;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 6;

	VkImageView imageView;
	vkCreateImageView(s_vulkanDevice, &imageViewCreateInfo, nullptr, &imageView);
	return imageView;
}

void SubmitCubeMapData(
	VkImage inTargetImage, void** inPixelData,
	uint32_t inImageWidth, uint32_t inImageHeight, uint32_t inImageSizeInBytes
)
{
	VkCommandBuffer commandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// 创建上传堆
	Buffer* pUploadBuffer[6];
	for (int i = 0; i < 6; ++i) {
		// 资源状态转换 -> 转换为传输目标状态
		VkImageSubresourceRange imageSubresourceRange = {};
		imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageSubresourceRange.baseMipLevel = 0;
		imageSubresourceRange.levelCount = 1;
		imageSubresourceRange.baseArrayLayer = i;
		imageSubresourceRange.layerCount = 1;

		TransferImageLayout(
			commandBuffer, inTargetImage, imageSubresourceRange,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);

		pUploadBuffer[i] = GenBufferObject(
			inImageSizeInBytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			inImageSizeInBytes,
			inPixelData[i]
		);

		// 将上传堆数据拷贝到显存
		SubmitBufferDataToImage(
			commandBuffer,
			pUploadBuffer[i]->buffer, inTargetImage,
			inImageWidth, inImageHeight,
			i
		);

		// 资源状态转换 -> 转换为采样状态
		TransferImageLayout(
			commandBuffer, inTargetImage, imageSubresourceRange,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);
	}

	vkEndCommandBuffer(commandBuffer);

	VkFence fence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(s_vulkanDevice, &fenceCreateInfo, nullptr, &fence);
	vkResetFences(s_vulkanDevice, 1, &fence);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(GetGraphicsQueue(), 1, &submitInfo, fence);
	VkResult result = vkWaitForFences(s_vulkanDevice, 1, &fence, true, UINT64_MAX);
	if (result == VK_SUCCESS)
	{
		OutputDebugStringA("Upload image to texture success.\n");
	}
	for (int i = 0; i < 6; ++i) {
		vkDestroyBuffer(s_vulkanDevice, pUploadBuffer[i]->buffer, nullptr);
		vkFreeMemory(s_vulkanDevice, pUploadBuffer[i]->memory, nullptr);
	}
	vkDestroyFence(s_vulkanDevice, fence, nullptr);
	vkFreeCommandBuffers(s_vulkanDevice, s_vulkanCommandPool, 1, &commandBuffer);
}