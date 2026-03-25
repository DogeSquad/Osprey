#pragma once

#include "vk_context.h"

#include <vulkan/vulkan_raii.hpp>

namespace osp {

struct Pipeline {
	struct Config {
        std::string           shaderPath;
        std::string           vertexEntry = "vertMain";
        std::string           fragEntry = "fragMain";
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::PolygonMode       polygonMode = vk::PolygonMode::eFill;
        bool                  hasVertexInput = true;
        bool                  depthTest = true;
        bool                  depthWrite = true;
    };

	vk::raii::Pipeline pipeline = nullptr;
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;

	Pipeline() = default;
	Pipeline(VkContext& context, vk::Format colorFormat, vk::Format depthFormat, const Config& config);

private:
	static std::vector<char> readFile(const std::string& path);
	static vk::raii::ShaderModule createShaderModule(VkContext& context, const std::vector<char>& code);
};
} // namespace osp