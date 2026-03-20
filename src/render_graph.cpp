#include "render_graph.h"

uint32_t FindMemoryType(vk::raii::PhysicalDevice* physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice->getMemoryProperties();

	// Find suitable memory type
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

namespace osp {
	void RenderGraph::AddResource(const std::string& name, vk::Format format, vk::Extent2D extent, vk::ImageUsageFlags usage, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
	{
		ImageResource resource(name);
		resource.format = format;
		resource.extent = extent;
		resource.usage = usage;
		resource.initialLayout = initialLayout;
		resource.finalLayout = finalLayout;

		resources.insert_or_assign(name, std::move(resource));
	}

	void RenderGraph::AddPass(std::string name, std::function<void(vk::raii::CommandBuffer&)> execute)
	{
		passes.emplace_back(std::move(name), std::move(execute));
	}
	void RenderGraph::AddPass(const std::string& name, const std::vector<std::string>& inputs, const std::vector<std::string>& outputs, std::function<void(vk::raii::CommandBuffer&)> executeFunc)
	{
		Pass pass;
		pass.name = name;
		pass.inputs = inputs;
		pass.outputs = outputs;
		pass.executeFunc = executeFunc;

		passes.push_back(std::move(pass));
	}

	void RenderGraph::Compile()
	{
		std::vector<std::vector<size_t>> dependencies(passes.size());
		std::vector<std::vector<size_t>> dependents(passes.size());

		std::unordered_map<std::string, size_t> resourceWriters;

		for (size_t i = 0; i < passes.size(); ++i) {
			const auto& pass = passes[i];

			for (const auto& input : pass.inputs) {
				auto it = resourceWriters.find(input);
				if (it != resourceWriters.end()) {
					dependencies[i].push_back(it->second);
					dependents[it->second].push_back(i);
				}
			}

			for (const auto& output : pass.outputs) {
				resourceWriters[output] = i;
			}
		}

		std::vector<bool> visited(passes.size(), false);  
		std::vector<bool> inStack(passes.size(), false);

		std::function<void(size_t)> visit = [&](size_t node) {
			if (inStack[node]) {
				throw std::runtime_error("Cycle detected in rendergraph");
			}

			if (visited[node]) {
				return;
			}

			inStack[node] = true;

			for (auto dependent : dependents[node]) {
				visit(dependent);
			}

			inStack[node] = false;
			visited[node] = true;
			executionOrder.push_back(node);
		};

		for (size_t i = 0; i < passes.size(); ++i) {
			if (!visited[i]) {
				visit(i);
			}
		}

		// Create Vulkan synchronization objects

		for (size_t i = 0; i < passes.size(); ++i) {
			for (auto dep : dependencies[i]) {
				semaphores.emplace_back(device->createSemaphore({}));
				semaphoreSignalWaitPairs.emplace_back(dep, i);
			}
		}

		for (auto& [name, resource] : resources) {
			vk::ImageCreateInfo imageInfo;
			imageInfo.setImageType(vk::ImageType::e2D)
				.setFormat(resource.format)
				.setExtent({ resource.extent.width, resource.extent.height, 1 }) 
				.setMipLevels(1)
				.setArrayLayers(1)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(resource.usage)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setInitialLayout(vk::ImageLayout::eUndefined);

			resource.image = device->createImage(imageInfo);

			// Allocate backing memory for the image
			vk::MemoryRequirements memRequirements = resource.image.getMemoryRequirements();

			vk::MemoryAllocateInfo allocInfo;
			allocInfo.setAllocationSize(memRequirements.size)
				.setMemoryTypeIndex(FindMemoryType(physicalDevice, memRequirements.memoryTypeBits,
					vk::MemoryPropertyFlagBits::eDeviceLocal));

			resource.memory = device->allocateMemory(allocInfo);
			resource.image.bindMemory(*resource.memory, 0);

			// Create image view for shader access
			vk::ImageViewCreateInfo viewInfo;
			viewInfo.setImage(*resource.image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(resource.format)
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

			resource.imageView = device->createImageView(viewInfo);
		}
	}

	void RenderGraph::Execute(vk::raii::CommandBuffer& cmd, vk::Queue queue)
	{
		std::vector<vk::CommandBuffer> cmdBuffers;
		std::vector<vk::Semaphore> waitSemaphores;
		std::vector<vk::PipelineStageFlags> waitStages;
		std::vector<vk::Semaphore> signalSemaphores;

		for (auto passIdx : executionOrder) {
			const auto& pass = passes[passIdx];

			waitSemaphores.clear();
			waitStages.clear();

			for (size_t i = 0; i < semaphoreSignalWaitPairs.size(); ++i) {
				if (semaphoreSignalWaitPairs[i].second != passIdx) continue;

				waitSemaphores.push_back(*semaphores[i]);
				waitStages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			}

			signalSemaphores.clear();
			for (size_t i = 0; i < semaphoreSignalWaitPairs.size(); ++i) {
				if (semaphoreSignalWaitPairs[i].first != passIdx) continue;
				
				signalSemaphores.push_back(*semaphores[i]);
			}

			cmd.begin({});

			for (const auto& input : pass.inputs) {
				auto& resource = resources[input];

				vk::ImageMemoryBarrier barrier;
				barrier.setOldLayout(resource.initialLayout)
					.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(*resource.image)
					.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
					.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite)
					.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

				cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,
					vk::PipelineStageFlagBits::eFragmentShader,
					vk::DependencyFlagBits::eByRegion,
					nullptr, nullptr, barrier // MIGHT BE FAULTY
				);
			}

			for (const auto& output : pass.outputs) {
				auto& resource = resources[output];

				vk::ImageMemoryBarrier barrier;
				barrier.setOldLayout(resource.initialLayout)
					.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(*resource.image)
					.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
					.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
					.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

				cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,
					vk::PipelineStageFlagBits::eColorAttachmentOutput,
					vk::DependencyFlagBits::eByRegion,
					nullptr, nullptr, barrier
				);
			}

			pass.executeFunc(cmd);

			for (const auto& output : pass.outputs) {
				auto& resource = resources[output];

				vk::ImageMemoryBarrier barrier;
				barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
					.setNewLayout(resource.finalLayout)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setImage(*resource.image)
					.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
					.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
					.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

				cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eAllCommands,
					vk::PipelineStageFlagBits::eColorAttachmentOutput,
					vk::DependencyFlagBits::eByRegion,
					nullptr, nullptr, barrier
				);
			}
		}

		cmd.end();

		vk::SubmitInfo submitInfo;
		submitInfo.setWaitSemaphoreCount(static_cast<uint32_t>(waitSemaphores.size()))
			.setPWaitSemaphores(waitSemaphores.data())
			.setPWaitDstStageMask(waitStages.data())
			.setCommandBufferCount(1)
			.setPCommandBuffers(&*cmd)
			.setSignalSemaphoreCount(static_cast<uint32_t>(signalSemaphores.size()))
			.setPSignalSemaphores(signalSemaphores.data());

		queue.submit(1, &submitInfo, nullptr);

		//for (auto& pass : passes) {
		//	pass.executeFunc(cmd);
		//}
	}

	void RenderGraph::Clear()
	{
		passes.clear();
	}
} // namespace osp