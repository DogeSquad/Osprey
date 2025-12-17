#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <glm/gtx/rotate_vector.hpp>

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

		glm::vec3 tubeColor{ 0, 170, 0 };
		tubeColor /= 256.0;
		float tubeRadius = 0.01f;
		int   verticesPerRing = 15;
		int   ringsPerNode = 2;

		std::vector<Vertex>& vertices = mesh.data.vertices;
		std::vector<uint32_t>& indices = mesh.data.indices;
		const std::vector<glm::vec3>& nodePositions = track.curve.controlPoints;
		const std::vector<glm::vec3>& nodeTangents = track.curve.controlTangents;
		const std::vector<double>& nodeRoll = track.roll;

		for (int i = 0; i < nodePositions.size(); i++)
		{
			glm::vec3 position = nodePositions[i];
			// Construct Frenet Frame
			glm::vec3 forward = glm::normalize(nodeTangents[i]);

			glm::mat3 rotation = glm::rotate(glm::radians((float)nodeRoll[i]), forward);
			glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
			glm::vec3 up = -glm::normalize(glm::cross(forward, right));

			// Construct Geometry
			vertices.push_back({ nodePositions[i] -    up * 0.1, tubeColor, {0.0, 0.0}, -up });
			vertices.push_back({ nodePositions[i] + right * 0.1, tubeColor, {0.0, 0.0},  up });
			vertices.push_back({ nodePositions[i] - right * 0.1, tubeColor, {0.0, 0.0},  up });

			// Inner Beams
			indices.push_back(3 * (i + 0) + 0);
			indices.push_back(3 * (i + 0) + 1);
			indices.push_back(3 * (i + 0) + 1);

			indices.push_back(3 * (i + 0) + 1);
			indices.push_back(3 * (i + 0) + 2);
			indices.push_back(3 * (i + 0) + 2);

			indices.push_back(3 * (i + 0) + 2);
			indices.push_back(3 * (i + 0) + 0);
			indices.push_back(3 * (i + 0) + 0);

			if (i == nodePositions.size() - 1)
			{
				break;
			}

			// Tangent Beams
			indices.push_back(3 * (i + 0) + 0);
			indices.push_back(3 * (i + 1) + 0);
			indices.push_back(3 * (i + 1) + 0);

			indices.push_back(3 * (i + 0) + 1);
			indices.push_back(3 * (i + 1) + 1);
			indices.push_back(3 * (i + 1) + 1);

			indices.push_back(3 * (i + 0) + 2);
			indices.push_back(3 * (i + 1) + 2);
			indices.push_back(3 * (i + 1) + 2);

			continue;

			// SIDE A
			indices.push_back(3 * (i+0) + 0);
			indices.push_back(3 * (i+1) + 1);
			indices.push_back(3 * (i+0) + 1);

			indices.push_back(3 * (i+0) + 0);
			indices.push_back(3 * (i+1) + 0);
			indices.push_back(3 * (i+1) + 1);

			// SIDE B
			indices.push_back(3 * (i + 0) + 1);
			indices.push_back(3 * (i + 1) + 2);
			indices.push_back(3 * (i + 0) + 2);

			indices.push_back(3 * (i + 0) + 1);
			indices.push_back(3 * (i + 1) + 1);
			indices.push_back(3 * (i + 1) + 2);

			// SIDE C
			indices.push_back(3 * (i + 0) + 2);
			indices.push_back(3 * (i + 1) + 0);
			indices.push_back(3 * (i + 0) + 0);

			indices.push_back(3 * (i + 0) + 2);
			indices.push_back(3 * (i + 1) + 2);
			indices.push_back(3 * (i + 1) + 0);
		}

		mesh.upload();
	}

	void generateMesh3()
	{
		mesh.data.vertices.clear();
		mesh.data.indices.clear();

		glm::vec3 tubeColor{ 170, 50, 50 };
		tubeColor /= 256.0;
		float tubeRadius = 0.01f;
		int   verticesPerRing = 15;
		int   ringsPerNode = 2;

		std::vector<Vertex>& vertices = mesh.data.vertices;
		std::vector<uint32_t>& indices = mesh.data.indices;
		const std::vector<glm::vec3>& nodePositions = track.curve.controlPoints;
		const std::vector<glm::vec3>& nodeTangents = track.curve.controlTangents;
		const std::vector<double>& nodeRoll = track.roll;

		glm::vec3 prevForward;
		glm::vec3 prevRight;
		glm::vec3 prevUp;
		bool first = true;
		glm::vec3 transportRight;
		glm::vec3 transportUp;

		for (int i = 0; i < nodePositions.size(); i++)
		{
			glm::vec3 position = nodePositions[i];
			// Construct Frenet Frame
			//glm::vec3 forward = glm::normalize(nodeTangents[i]);

			//glm::mat3 rotation = glm::rotate(glm::radians((float)nodeRoll[i]), forward);
			//glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
			//glm::vec3 up = rotation * glm::normalize(glm::cross(forward, right));
			//	
			glm::vec3 forward = glm::normalize(nodeTangents[i]);

			if (first)
			{
				glm::vec3 upGuess = UP_DIR;
				if (glm::abs(glm::dot(upGuess, forward)) > 0.9f)
					upGuess = glm::vec3(1, 0, 0);

				prevRight = glm::normalize(glm::cross(forward, upGuess));
				prevUp = glm::cross(prevRight, forward);
				transportRight = glm::normalize(glm::cross(forward, upGuess));
				transportUp = glm::cross(transportRight, forward);
				first = false;
			}
			else
			{
				// Parallel transport
				glm::vec3 axis = glm::cross(prevForward, forward);
				float len = glm::length(axis);

				if (len > 1e-5f)
				{
					axis /= len;
					float angle = glm::acos(glm::clamp(glm::dot(prevForward, forward), -1.0f, 1.0f));
					transportRight = glm::rotate(transportRight, angle, axis);
					transportUp = glm::cross(transportRight, forward);
				}
			}
			float roll = glm::radians((float)nodeRoll[i]);

			glm::vec3 right = glm::rotate(transportRight, roll, forward);
			glm::vec3 up = glm::rotate(transportUp, roll, forward);
			prevForward = forward;
			transportRight = glm::normalize(transportRight);
			transportUp = glm::normalize(glm::cross(forward, transportRight));

			// Construct Geometry
			glm::vec3 tubeCenter = position + -right * 1.0;

			auto appendVerticesRing = [&](glm::vec3 tubeCenter)
			{
				for (int v = 0; v < verticesPerRing; v++)
				{
					float radians = (v / (float)verticesPerRing) * glm::two_pi<float>();
					glm::vec3 vertexPosition(nodePositions[i]);
					vertexPosition += tubeRadius * right * glm::cos(glm::half_pi<float>() + radians);
					vertexPosition += tubeRadius * up * glm::sin(glm::half_pi<float>() + radians);

					vertices.push_back({ vertexPosition, tubeColor, {0.0, 0.0}, glm::normalize(vertexPosition - tubeCenter) });
				}
			};
			appendVerticesRing(tubeCenter);
			tubeCenter = position + right * 1.0;
			appendVerticesRing(tubeCenter);

			if (i == nodePositions.size() - 1)
			{
				break;
			}

			uint32_t currBase = i * ringsPerNode * verticesPerRing;
			uint32_t nextBase = (i + 1) * ringsPerNode * verticesPerRing;
			for (int r = 0; r < ringsPerNode; r++)
			{
				uint32_t ringOffset = r * verticesPerRing;

				for (int v = 0; v < verticesPerRing; v++)
				{
					uint32_t v0 = currBase + ringOffset + v;
					uint32_t v1 = currBase + ringOffset + (v + 1) % verticesPerRing;
					uint32_t v2 = nextBase + ringOffset + (v + 1) % verticesPerRing;
					uint32_t v3 = nextBase + ringOffset + v;

					// First triangle
					indices.push_back(v0);
					indices.push_back(v1);
					indices.push_back(v2);

					// Second triangle
					indices.push_back(v2);
					indices.push_back(v3);
					indices.push_back(v0);
				}
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