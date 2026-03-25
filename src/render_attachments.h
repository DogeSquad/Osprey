#pragma once

#include "image.h"

namespace osp {

struct RenderAttachments
{
	Image color;
	Image depth;

	RenderAttachments() = default;
	RenderAttachments(VkContext& context, vk::Extent2D extent, vk::Format colorFormat);

	void transitionToMain(vk::raii::CommandBuffer& cmd);
};

} // namespace osp