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

namespace osp
{

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
			vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)) };
	}
};

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