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
	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	} data;
	GpuBuffer vertexBuffer;
	GpuBuffer indexBuffer;

	Mesh() = default;

	void upload(VkContext& context, vk::raii::CommandPool& commandPool)
	{
		vertexBuffer.upload(context, commandPool, sizeof(data.vertices[0]) * data.vertices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, data.vertices.data());
		indexBuffer.upload(context, commandPool, sizeof(data.indices[0]) * data.indices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, data.indices.data());
	}
};

}