#pragma once

#include "VulkanUtils.h"

class FrameBuffer
{
public:
	void InitWithSize(int inWidth, int inHeight);
	void BeginRender(VkCommandBuffer inCommandBuffer);

	Texture2D* m_colorRenderTarget;
	Texture2D* m_depthStencilRenderTarget;
	VkRenderPass m_renderPass;
	VkFramebuffer m_framebuffer;
};
