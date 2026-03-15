#include "Scene.h"
#include "VulkanUtils.h"

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
}

void RenderOneFrame(float inFrameTimeInSeconds)
{
	VkCommandBuffer vulkanCommandBuffer = CreateCommandBuffer();
	BeginCommandBuffer(vulkanCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	BeginSwapChainRenderPass(vulkanCommandBuffer);

	EndSwapChainRenderPass(vulkanCommandBuffer);
}