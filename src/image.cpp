#include "image.h"

#include "memory_utils.h"

namespace osp {

Image::Image(VkContext& context, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect, vk::MemoryPropertyFlags properties)
{
	createImage(context, width, height, mipLevels, numSamples, format, tiling, usage, properties);
	createImageView(context, format, aspect, mipLevels);
}
void Image::createImageView(VkContext& context, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	vk::ImageViewCreateInfo viewInfo{
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.subresourceRange = {aspectFlags, 0, mipLevels, 0, 1} };
	view = vk::raii::ImageView(context.device, viewInfo);
}

void Image::createImage(VkContext& context, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	vk::ImageCreateInfo imageInfo{
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = {width, height, 1},
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = numSamples,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined };
	image = vk::raii::Image(context.device, imageInfo);

	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties) };
	memory = vk::raii::DeviceMemory(context.device, allocInfo);
	image.bindMemory(memory, 0);
}

void Image::transitionLayout(vk::raii::CommandBuffer& cmd,
	vk::ImageLayout         newLayout,
	vk::AccessFlags2        srcAccessMask,
	vk::AccessFlags2        dstAccessMask,
	vk::PipelineStageFlags2 srcStageMask,
	vk::PipelineStageFlags2 dstStageMask,
	vk::ImageAspectFlags    imageAspectFlags)
{
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = srcStageMask,
		.srcAccessMask = srcAccessMask,
		.dstStageMask = dstStageMask,
		.dstAccessMask = dstAccessMask,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			   .aspectMask = imageAspectFlags,
			   .baseMipLevel = 0,
			   .levelCount = 1,
			   .baseArrayLayer = 0,
			   .layerCount = 1} };
	vk::DependencyInfo dependency_info = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier };
	cmd.pipelineBarrier2(dependency_info);
	currentLayout = newLayout;
}

} // namespace osp