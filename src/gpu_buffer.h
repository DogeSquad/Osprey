#pragma once

#include "vk_context.h"
#include "memory_utils.h"

#include <memory>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace osp
{

struct GpuBuffer
{
	vk::raii::Buffer       buffer = nullptr;
	vk::raii::DeviceMemory bufferMemory = nullptr;

	GpuBuffer() = default;

	GpuBuffer(VkContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		create(context, size, usage, properties);
	}

	void create(VkContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		createBuffer(context, size, usage, properties, buffer, bufferMemory);
	}

	void upload(VkContext& context, const vk::raii::CommandPool& commandPool, size_t bufferSize, vk::BufferUsageFlags usage, void* uploadData)
	{
		vk::DeviceSize vkBufferSize = bufferSize;

		GpuBuffer stagingBuffer(context, vkBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBuffer.bufferMemory.mapMemory(0, bufferSize);
		memcpy(data, uploadData, vkBufferSize);
		stagingBuffer.bufferMemory.unmapMemory();

		createBuffer(context, vkBufferSize, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, bufferMemory);

		stagingBuffer.copyTo(context, commandPool, buffer, vkBufferSize);
	}

	void copyTo(VkContext& context, const vk::raii::CommandPool& commandPool, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
	{
		copyBuffer(context, commandPool, buffer, dstBuffer, size);
	}

	void copyTo(VkContext& context, const vk::raii::CommandPool& commandPool, GpuBuffer& dstGpuBuffer, vk::DeviceSize size)
	{
		copyTo(context, commandPool, dstGpuBuffer.buffer, size);
	}
};

}
