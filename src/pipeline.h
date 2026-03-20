#pragma once

#include "vk_context.h"

#include <vulkan/vulkan_raii.hpp>

namespace osp {

struct Pipeline {
	vk::raii::Pipeline pipeline = nullptr;
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;

	Pipeline() = default;
	Pipeline(VkContext& context, vk::Format colorFormat, vk::Format depthFormat);

private:
	static std::vector<char> readFile(const std::string& path);
	static vk::raii::ShaderModule createShaderModule(VkContext& context, const std::vector<char>& code);
};
} // namespace osp