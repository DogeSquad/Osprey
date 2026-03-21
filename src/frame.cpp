#include "frame.h"

#include "memory_utils.h"

namespace osp {

Frame::Frame(VkContext& context, vk::raii::CommandPool& commandPool, vk::raii::DescriptorPool& descriptorPool, vk::DescriptorSetLayout layout)
{
	// Set up descriptor sets
	vk::DeviceSize         bufferSize = sizeof(UniformBufferObject);

	createBuffer(context, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffer, uniformBufferMemory);
	uniformBufferMapped = uniformBufferMemory.mapMemory(0, bufferSize);

	vk::DescriptorSetAllocateInfo descriptorAllocInfo{
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout };
	descriptorSet = std::move(context.device.allocateDescriptorSets(descriptorAllocInfo).front());

	vk::DescriptorBufferInfo bufferInfo{
		.buffer = uniformBuffer,
		.offset = 0,
		.range = sizeof(UniformBufferObject) 
	};
	vk::WriteDescriptorSet write{
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.pBufferInfo = &bufferInfo
	};
	context.device.updateDescriptorSets(write, {});

	// Set up command buffer
	vk::CommandBufferAllocateInfo cmdAllocInfo{ 
		.commandPool = commandPool, 
		.level = vk::CommandBufferLevel::ePrimary, 
		.commandBufferCount = 1 };
	cmd = std::move(vk::raii::CommandBuffers(context.device, cmdAllocInfo).front());

	imageAvailable = vk::raii::Semaphore(context.device, vk::SemaphoreCreateInfo());

	inFlight = vk::raii::Fence(context.device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
}

void Frame::updateUBO(glm::mat4 model, glm::mat4 view, glm::mat4 proj, glm::vec3 lightDir)
{
	UniformBufferObject ubo{};
	ubo.model = model;
	ubo.view = view;
	ubo.proj = proj;
	ubo.invView = glm::inverse(view);
	ubo.invProj = glm::inverse(proj);
	ubo.lightDir = lightDir;

	memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
}

}