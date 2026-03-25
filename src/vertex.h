#pragma once

#include <vulkan/vulkan_raii.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace osp {

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

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};


} // namespace osp

template <>
struct std::hash<osp::Vertex>
{
	size_t operator()(osp::Vertex const& vertex) const noexcept
	{
		auto h = std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec3>()(vertex.color) << 1);
		h = (h >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
		h = (h >> 1) ^ (std::hash<glm::vec3>()(vertex.normal) << 1);
		return h;
	}
};