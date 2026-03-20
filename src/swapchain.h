#pragma once

#include "vk_context.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

namespace osp {

struct Swapchain {
	GLFWwindow* window;
	vk::raii::SwapchainKHR swapChainKHR = nullptr;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::Extent2D extent;

	std::vector<vk::Image> images;
	std::vector<vk::raii::ImageView> imageViews;
	std::vector<vk::raii::Semaphore> renderFinished;

	Swapchain() = default;
	Swapchain(VkContext& context, vk::SurfaceKHR surface, GLFWwindow* window);
	Swapchain(const Swapchain&) = delete; 
	Swapchain& operator=(const Swapchain&) = delete;
	Swapchain(Swapchain&&) noexcept = default;
	Swapchain& operator=(Swapchain&&) noexcept = default;

	~Swapchain();

	void transitionToMain(vk::raii::CommandBuffer& cmd, uint32_t imageIndex);
	void transitionToPresent(vk::raii::CommandBuffer& cmd, uint32_t imageIndex);
private:
	vk::SurfaceFormatKHR chooseFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR   choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	[[nodiscard]] vk::Extent2D         chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
	uint32_t             chooseMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
};

} // namespace osp