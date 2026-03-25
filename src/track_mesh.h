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
	Mesh mesh;
	Track* track;

	const float sampleSpacing = 0.01f;
	const int   tieEvery = 50;
	const float railOffset = 0.075f;
	const float railRadius = 0.015f;

	const int segments = 30;

	TrackMesh() = default;

	void generateTube(
		const std::vector<glm::vec3>& positions,
		const std::vector<glm::mat3>& frames,
		glm::vec2 offset, float radius, int segments, glm::vec3 color) 
	{
		auto& vertices = mesh.data.vertices;
		auto& indices = mesh.data.indices;

		uint32_t baseVertex = vertices.size();

		for (int i = 0; i < positions.size(); i++) {
			glm::vec3 right = frames[i][0];
			glm::vec3 up = frames[i][1];
			glm::vec3 center = positions[i] + offset.x * right + offset.y * up;

			for (int s = 0; s < segments; s++) {
				float angle = (float)s / segments * glm::two_pi<float>();
				glm::vec3 normal = glm::cos(angle) * right + glm::sin(angle) * up;
				glm::vec3 pos = center + normal * radius;

				vertices.push_back({
					pos,
					color,
					glm::vec2((float)s / segments, (float)i / positions.size()),
					normal
				});

				// Indices
				if (i == positions.size() - 1) continue;

				uint32_t a = baseVertex + i * segments + s;
				uint32_t b = baseVertex + i * segments + (s + 1) % segments;
				uint32_t c = baseVertex + (i + 1) * segments + s;
				uint32_t d = baseVertex + (i + 1) * segments + (s + 1) % segments;

				indices.push_back(a);
				indices.push_back(c);
				indices.push_back(b);

				indices.push_back(b);
				indices.push_back(c);
				indices.push_back(d);
			}
		}
	}

	void generateCrossTie(
		glm::vec3 center, glm::vec3 right, glm::vec3 up,
		glm::vec3 forward, float railOffset, glm::vec3 color,
		int segments = 10, float tieRadius = 0.008f)
	{
		glm::vec3 leftPos = center - right * railOffset;
		glm::vec3 rightPos = center + right * railOffset;

		// build two positions and frames for a single tube segment
		std::vector<glm::vec3> tiePositions = { leftPos, rightPos };

		// for the tie, "forward" is along right axis
		// so we need a frame where right/up are perpendicular to the tie direction
		glm::vec3 tieForward = glm::normalize(rightPos - leftPos);
		glm::vec3 tieRight = forward; // track forward becomes tie's radial axis
		glm::vec3 tieUp = glm::normalize(glm::cross(tieForward, tieRight));
		tieRight = glm::normalize(glm::cross(tieUp, tieForward));

		std::vector<glm::mat3> tieFrames = {
			glm::mat3(tieRight, tieUp, tieForward),
			glm::mat3(tieRight, tieUp, tieForward)
		};

		generateTube(tiePositions, tieFrames, glm::vec2(0.0f), tieRadius, segments, color);
	}

	void generateMesh()
	{
		if (!track) return;

		mesh.data.vertices.clear();
		mesh.data.indices.clear();

		//glm::vec3 color{ 0.95f, 0.05f, 0.1f };
		glm::vec3 color{ 0.0f, 0.05f, 0.95f };

		std::vector<glm::vec3> positions;
		std::vector<glm::mat3> frames;

		float totalLength = track->totalLength();
		int numSamples = static_cast<int>(totalLength / sampleSpacing);

		for (int i = 0; i <= numSamples; i++) {
			float s = (float)i / (float)numSamples * totalLength;
			glm::mat4 fren = track->evaluateFrenet(s);
			glm::vec3 pos = fren[3];

			glm::vec3 right = glm::vec3(fren[0]);
			glm::vec3 up = glm::vec3(fren[1]);
			glm::vec3 forward = glm::vec3(fren[2]);

			positions.push_back(pos);
			frames.push_back(glm::mat3(right, up, forward));
		}

		generateTube(positions, frames, glm::vec2(-railOffset, 0.0f), railRadius, segments, color);
		generateTube(positions, frames, glm::vec2(railOffset, 0.0f), railRadius, segments, color);

		// cross ties
		for (int i = 0; i < positions.size(); i += tieEvery) {
			generateCrossTie(positions[i], frames[i][0], frames[i][1], frames[i][2], railOffset, color);
		}
	}

	void generateWireframeMesh()
	{
		if (!track) return;

		mesh.data.vertices.clear();
		mesh.data.indices.clear();

		glm::vec3 tubeColor{ 0, 170, 0 };
		tubeColor /= 256.0;
		float tubeRadius = 0.01f;
		int   verticesPerRing = 15;
		int   ringsPerNode = 2;

		std::vector<Vertex>& vertices = mesh.data.vertices;
		std::vector<uint32_t>& indices = mesh.data.indices;

		// TODO: Proper generation
		const std::vector<glm::vec3>* nodeTangents;

		if (PiecewiseLinearCurve* linCurve = dynamic_cast<PiecewiseLinearCurve*>(track->curve.get())) {
			nodeTangents = &linCurve->controlTangents;
		}
		else if (HermiteCurve* hermCurve = dynamic_cast<HermiteCurve*>(track->curve.get())){
			nodeTangents = &hermCurve->controlTangents;
		}

		const std::vector<float>& nodeRoll = track->roll;





		std::vector<glm::vec3> positions;
		std::vector<glm::mat3> frames;

		float totalLength = track->totalLength();
		int numSamples = static_cast<int>(totalLength / sampleSpacing);

		for (int i = 0; i <= numSamples; i++) {
			float s = (float)i / (float)numSamples * totalLength;
			glm::mat4 fren = track->evaluateFrenet(s);
			glm::vec3 pos = fren[3];

			glm::vec3 right = glm::vec3(fren[0]);
			glm::vec3 up = glm::vec3(fren[1]);
			glm::vec3 forward = glm::vec3(fren[2]);

			positions.push_back(pos);
			frames.push_back(glm::mat3(right, up, forward));
		}

		generateTube(positions, frames, glm::vec2(-railOffset, 0.0f), 0.0f, 1, tubeColor);
		generateTube(positions, frames, glm::vec2(railOffset, 0.0f), 0.0f, 1, tubeColor);



		// cross ties
		for (int i = 0; i < positions.size(); i += tieEvery) {
			generateCrossTie(positions[i], frames[i][0], frames[i][1], frames[i][2], railOffset, tubeColor, 1, 0.0f);
		}











		//const size_t numNodes = track->curve->getNumControlNodes();

		//for (int i = 0; i < numNodes; i++)
		//{
		//	glm::vec3 position = track->curve->getControlPoint(i);
		//	// Construct Frenet Frame
		//	glm::vec3 forward = nodeTangents[i]->clone();
		//	forward = glm::normalize(nodeTangents[i]);

		//	glm::mat3 rotation = glm::rotate(glm::radians((float)nodeRoll[i]), forward);
		//	glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
		//	glm::vec3 up = -glm::normalize(glm::cross(forward, right));

		//	// Construct Geometry
		//	vertices.push_back({ nodePositions[i] -    up * 0.1, tubeColor, {0.0, 0.0}, -up });
		//	vertices.push_back({ nodePositions[i] + right * 0.1, tubeColor, {0.0, 0.0},  up });
		//	vertices.push_back({ nodePositions[i] - right * 0.1, tubeColor, {0.0, 0.0},  up });

		//	// Inner Beams
		//	indices.push_back(3 * (i + 0) + 0);
		//	indices.push_back(3 * (i + 0) + 1);
		//	indices.push_back(3 * (i + 0) + 1);

		//	indices.push_back(3 * (i + 0) + 1);
		//	indices.push_back(3 * (i + 0) + 2);
		//	indices.push_back(3 * (i + 0) + 2);

		//	indices.push_back(3 * (i + 0) + 2);
		//	indices.push_back(3 * (i + 0) + 0);
		//	indices.push_back(3 * (i + 0) + 0);

		//	if (i == nodePositions.size() - 1)
		//	{
		//		break;
		//	}

		//	// Tangent Beams
		//	indices.push_back(3 * (i + 0) + 0);
		//	indices.push_back(3 * (i + 1) + 0);
		//	indices.push_back(3 * (i + 1) + 0);

		//	indices.push_back(3 * (i + 0) + 1);
		//	indices.push_back(3 * (i + 1) + 1);
		//	indices.push_back(3 * (i + 1) + 1);

		//	indices.push_back(3 * (i + 0) + 2);
		//	indices.push_back(3 * (i + 1) + 2);
		//	indices.push_back(3 * (i + 1) + 2);

		//	continue;

		//	// SIDE A
		//	indices.push_back(3 * (i+0) + 0);
		//	indices.push_back(3 * (i+1) + 1);
		//	indices.push_back(3 * (i+0) + 1);

		//	indices.push_back(3 * (i+0) + 0);
		//	indices.push_back(3 * (i+1) + 0);
		//	indices.push_back(3 * (i+1) + 1);

		//	// SIDE B
		//	indices.push_back(3 * (i + 0) + 1);
		//	indices.push_back(3 * (i + 1) + 2);
		//	indices.push_back(3 * (i + 0) + 2);

		//	indices.push_back(3 * (i + 0) + 1);
		//	indices.push_back(3 * (i + 1) + 1);
		//	indices.push_back(3 * (i + 1) + 2);

		//	// SIDE C
		//	indices.push_back(3 * (i + 0) + 2);
		//	indices.push_back(3 * (i + 1) + 0);
		//	indices.push_back(3 * (i + 0) + 0);

		//	indices.push_back(3 * (i + 0) + 2);
		//	indices.push_back(3 * (i + 1) + 2);
		//	indices.push_back(3 * (i + 1) + 0);
		//}
	}

	void upload(VkContext& context, vk::raii::CommandPool& commandPool)
	{
		mesh.upload(context, commandPool);
	}
};

}