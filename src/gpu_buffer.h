#pragma once

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
	const vk::raii::Device& device;
	const vk::raii::PhysicalDevice& physicalDevice;
	const vk::raii::Queue& queue;
	const vk::raii::CommandPool& commandPool;

	vk::raii::Buffer       buffer = nullptr;
	vk::raii::DeviceMemory bufferMemory = nullptr;

	GpuBuffer(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		const vk::raii::Queue& queue,
		const vk::raii::CommandPool& commandPool
	)
		: device(device)
		, physicalDevice(physicalDevice)
		, queue(queue)
		, commandPool(commandPool)
	{
	}

	void uploadData(size_t bufferSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, void* uploadData)
	{
		vk::DeviceSize vkBufferSize = bufferSize;

		vk::raii::Buffer       stagingBuffer({});
		vk::raii::DeviceMemory stagingBufferMemory({});
		createBuffer(vkBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

		void* data = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(data, uploadData, vkBufferSize);
		stagingBufferMemory.unmapMemory();

		createBuffer(vkBufferSize, usage, properties, buffer, bufferMemory);

		copyBuffer(stagingBuffer, buffer, vkBufferSize);
	}

	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
	{
		vk::BufferCreateInfo bufferInfo{
			.size = size,
			.usage = usage,
			.sharingMode = vk::SharingMode::eExclusive };
		buffer = vk::raii::Buffer(device, bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) };
		bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		buffer.bindMemory(bufferMemory, 0);
	}

	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::raii::CommandBuffer       commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
		commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{ .size = size });
		commandCopyBuffer.end();
		queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, nullptr);
		queue.waitIdle();
	}


	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
};

}
