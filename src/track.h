#pragma once
#include <vector>
#include <algorithm>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <yaml-cpp/yaml.h>

#include "mesh.h"
#include "constants.h"
#include "piecewise_linear_curve.h"
#include "hermite_curve.h"

namespace osp
{

struct Track 
{
	std::unique_ptr<ICurve> curve;
	std::vector<float> roll;

	void load(const std::string& path) 
	{

		//YAML::Node config = YAML::LoadFile(path);
		//
		//auto c = std::make_unique<HermiteCurve>();
		//
		//if (config["points"]) {
		//	for (const auto& point : config["points"]) {
		//		c->controlPoints.push_back({
		//			point[0].as<float>(),
		//			point[1].as<float>(),
		//			point[2].as<float>()
		//			});
		//	}
		//}
		//roll = config["roll"].as<std::vector<float>>();
		//curve = std::move(c);
		//curve->update();
		YAML::Node config = YAML::LoadFile(path);
		
		auto linCurve = std::make_unique<PiecewiseLinearCurve>();
		
		linCurve->knots = config["knots"].as<std::vector<float>>();
		if (config["points"]) {
			for (const auto& point : config["points"]) {
				float x = point[0].as<float>();
				float y = point[1].as<float>();
				float z = point[2].as<float>();
		
				// Add the point to the vector
				linCurve->controlPoints.push_back(glm::vec3(x, y, z));
			}
		}
		roll = config["roll"].as<std::vector<float>>();
		
		curve = std::move(linCurve);
		curve->update();
	}

	void save(const std::string& path)
	{
		PiecewiseLinearCurve* linCurve = dynamic_cast<PiecewiseLinearCurve*>(curve.get());
		if (!linCurve) return;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "knots" << YAML::Value << YAML::BeginSeq;
		for (const auto& k : linCurve->knots)
		{
			out << k;
		}
		out << YAML::EndSeq;
		out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
		for (const auto& p : linCurve->controlPoints)
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

	void addNextSegment()
	{
		curve->extendBack();
		roll.push_back(roll.back());
		curve->update();
	}

	void removeLastSegment()
	{
		curve->removeBack();
		roll.pop_back();
		curve->update();
	}

	glm::vec3 evaluatePosition(float s)
	{
		return curve->evaluate(s);
	}

	glm::mat4 evaluateFrenetInterpolated(float s)
	{
		return curve->evaluateFrenetInterpolated(s, roll);
	}

	float totalLength()
	{
		return curve->totalLength();
	}

	void update()
	{
		curve->update();
	}
};

}