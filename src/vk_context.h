#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace osp 
{

struct VkContext
{
	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::SurfaceKHR surface = nullptr;
	vk::raii::PhysicalDevice physicalDevice = nullptr;
	vk::raii::Device device = nullptr;
	vk::raii::Queue queue = nullptr;
	uint32_t queueIndex = ~0;
	vk::SampleCountFlagBits msaaSamples;
};

}