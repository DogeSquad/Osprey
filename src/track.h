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
	struct TrackProfile {
		// Inspired by Vekoma Double Spine Track
		// https://themeparkreview.com/forum/uploads/monthly_2015_07/1072184159_IntaminDoubleSpineTrackDimensions.PNG.a2dad476ee138e4d324cbd482778a9d8.PNG
		// Converted from cm to meters
		const float runningRailRadius = 0.13f / 2.0f;
		const float railDistanceToCenter = 0.45f; // Running Rail Center to Center
		const float tieRadius = 0.05f;

		const float mainSplineRadius = 0.39f / 2.0f;
		const float mainSplineOffset = 0.4f;
	} const profile;

	struct TransportFrame {
		glm::vec3 right;
		glm::vec3 up;
		glm::vec3 forward;
		float s;
	};

	struct Node {
		// Idea for Later: Make Node more general as in a thing selectable and editable in 3d space. Idea for other node types
		// KineticNode: Controls custom movement on a track.
		// DesignNode: Control the track design, parameterised by position (s) on track.
		// StructureNode: Control support type.
		// TriggerNode: Can send triggers to the outer programming context.
		//
		// IMPORTANT: The additional nodes parameters should be "addressable" as in being able to be modified
		// Programming is an essential part to make custom movement idea possible.
		glm::vec3 position;
		float roll = 0.0f;
		float weight = 1.0f;

		Node(glm::vec3 _position, float _roll, float _weight) :
			position(_position),
			roll(_roll),
			weight(_weight){}
	};


	// TODO Handle insufficient number of control points
	std::unique_ptr<ICurve> curve;
	std::vector<Node>       nodes;

	std::vector<TransportFrame> transportFrames;

	Track() = default;

	void createEmpty()
	{
		std::unique_ptr<ICurve> tempCurve = std::make_unique<HermiteCurve>();
		nodes.clear();

		tempCurve->appendControlPoint(glm::vec3(0.0f));
		tempCurve->appendControlPoint(glm::vec3(1.0f, 0.0f, 0.0f));

		nodes.emplace_back(glm::vec3(0.0f), 0.0f, 1.0f);
		nodes.emplace_back(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f);

		curve = std::move(tempCurve);
		update();curve->update();
	}

	void load(const std::string& path) 
	{
		YAML::Node config = YAML::LoadFile(path);

		std::string curveType;
		std::unique_ptr<ICurve> tempCurve;

		nodes.clear();
		if (!config["curveType"] || (curveType = config["curveType"].as<std::string>()).compare("linear") == 0) {
			tempCurve = std::make_unique<PiecewiseLinearCurve>();
		}
		else if ((curveType = config["curveType"].as<std::string>()).compare("hermite") == 0){
			tempCurve = std::make_unique<HermiteCurve>();
		}
		
		if (config["points"] && config["roll"]) {
			auto points = config["points"].as<std::vector<std::vector<float>>>();
			auto rolls = config["roll"].as<std::vector<float>>();
			for (size_t i = 0; i < points.size(); i++) {
				float x = points[i][0];
				float y = points[i][1];
				float z = points[i][2];
		
				nodes.emplace_back(glm::vec3(x, y, z), rolls[i], 1.0f);
				// Add the point to the vector
				tempCurve->appendControlPoint(glm::vec3(x, y, z));
			}
		}


		//if (config["points"]) {
		//	for (const auto& point : config["points"]) {
		//		float x = point[0].as<float>();
		//		float y = point[1].as<float>();
		//		float z = point[2].as<float>();
		//
		//		// Add the point to the vector
		//		tempCurve->appendControlPoint(glm::vec3(x, y, z));
		//	}
		//}
		//if (config["roll"]) {
		//	auto rolls = config["roll"].as<std::vector<float>>();
		//}
		
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
		const size_t N = nodes.size();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "curveType" << YAML::Value << curveType;
		out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
		for (size_t i = 0; i < N; i++) {
			glm::vec3 p = nodes[i].position;
			out << YAML::Flow << YAML::BeginSeq << p.x << p.y << p.z << YAML::EndSeq;
		}
		out << YAML::EndSeq;
		out << YAML::Key << "roll" << YAML::Value << YAML::BeginSeq;
		for (size_t i = 0; i < N; i++) {
			out << nodes[i].roll;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;


		std::ofstream fout(path);
		fout << out.c_str();
		fout.close();
	}

	void applyModification(size_t i) 
	{
		curve->setControlPoint(i, nodes[i].position);
		curve->update();
	}

	void addNextSegment()
	{
		curve->extendBack();
		nodes.emplace_back(curve->getControlPoint(curve->getNumControlPoints() - 1), nodes[nodes.size()-1].roll, 1.0f);
		curve->update();
	}

	void removeLastSegment()
	{
		curve->removeBack();
		nodes.pop_back();
		curve->update();
	}

	glm::vec3 evaluatePosition(float s)
	{
		return curve->evaluate(s);
	}

	glm::mat4 evaluateFrenet(float s)
	{
		size_t seg;
		glm::vec3 pos = curve->evaluate(s, &seg);
		auto      frame = sampleTransportFrame(s);

		// find roll at this arc length by interpolating between nodes
		float  t = curve->normalizedInSegment(s);
		float  rollVal = glm::mix(nodes[seg].roll, nodes[seg+1].roll, t);

		// apply roll on top of transport frame
		glm::mat3 rollRot = glm::mat3(glm::rotate(glm::radians(rollVal), frame.forward));
		glm::vec3 right = glm::normalize(rollRot * frame.right);
		glm::vec3 up = glm::normalize(glm::cross(right, frame.forward));

		glm::mat4 result = glm::identity<glm::mat4>();
		result[0] = glm::vec4(right, 0);
		result[1] = glm::vec4(up, 0);
		result[2] = glm::vec4(frame.forward, 0);
		result[3] = glm::vec4(pos, 1);
		return result;
	}

	float totalLength()
	{
		return curve->totalLength();
	}

	void update()
	{
		curve->update();
		precomputeTransportFrames();
	}

	//
	// Frenet Frame calculations
	//
	float samplesPerMeter = 2.0f;

	void precomputeTransportFrames() {
		int numSamples = (int)(curve->totalLength() * samplesPerMeter + 0.5f);

		transportFrames.clear();
		transportFrames.reserve(numSamples);

		float total = totalLength();

		// first frame — bootstrap with world up
		float     s0 = 0.0f;
		glm::vec3 t0 = glm::normalize(curve->getTangentAtLength(s0));
		glm::vec3 r0 = glm::normalize(glm::cross(t0, glm::vec3(0, 1, 0)));
		glm::vec3 u0 = glm::cross(r0, t0);
		transportFrames.push_back({ r0, u0, t0, s0 });

		for (int i = 1; i < numSamples; i++) {
			float     s1 = (float)i / (numSamples - 1) * total;
			glm::vec3 t1 = glm::normalize(curve->getTangentAtLength(s1));

			auto& prev = transportFrames.back();

			// rotate previous frame to align with new tangent
			glm::vec3 axis = glm::cross(prev.forward, t1);
			float     len = glm::length(axis);

			TransportFrame frame;
			frame.s = s1;
			frame.forward = t1;

			if (len < 1e-6f) {
				frame.right = prev.right;
				frame.up = prev.up;
			}
			else {
				axis = axis / len;
				float dot = glm::clamp(glm::dot(prev.forward, t1), -1.0f, 1.0f);
				float angle = acos(dot);
				glm::mat3 rot = glm::mat3(glm::rotate(angle, axis));
				frame.right = glm::normalize(rot * prev.right);
				frame.up = glm::normalize(rot * prev.up);
			}
			transportFrames.push_back(frame);
		}
	}

	TransportFrame sampleTransportFrame(float s) {
		if (transportFrames.empty()) return {};
		s = glm::clamp(s, 0.0f, totalLength());

		// binary search for surrounding frames
		auto it = std::lower_bound(transportFrames.begin(), transportFrames.end(), s,
			[](const TransportFrame& f, float val) { return f.s < val; });

		if (it == transportFrames.begin()) return transportFrames.front();
		if (it == transportFrames.end())   return transportFrames.back();

		auto& f1 = *(it - 1);
		auto& f2 = *it;

		float t = (s - f1.s) / (f2.s - f1.s);

		// slerp the axes for smooth interpolation
		TransportFrame result;
		result.s = s;
		result.forward = glm::normalize(glm::mix(f1.forward, f2.forward, t));
		result.right = glm::normalize(glm::mix(f1.right, f2.right, t));
		result.up = glm::normalize(glm::mix(f1.up, f2.up, t));
		return result;
	}
};

} // namespace osp