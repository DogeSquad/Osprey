#pragma once
#include <vector>
#include <algorithm>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <yaml-cpp/yaml.h>

namespace osp
{
struct PiecewiseLinearCurve 
{
	std::vector<double> knots;
	std::vector<glm::vec3> controlPoints;

	double length;
	std::vector<glm::vec3> controlTangents;

	PiecewiseLinearCurve() 
	{
		knots = std::vector<double>();
		controlPoints = std::vector<glm::vec3>();
		length = 0.0;
	}

	void calculateLength() 
	{
		length = 0.0;

		if (controlPoints.size() <= 1)
			return;

		for (int i = 0; i < controlPoints.size() - 1; i++) 
		{
			length += glm::distance(controlPoints[i], controlPoints[i + 1]);
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

		for (int i = 0; i < N-1; i++)
		{
			controlTangents.push_back(glm::normalize(controlPoints[i+1] - controlPoints[i]));
		}
		controlTangents.push_back(controlTangents[N-2]);
	}

	glm::vec3 getControlPointNormalInterpolated(uint32_t index) 
	{
		if (controlTangents.size() == 0)
		{
			return glm::vec3(0.0, 0.0, 0.0);
		}

		if (index <= 0)
		{
			return controlTangents[0];
		}
		else if (index >= controlTangents.size()-1)
		{
			return controlTangents[controlTangents.size()-1];
		}

		return glm::normalize(0.5f * controlTangents[index] + 0.5f * controlTangents[index-1]);
	}

	glm::vec3 evaluate(double u) 
	{
		// TODO: Add bounds handling
		if (controlPoints.size() == 0) 
		{
			return glm::vec3(0.0, 0.0, 0.0);
		}

		if (u <= 0) 
		{
			return controlPoints[0];
		} 
		else if (u >= length)
		{
			return controlPoints[controlPoints.size()-1];
		}

		auto it = std::lower_bound(knots.begin(), knots.end(), u);

		uint32_t index = std::distance(knots.begin(), it);
		index = (index <= 1) ? 0 : index - 1;

		double sectionLength = knots[index + 1] - knots[index];
		float t = (u - knots[index]) / sectionLength;

		glm::vec3 interp = (1 - t) * controlPoints[index] + t * controlPoints[index + 1];

		return interp;
	}
};

struct Track 
{
	PiecewiseLinearCurve curve;

	Track() 
	{

	}

	void load(const std::string& path) 
	{
		YAML::Node config = YAML::LoadFile(path);

		curve = PiecewiseLinearCurve();
		curve.knots = config["knots"].as<std::vector<double>>();
		if (config["points"]) {
			for (const auto& point : config["points"]) {
				float x = point[0].as<float>();
				float y = point[1].as<float>();
				float z = point[2].as<float>();

				// Add the point to the vector
				curve.controlPoints.push_back(glm::vec3(x, y, z));
			}
		}
		curve.calculateLength();
		curve.calculateTangents();

		//for (int i = 0; i < curve.knots.size(); i++) {
		//	std::cout << curve.knots[i] << ": " << glm::to_string(curve.controlPoints[i]) << std::endl;
		//}
	}
};

}