#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>
#include <functional>

namespace osp {

class RenderGraph {
private:
	struct Pass {
		std::string name;
		std::function<void(vk::raii::CommandBuffer&)> executeFunc;
		std::vector<std::string> inputs;
		std::vector<std::string> outputs;
	};

	struct ImageResource {
		std::string name;
		vk::Format format;
		vk::Extent2D extent;
		vk::ImageUsageFlags usage;
		vk::ImageLayout initialLayout;
		vk::ImageLayout finalLayout;

		vk::raii::Image image = nullptr;
		vk::raii::DeviceMemory memory = nullptr;
		vk::raii::ImageView imageView = nullptr;
	};

	std::unordered_map<std::string, ImageResource> resources;
	std::vector<Pass> passes;
	std::vector<size_t> executionOrder;

	std::vector<vk::raii::Semaphore> semaphores;
	std::vector<std::pair<size_t, size_t>> semaphoreSignalWaitPairs;

	vk::raii::Device* device = nullptr;
	vk::raii::PhysicalDevice* physicalDevice = nullptr;

public:
	explicit RenderGraph(vk::raii::Device& dev, vk::raii::PhysicalDevice& physDev) : 
		device(&dev), physicalDevice(&physDev) {}

	void AddResource(const std::string& name, vk::Format format, vk::Extent2D extent,
		vk::ImageUsageFlags usage, vk::ImageLayout initialLayout,
		vk::ImageLayout finalLayout);

	void AddPass(std::string name, std::function<void(vk::raii::CommandBuffer&)> executeFunc);
	void AddPass(const std::string& name,
		const std::vector<std::string>& inputs,
		const std::vector<std::string>& outputs,
		std::function<void(vk::raii::CommandBuffer&)> executeFunc);

	void Compile();

	ImageResource* GetImageResource(const std::string& name) 
	{ 
		auto it = resources.find(name);
		return (it != resources.end()) ? &it->second : nullptr;
	}

	void Execute(vk::raii::CommandBuffer& cmd, vk::Queue queue);
	void Clear();
};

}