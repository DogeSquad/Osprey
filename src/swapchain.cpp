#include "swapchain.h"

#include <ranges>


namespace osp {

static void transitionImageLayout(
	vk::raii::CommandBuffer& cmd,
	vk::Image               image,
	vk::ImageLayout         old_layout,
	vk::ImageLayout         new_layout,
	vk::AccessFlags2        src_access_mask,
	vk::AccessFlags2        dst_access_mask,
	vk::PipelineStageFlags2 src_stage_mask,
	vk::PipelineStageFlags2 dst_stage_mask,
	vk::ImageAspectFlags    image_aspect_flags)
{
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
				.aspectMask = image_aspect_flags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1} };
	vk::DependencyInfo dependency_info = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier };
	cmd.pipelineBarrier2(dependency_info);
}

Swapchain::Swapchain(VkContext& context, vk::SurfaceKHR surface, GLFWwindow* window)
{
	this->window = window;
	auto surfaceCapabilities = context.physicalDevice.getSurfaceCapabilitiesKHR(*context.surface);
	extent = chooseExtent(surfaceCapabilities);
	surfaceFormat = chooseFormat(context.physicalDevice.getSurfaceFormatsKHR(*context.surface));
	vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *context.surface,
												   .minImageCount = chooseMinImageCount(surfaceCapabilities),
												   .imageFormat = surfaceFormat.format,
												   .imageColorSpace = surfaceFormat.colorSpace,
												   .imageExtent = extent,
												   .imageArrayLayers = 1,
												   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
												   .imageSharingMode = vk::SharingMode::eExclusive,
												   .preTransform = surfaceCapabilities.currentTransform,
												   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
												   .presentMode = choosePresentMode(context.physicalDevice.getSurfacePresentModesKHR(*context.surface)),
												   .clipped = true };

	swapChainKHR = vk::raii::SwapchainKHR(context.device, swapChainCreateInfo);
	images = swapChainKHR.getImages();

	// Create image views
	assert(imageViews.empty());

	vk::ImageViewCreateInfo imageViewCreateInfo{
		.viewType = vk::ImageViewType::e2D,
		.format = surfaceFormat.format,
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
	for (auto image : images)
	{
		imageViewCreateInfo.image = image;
		imageViews.emplace_back(context.device, imageViewCreateInfo);
		renderFinished.emplace_back(context.device, vk::SemaphoreCreateInfo());
	}
}
Swapchain::~Swapchain()
{
	swapChainKHR.clear();
}

void Swapchain::transitionToMain(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
	transitionImageLayout(
		cmd,
		images[imageIndex],
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal,
		{},
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor
	);
}
void Swapchain::transitionToPresent(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
	transitionImageLayout(
		cmd,
		images[imageIndex],
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		{},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eBottomOfPipe,
		vk::ImageAspectFlagBits::eColor
	);
}

vk::SurfaceFormatKHR Swapchain::chooseFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	assert(!availableFormats.empty());
	const auto formatIt = std::ranges::find_if(
		availableFormats,
		[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Swapchain::choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
	return std::ranges::any_of(availablePresentModes,
		[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
		vk::PresentModeKHR::eMailbox :
		vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != 0xFFFFFFFF)
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	return {
		std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
}

uint32_t Swapchain::chooseMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
	auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
	if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}
	return minImageCount;
}

} // namespace osp