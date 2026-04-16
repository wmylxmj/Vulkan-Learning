#include "FrameBuffer.h"

void FrameBuffer::InitWithSize(int inWidth, int inHeight)
{
	// Render Target
	m_colorRenderTarget = new Texture2D();
	m_colorRenderTarget->width = inWidth;
	m_colorRenderTarget->height = inHeight;
	m_colorRenderTarget->format = VK_FORMAT_R8G8B8A8_UNORM;
	m_colorRenderTarget->aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	GenImage(
		m_colorRenderTarget,
		m_colorRenderTarget->width,
		m_colorRenderTarget->height,
		m_colorRenderTarget->format,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	m_colorRenderTarget->imageView = GenImageView2D(
		m_colorRenderTarget->image,
		m_colorRenderTarget->format,
		m_colorRenderTarget->aspectFlags
	);

	m_depthStencilRenderTarget = new Texture2D();
	m_depthStencilRenderTarget->width = inWidth;
	m_depthStencilRenderTarget->height = inHeight;
	m_depthStencilRenderTarget->format = VK_FORMAT_D24_UNORM_S8_UINT;
	m_depthStencilRenderTarget->aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	GenImage(
		m_depthStencilRenderTarget,
		m_depthStencilRenderTarget->width,
		m_depthStencilRenderTarget->height,
		m_depthStencilRenderTarget->format,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	m_depthStencilRenderTarget->imageView = GenImageView2D(
		m_depthStencilRenderTarget->image,
		m_depthStencilRenderTarget->format,
		m_depthStencilRenderTarget->aspectFlags
	);

	// Render Pass
	VkDevice vulkanDevice = GetVulkanDevice();
	VkAttachmentDescription attachmentDescriptions[2] = {};
	// Color Buffer
	attachmentDescriptions[0].format = m_colorRenderTarget->format;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

	vkCreateRenderPass(vulkanDevice, &renderPassCreateInfo, nullptr, &m_renderPass);

	// Frame Buffer
	VkImageView attachments[2] = {};
	attachments[0] = m_colorRenderTarget->imageView;
	attachments[1] = m_depthStencilRenderTarget->imageView;

	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = m_renderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = attachments;
	framebufferCreateInfo.width = (uint32_t)inWidth;
	framebufferCreateInfo.height = (uint32_t)inHeight;
	framebufferCreateInfo.layers = 1;

	vkCreateFramebuffer(vulkanDevice, &framebufferCreateInfo, nullptr, &m_framebuffer);
}