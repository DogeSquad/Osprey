#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "track.h"

namespace osp
{

struct TrackMesh
{
	const vk::raii::Device& device;
	const vk::raii::PhysicalDevice& physicalDevice;
	const vk::raii::Queue& queue;
	const vk::raii::CommandPool& commandPool;

	Mesh mesh;
	Track track;

	TrackMesh(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		const vk::raii::Queue& queue,
		const vk::raii::CommandPool& commandPool
	)
		: device(device)
		, physicalDevice(physicalDevice)
		, queue(queue)
		, commandPool(commandPool)
		, mesh(device, physicalDevice, queue, commandPool)
		, track()
	{
	}

	void generateMesh()
	{
		mesh.data.vertices.clear();
		mesh.data.indices.clear();

		glm::vec3 tubeColor{ 170, 50, 50 };

		std::vector<Vertex>& vertices = mesh.data.vertices;
		std::vector<uint32_t>& indices = mesh.data.indices;
		const std::vector<glm::vec3>& nodePositions = track.curve.controlPoints;
		const std::vector<glm::vec3>& nodeTangents = track.curve.controlTangents;

		for (int i = 0; i < nodePositions.size(); i++)
		{
			glm::vec3 position = nodePositions[i];
			// Construct Frenet Frame
			glm::vec3 forward = glm::normalize(nodeTangents[i]);
			glm::vec3 right = glm::normalize(glm::cross(forward, UP_DIR));
			glm::vec3 up = glm::normalize(glm::cross(forward, right));

			glm::vec3 vertLeft = position + -right * 0.1;
			mesh.data.vertices.push_back({ vertLeft, tubeColor, {0.0, 0.0}, up });
			glm::vec3 vertRight = position + right * 0.1;
			mesh.data.vertices.push_back({ vertLeft, tubeColor, {0.0, 0.0}, up });

			if (i < nodePositions.size() - 1) 
			{
				indices.push_back(2 * i + 1);
				indices.push_back(2 * (i + 1) + 0);
				indices.push_back(2 * i + 0);

				indices.push_back(2 * i + 1);
				indices.push_back(2 * (i + 1) + 1);
				indices.push_back(2 * (i + 1) + 0);
			}
		}

		mesh.upload();
	}

	void generateMesh2()
	{
		mesh.data.vertices.clear();
		mesh.data.indices.clear();

		// An example of generating tube vertices aligned along the x-axis. NOTE: Calculations rely heavily on predefined function and it being only defined along X.
		glm::vec3 tubeColor{ 170, 50, 50 };
		tubeColor /= 256.0;
		int                    numNodes = 350;
		std::vector<glm::vec3> nodePositions;
		std::vector<glm::vec3> nodeForwards;
		std::vector<glm::vec3> nodeUps;
		nodePositions.reserve(numNodes);
		nodeUps.reserve(numNodes);
		for (int i = 0; i < numNodes; i++)
		{
			float thetaPercent = i / float(numNodes);
			float x = 2.0f * thetaPercent - 1.0f;
			float y = 0.2f * glm::sin(30.0f * thetaPercent);
			nodePositions.push_back({ x, y, 0.0 });

			float dx = 2.0f;
			float dy = 6.0f * glm::cos(30.0f * thetaPercent);
			nodeForwards.push_back(glm::normalize(glm::vec3{ dx, dy, 0.0f }));

			nodeUps.push_back(glm::rotate(nodeForwards[i], -glm::half_pi<float>(), { 0.0f, 0.0, 1.0 }));
		}


		float tubeRadius = 0.01;
		int   verticesPerNode = 15;
		int   i = 0;
		for (; i < nodePositions.size(); i++)
		{
			float percentAlong = i / float(nodePositions.size());
			for (int v = 0; v < verticesPerNode; v++)
			{
				float radians = (v / (float)verticesPerNode) * glm::two_pi<float>();

				glm::vec3 vertexPosition(nodePositions[i]);
				vertexPosition.z += tubeRadius * glm::cos(glm::half_pi<float>() + radians);
				vertexPosition += tubeRadius * nodeUps[i] * glm::sin(glm::half_pi<float>() + radians);

				mesh.data.vertices.push_back({ vertexPosition, tubeColor, {0.0, 0.0}, glm::normalize(vertexPosition - nodePositions[i]) });

				// We do not index the last ring of vertices.
				if (i == nodePositions.size() - 1)
					continue;

				mesh.data.indices.push_back(i * verticesPerNode + v);
				mesh.data.indices.push_back(i * verticesPerNode + ((v + 1) % verticesPerNode));
				mesh.data.indices.push_back((i + 1) * verticesPerNode + ((v + 1) % verticesPerNode));

				mesh.data.indices.push_back((i + 1) * verticesPerNode + ((v + 1) % verticesPerNode));
				mesh.data.indices.push_back((i + 1) * verticesPerNode + v);
				mesh.data.indices.push_back(i * verticesPerNode + v);
			}
		}

		for (i = verticesPerNode; i < nodePositions.size() - verticesPerNode; i++)
		{
			glm::vec3 vertPos = mesh.data.vertices[i].pos;
			glm::vec3 backCross = glm::cross(mesh.data.vertices[i - verticesPerNode].pos - vertPos, mesh.data.vertices[i - 1].pos - vertPos);
			glm::vec3 forwardCross = glm::cross(mesh.data.vertices[i + verticesPerNode].pos - vertPos, mesh.data.vertices[i + 1].pos - vertPos);
			glm::vec3 interpolatedNormal = glm::mix(backCross, forwardCross, 0.5f);
			mesh.data.vertices[i].normal = glm::normalize(interpolatedNormal);
		}
		
		mesh.upload();
	}
};

}