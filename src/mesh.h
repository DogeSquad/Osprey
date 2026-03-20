#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "gpu_buffer.h"
#include "vertex.h"

namespace osp
{

struct Mesh 
{
	const vk::raii::Device& device;
	const vk::raii::PhysicalDevice& physicalDevice;
	const vk::raii::Queue& queue;
	const vk::raii::CommandPool& commandPool;

	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	} data;
	GpuBuffer vertexBuffer;
	GpuBuffer indexBuffer;

	Mesh(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		const vk::raii::Queue& queue,
		const vk::raii::CommandPool& commandPool
	)
		: device(device)
		, physicalDevice(physicalDevice)
		, queue(queue)
		, commandPool(commandPool)
		, vertexBuffer(device, physicalDevice, queue, commandPool)
		, indexBuffer(device, physicalDevice, queue, commandPool)
	{
	}

	void upload()
	{
		vertexBuffer.uploadData(sizeof(data.vertices[0]) * data.vertices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, data.vertices.data());
		indexBuffer.uploadData(sizeof(data.indices[0]) * data.indices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, data.indices.data());
	}
};

}