#pragma once

#include "curve.h"

#include "constants.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace osp {
	// cubic hermite basis
	static glm::vec3 hermiteInterpolate(
		glm::vec3 p0, glm::vec3 m0,
		glm::vec3 p1, glm::vec3 m1, float t)
	{
		float t2 = t * t;
		float t3 = t2 * t;

		float h00 = 2 * t3 - 3 * t2 + 1;
		float h10 = t3 - 2 * t2 + t;
		float h01 = -2 * t3 + 3 * t2;
		float h11 = t3 - t2;

		return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
	}

	// derivative of hermite for tangent direction
	static glm::vec3 hermiteDerivative(
		glm::vec3 p0, glm::vec3 m0,
		glm::vec3 p1, glm::vec3 m1, float t)
	{
		float t2 = t * t;

		float h00 = 6 * t2 - 6 * t;
		float h10 = 3 * t2 - 4 * t + 1;
		float h01 = -6 * t2 + 6 * t;
		float h11 = 3 * t2 - 2 * t;

		return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
	}

	struct HermiteCurve : public ICurve {
		std::vector<float> knots;
		std::vector<glm::vec3> controlPoints;

		std::vector<glm::vec3> controlTangents;

		std::vector<float> segmentLengths;
		std::vector<float> cumulativeLengths;

		HermiteCurve() = default;

		void update() override
		{
			calculateTangents();
			calculateLength();
		}

		float totalLength() const override {
			return cumulativeLengths.empty() ? 0.0f : cumulativeLengths.back();
		}

		void extendBack() override
		{
			float segmentLength = segmentLengths.back();
			glm::vec3 newControlPoint = controlPoints.back() + segmentLength * controlTangents.back();
			knots.push_back(knots.back() + 1);
			controlTangents.push_back(controlTangents.back());
			controlPoints.push_back(newControlPoint);
			cumulativeLengths.push_back(cumulativeLengths.back() + segmentLength);
			segmentLengths.push_back(segmentLength);
		}
		void removeBack() override
		{
			knots.pop_back();
			controlPoints.pop_back();
			controlTangents.pop_back();
			cumulativeLengths.pop_back();
			segmentLengths.pop_back();
		}

		float approximateSegmentLength(size_t seg, int steps) {
			float len = 0.0f;
			glm::vec3 prev = controlPoints[seg];
			for (int j = 1; j <= steps; j++) {
				float t = (float)j / steps;
				glm::vec3 curr = hermiteInterpolate(
					controlPoints[seg], controlTangents[seg],
					controlPoints[seg + 1], controlTangents[seg + 1], t);
				len += glm::distance(prev, curr);
				prev = curr;
			}
			return len;
		}

		void calculateLength()
		{
			if (controlPoints.size() <= 1)
				return;
			segmentLengths.clear();
			cumulativeLengths.clear();

			segmentLengths.reserve(controlPoints.size() - 1);
			cumulativeLengths.reserve(controlPoints.size() - 1);

			float totalLength = 0.0;
			for (int i = 0; i < controlPoints.size() - 1; i++)
			{
				segmentLengths.push_back(approximateSegmentLength(i, 16));
				totalLength += segmentLengths[i];
				cumulativeLengths.push_back(totalLength);
			}
		}

		void calculateTangents()
		{
			uint32_t N = controlPoints.size();
			if (N <= 1)
			{
				return;
			}

			controlTangents = std::vector<glm::vec3>();
			controlTangents.reserve(N);

			controlTangents.push_back(glm::normalize(controlPoints[1] - controlPoints[0]));

			for (int i = 1; i < N - 1; i++)
			{
				controlTangents.push_back(0.5f * (controlPoints[i + 1] - controlPoints[i - 1]));
			}
			controlTangents.push_back(glm::normalize(controlPoints[N - 1] - controlPoints[N - 2]));
		}

		float normalizedToArcLength(float u)  override
		{
			if (cumulativeLengths.empty())
				return 0.0f;
			return u * cumulativeLengths.back();
		}

		float arcLengthToNormalized(float s)  override
		{
			if (cumulativeLengths.empty())
				return 0.0f;
			return s / cumulativeLengths.back();
		}

		size_t getSegmentAtLength(float s)  override
		{
			if (cumulativeLengths.empty())
				return 0;
			s = glm::clamp(s, 0.0f, cumulativeLengths.back());

			// Find segment (maybe implement binary search?)
			size_t seg = 0;
			while (cumulativeLengths[seg] < s)
				seg++;

			return seg;
		}

		glm::vec3 evaluate(float s, size_t* i = nullptr) override
		{
			if (cumulativeLengths.empty())
			{
				return glm::vec3(0.0, 0.0, 0.0);
			}
			s = glm::clamp(s, 0.0f, cumulativeLengths.empty() ? 0.0f : cumulativeLengths.back());

			// Find segment (maybe implement binary search?)
			size_t seg = getSegmentAtLength(s);

			float segStart = (seg == 0 ? 0.0f : cumulativeLengths[seg - 1]);
			float t = (s - segStart) / segmentLengths[seg];

			if (i != nullptr)
			{
				*i = seg;
			}

			return hermiteInterpolate(controlPoints[seg], controlTangents[seg], controlPoints[seg+1], controlTangents[seg+1], t);
		}

		glm::vec3 evaluateNormalized(float u)
		{
			return evaluate(normalizedToArcLength(u));
		}

		glm::mat4 evaluateFrenetInterpolated(float s, const std::vector<float>& roll) override
		{
			size_t i = 0;
			glm::vec3 position = evaluate(s, &i);
			float localT;
			if (i == 0)
			{
				localT = s / segmentLengths[0];
			}
			else
			{
				localT = (s - cumulativeLengths[i - 1]) / segmentLengths[i]; // TODO Division by 0?
			}
			// Construct Frenet Frame
			glm::vec3 forward = glm::normalize(hermiteDerivative(
				controlPoints[i], controlTangents[i],
				controlPoints[i + 1], controlTangents[i + 1], s));

			glm::mat3 rotation = glm::rotate(glm::radians((float)glm::mix(roll[i], roll[i + 1], localT)), forward);
			glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
			glm::vec3 up = -glm::normalize(glm::cross(forward, right));

			glm::mat4 frenet = glm::identity<glm::mat4>();
			frenet[3][3] = 1.0f;
			setColumn(frenet, right, 0);
			setColumn(frenet, up, 1);
			setColumn(frenet, forward, 2);
			setColumn(frenet, position, 3);

			return frenet;
		}

		glm::mat4 evaluateFrenet(float s, const std::vector<float>& roll) override
		{
			size_t i = 0;
			glm::vec3 position = evaluate(s, &i);

			// Construct Frenet Frame
			glm::vec3 forward = glm::normalize(controlTangents[i]);

			glm::mat3 rotation = glm::rotate(glm::radians((float)roll[i]), forward);
			glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
			glm::vec3 up = -glm::normalize(glm::cross(forward, right));

			glm::mat4 frenet = glm::identity<glm::mat4>();
			setColumn(frenet, right, 0);
			setColumn(frenet, up, 1);
			setColumn(frenet, forward, 2);
			setColumn(frenet, position, 3);

			return frenet;
		}
	};

} // namespace osp