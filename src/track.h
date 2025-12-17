#pragma once
#include <vector>
#include <algorithm>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <yaml-cpp/yaml.h>

#include "mesh.h"
#include "constants.h"

namespace osp
{
struct PiecewiseLinearCurve 
{
	std::vector<float> knots;
	std::vector<glm::vec3> controlPoints;

	std::vector<glm::vec3> controlTangents;

	std::vector<float> segmentLengths;
	std::vector<float> cumulativeLengths;

	PiecewiseLinearCurve() 
	{
		knots = std::vector<float>();
		controlPoints = std::vector<glm::vec3>();

		segmentLengths = std::vector<float>();
		cumulativeLengths = std::vector<float>();
	}

	void calculateLength() 
	{

		if (controlPoints.size() <= 1)
			return;

		segmentLengths.reserve(controlPoints.size()-1);
		cumulativeLengths.reserve(controlPoints.size()-1);

		float totalLength = 0.0;
		for (int i = 0; i < controlPoints.size() - 1; i++) 
		{
			segmentLengths.push_back(glm::distance(controlPoints[i], controlPoints[i + 1]));
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
		
		for (int i = 1; i < N-1; i++)
		{ 
			glm::vec3 back = controlPoints[i] - controlPoints[i - 1];
			glm::vec3 forward = controlPoints[i+1] - controlPoints[i];
			controlTangents.push_back(glm::normalize(glm::mix(back, forward, 0.5f)));
		}
		controlTangents.push_back(glm::normalize(controlPoints[N - 1] - controlPoints[N - 2]));
	}

	glm::vec3 evaluate(float s, size_t* i = nullptr) 
	{
		s = glm::clamp(s, 0.0f, cumulativeLengths.empty() ? 0.0f : cumulativeLengths.back());

		// Find segment (maybe implement binary search?)
		size_t seg = getSegmentAtLength(s);

		float segStart = (seg == 0 ? 0.0f : cumulativeLengths[seg - 1]);
		float t = (s - segStart) / segmentLengths[seg];

		if (i != nullptr)
		{
			*i = seg;
		}

		return glm::mix(controlPoints[seg], controlPoints[seg + 1], t);
	}

	glm::vec3 evaluateFrenet(float s)
	{
		//size_t i = 0;
		//

		//glm::vec3 position = nodePositions[i];
		//// Construct Frenet Frame
		//glm::vec3 forward = glm::normalize(nodeTangents[i]);

		//glm::mat3 rotation = glm::rotate(glm::radians((float)nodeRoll[i]), forward);
		//glm::vec3 right = rotation * glm::normalize(glm::cross(forward, UP_DIR));
		//glm::vec3 up = -glm::normalize(glm::cross(forward, right));
	}

	float normalizedToArcLength(float u)
	{
		if (cumulativeLengths.empty())
			return 0.0f;
		return u * cumulativeLengths.back();
	}

	float arcLengthToNormalized(float s)
	{
		if (cumulativeLengths.empty())
			return 0.0f;
		return s / cumulativeLengths.back();
	}

	size_t getSegmentAtLength(float s)
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

	glm::vec3 evaluateNormalized(float u)
	{
		return evaluate(normalizedToArcLength(u));
	}
};

struct Track 
{
	PiecewiseLinearCurve curve;

	std::vector<float> roll;

	void load(const std::string& path) 
	{
		YAML::Node config = YAML::LoadFile(path);

		curve = PiecewiseLinearCurve();
		curve.knots = config["knots"].as<std::vector<float>>();
		if (config["points"]) {
			for (const auto& point : config["points"]) {
				float x = point[0].as<float>();
				float y = point[1].as<float>();
				float z = point[2].as<float>();

				// Add the point to the vector
				curve.controlPoints.push_back(glm::vec3(x, y, z));
			}
		}
		roll = config["roll"].as<std::vector<float>>();
		update();
	}

	void save(const std::string& path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "knots" << YAML::Value << YAML::BeginSeq;
		for (const auto& k : curve.knots)
		{
			out << k;
		}
		out << YAML::EndSeq;
		out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
		for (const auto& p : curve.controlPoints)
		{
			out << YAML::Flow << YAML::BeginSeq << p.x << p.y << p.z << YAML::EndSeq;
		}
		out << YAML::EndSeq;
		out << YAML::Key << "roll" << YAML::Value << YAML::BeginSeq;
		for (const auto& r : roll)
		{
			out << r;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;


		std::ofstream fout(path);
		fout << out.c_str();
		fout.close();
	}

	void update()
	{
		curve.calculateLength();
		curve.calculateTangents();
	}
};

}