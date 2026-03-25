#pragma once

#include "vk_context.h"
#include "gpu_buffer.h"

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

namespace osp {

struct Frame {
	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 invView;
		alignas(16) glm::mat4 invProj;
		alignas(16) glm::vec3 lightDir;
		alignas(16) glm::vec3 cameraPos;
	};

	// Resources
	GpuBuffer uniformBuffer;
	void* uniformBufferMapped = nullptr;
	vk::raii::DescriptorSet descriptorSet{ nullptr };

	// Sync
	vk::raii::Fence inFlight = nullptr;
	vk::raii::Semaphore imageAvailable = nullptr;
	vk::raii::CommandBuffer cmd{ nullptr };

	Frame() = default;
	Frame(VkContext& context, vk::raii::CommandPool& commandPool, vk::raii::DescriptorPool& descriptorPool, vk::DescriptorSetLayout layout);

	void updateUBO(glm::mat4 model, glm::mat4 view, glm::mat4 proj, glm::vec3 lightDir, glm::vec3 cameraPos) const;
};

} // namespace osp