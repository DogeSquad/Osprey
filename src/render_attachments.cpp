#include "render_attachments.h"

namespace osp {

RenderAttachments::RenderAttachments(VkContext& context, vk::Extent2D extent, vk::Format colorFormat)
{
	// Create color resource for main pass
	color = osp::Image(context,
		extent.width,
		extent.height,
		1,
		context.msaaSamples,
		colorFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
		vk::ImageAspectFlagBits::eColor,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);
	
	// Create depth resource for main pass
	vk::Format depthFormat = findDepthFormat(context);
	depth = osp::Image(context,
		extent.width,
		extent.height,
		1,
		context.msaaSamples,
		depthFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);
}

void RenderAttachments::transitionToMain(vk::raii::CommandBuffer& cmd)
{
	color.transitionLayout(cmd,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor
	);
	depth.transitionLayout(cmd,
		vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth
	);
}

} // namespace osp