#pragma once

#include "vk_context.h"

namespace osp {

inline uint32_t findMemoryType(VkContext& context, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = context.physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

inline void createBuffer(VkContext& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
{
	vk::BufferCreateInfo bufferInfo{
		.size = size,
		.usage = usage,
		.sharingMode = vk::SharingMode::eExclusive };
	buffer = vk::raii::Buffer(context.device, bufferInfo);
	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties) };
	bufferMemory = vk::raii::DeviceMemory(context.device, allocInfo);
	buffer.bindMemory(bufferMemory, 0);
}

} // namespace osp