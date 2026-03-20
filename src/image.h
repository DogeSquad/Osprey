#pragma once

#include "vk_context.h"

#include <vulkan/vulkan_raii.hpp>

namespace osp {

inline vk::Format findSupportedFormat(VkContext& context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (const auto format : candidates)
	{
		vk::FormatProperties props = context.physicalDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

inline vk::Format findDepthFormat(VkContext& context)
{
	return findSupportedFormat(context,
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

struct Image {
	vk::raii::Image image = nullptr;
	vk::raii::DeviceMemory memory = nullptr;
	vk::raii::ImageView view = nullptr;
	vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;

	Image() = default;
	Image(VkContext& context, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect, vk::MemoryPropertyFlags properties);
	void transitionLayout(vk::raii::CommandBuffer& cmd,
		vk::ImageLayout         newLayout,
		vk::AccessFlags2        srcAccessMask,
		vk::AccessFlags2        dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask,
		vk::ImageAspectFlags    imageAspectFlags);
private:
	void createImageView(VkContext& context, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImage(VkContext& context, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
};

} // namespace osp