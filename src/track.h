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
	// TODO Handle insufficient number of control points
	std::unique_ptr<ICurve> curve;
	std::vector<float> roll;

	Track() = default;

	void createEmpty()
	{
		std::unique_ptr<ICurve> tempCurve = std::make_unique<HermiteCurve>();
		roll.clear();

		tempCurve->appendControlPoint(glm::vec3(0.0f));
		tempCurve->appendControlPoint(glm::vec3(1.0f, 0.0f, 0.0f));
		roll.push_back(0.0f);
		roll.push_back(0.0f);

		curve = std::move(tempCurve);
		curve->update();
	}

	void load(const std::string& path) 
	{
		YAML::Node config = YAML::LoadFile(path);

		std::string curveType;
		std::unique_ptr<ICurve> tempCurve;
		if (!config["curveType"] || (curveType = config["curveType"].as<std::string>()).compare("linear") == 0) {
			tempCurve = std::make_unique<PiecewiseLinearCurve>();
		}
		else if ((curveType = config["curveType"].as<std::string>()).compare("hermite") == 0){
			tempCurve = std::make_unique<HermiteCurve>();
		}
		
		if (config["points"]) {
			for (const auto& point : config["points"]) {
				float x = point[0].as<float>();
				float y = point[1].as<float>();
				float z = point[2].as<float>();
		
				// Add the point to the vector
				tempCurve->appendControlPoint(glm::vec3(x, y, z));
			}
		}
		roll = config["roll"].as<std::vector<float>>();
		
		curve = std::move(tempCurve);
		curve->update();
	}

	void save(const std::string& path)
	{
		std::string curveType = "linear";
		if (PiecewiseLinearCurve* tempCurve = dynamic_cast<PiecewiseLinearCurve*>(curve.get())) {
			curveType = "linear";
		} 
		else if (HermiteCurve* tempCurve = dynamic_cast<HermiteCurve*>(curve.get())) {
			curveType = "hermite";
		}
		else {
			curveType = "linear";
		}
		const size_t N = curve->getNumControlPoints();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "curveType" << YAML::Value << curveType;
		out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
		for (size_t i = 0; i < N; i++) {
			glm::vec3 p = curve->getControlPoint(i);
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

	glm::mat4 evaluateFrenet(float s)
	{
		return curve->evaluateFrenet(s, roll);
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