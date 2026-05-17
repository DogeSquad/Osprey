#pragma once
#include <vector>
#include <algorithm>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <yaml-cpp/yaml.h>

#include "mesh.h"
#include "constants.h"
//#include "piecewise_linear_curve.h"
//#include "hermite_curve.h"
#include "nurbs_curve.h"

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

	struct TrackNode {
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
		bool pinned = false;

		TrackNode() = default;
		TrackNode(glm::vec3 _position, float _roll, float _weight, float _pinned = false) :
			position(_position),
			roll(_roll),
			weight(_weight),
			pinned(_pinned){}
	};

	// TODO Handle insufficient number of control points
	std::unique_ptr<ICurve> curve;
	std::vector<TrackNode>  nodes;

    // Arc length lookup table
    std::vector<float> arcLengthTable;
    static const int ARC_LENGTH_SAMPLES = 1000;

    Track() = default;

    void createEmpty() {
        curve = std::make_unique<NURBSCurve>();
        nodes.clear();

        nodes.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(2.0f, 2.0f, 0.5f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(4.0f, 2.0f, 1.0f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(6.0f, 0.0f, 0.5f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(8.0f, 0.0f, 0.5f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(10.0f, 1.0f, 0.0f), 0.0f, 1.0f, false);
        nodes.emplace_back(glm::vec3(12.0f, 0.0f, 0.0f), 0.0f, 1.0f, false);

        //nodes.emplace_back(glm::vec3(0.0f), 0.0f, 1.0f, true);
        //nodes.emplace_back(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f, true);

        syncToMathCurve();
        update();
    }

    void load(const std::string& path) {
        YAML::Node config = YAML::LoadFile(path);

        std::string curveType = config["curveType"] ? config["curveType"].as<std::string>() : "linear";

        //if (curveType == "linear") {
        //    curve = std::make_unique<PiecewiseLinearCurve>();
        //}
        //else if (curveType == "hermite") {
        //    curve = std::make_unique<HermiteCurve>();
        //}
        if (curveType == "nurbs") {
            curve = std::make_unique<NURBSCurve>();
        }
        //else {
        //    curve = std::make_unique<PiecewiseLinearCurve>();
        //}

        nodes.clear();
        if (config["points"] && config["roll"]) {
            auto points = config["points"].as<std::vector<std::vector<float>>>();
            auto rolls = config["roll"].as<std::vector<float>>();

            std::vector<float> weights = config["weight"] ?
                config["weight"].as<std::vector<float>>() :
                std::vector<float>(points.size(), 1.0f);

            for (size_t i = 0; i < points.size(); i++) {
                glm::vec3 pos(points[i][0], points[i][1], points[i][2]);
                float roll = i < rolls.size() ? rolls[i] : 0.0f;
                float weight = i < weights.size() ? weights[i] : 1.0f;

                nodes.emplace_back(pos, roll, weight, false);
            }
        }

        syncToMathCurve();
        update();
    }

    void save(const std::string& path) {
        std::string curveType = "nurbs"; // Default
        //if (dynamic_cast<PiecewiseLinearCurve*>(curve.get())) {
        //    curveType = "linear";
        //}
        //else if (dynamic_cast<HermiteCurve*>(curve.get())) {
        //    curveType = "hermite";
        //}
        if (dynamic_cast<NURBSCurve*>(curve.get())) {
            curveType = "nurbs";
        }

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "curveType" << YAML::Value << curveType;

        out << YAML::Key << "points" << YAML::Value << YAML::BeginSeq;
        for (const auto& node : nodes) {
            out << YAML::Flow << YAML::BeginSeq << node.position.x << node.position.y << node.position.z << YAML::EndSeq;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "roll" << YAML::Value << YAML::BeginSeq;
        for (const auto& node : nodes) {
            out << node.roll;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "weight" << YAML::Value << YAML::BeginSeq;
        for (const auto& node : nodes) {
            out << node.weight;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(path);
        fout << out.c_str();
    }

    // Track evaluation methods using new architecture
    glm::vec3 evaluatePosition(float s) {
        float u = arcLengthToParameter(s);
        return curve->evaluate(u);
    }

    glm::mat4 evaluateFrenet(float s) {
        // Clamp s to valid range
        s = glm::clamp(s, 0.0f, totalLength());

        glm::vec3 position = evaluatePosition(s);
        glm::vec3 tangent = tangentAtArcLength(s);

        // Check for invalid position or tangent
        if (std::isnan(position.x) || std::isnan(position.y) || std::isnan(position.z) ||
            std::isnan(tangent.x) || std::isnan(tangent.y) || std::isnan(tangent.z)) {
            std::cerr << "ERROR: NaN in position or tangent at s=" << s << std::endl;
            return glm::identity<glm::mat4>();
        }

        float tangentLength = glm::length(tangent);
        if (tangentLength < 1e-6f) {
            std::cerr << "ERROR: Zero tangent at s=" << s << std::endl;
            return glm::identity<glm::mat4>();
        }

        glm::vec3 forward = tangent / tangentLength; // Normalize safely
        float rollAngle = interpolateRoll(s);

        if (std::isnan(rollAngle) || std::isinf(rollAngle)) {
            rollAngle = 0.0f; // Fallback
        }

        // Build frame with safety checks
        glm::vec3 worldUp = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::cross(forward, worldUp);
        float rightLength = glm::length(right);

        if (rightLength < 1e-6f) {
            // Forward is parallel to world up, use different reference
            worldUp = glm::vec3(1, 0, 0);
            right = glm::cross(forward, worldUp);
            rightLength = glm::length(right);
        }

        if (rightLength > 1e-6f) {
            right = right / rightLength;
        }
        else {
            right = glm::vec3(1, 0, 0); // Fallback
        }

        glm::vec3 up = glm::cross(right, forward);

        // Apply roll
        if (std::abs(rollAngle) > 1e-6f) {
            glm::mat3 rollRotation = glm::rotate(glm::radians(rollAngle), forward);
            right = rollRotation * right;
            up = rollRotation * up;
        }

        glm::mat4 result = glm::identity<glm::mat4>();
        result[0] = glm::vec4(right, 0);
        result[1] = glm::vec4(up, 0);
        result[2] = glm::vec4(forward, 0);
        result[3] = glm::vec4(position, 1);

        return result;
    }

    float totalLength() {
        return curve ? curve->length() : 0.0f;
    }

    void update() {
        if (!curve) return;
        curve->update();
        buildArcLengthTable();
    }

    // Node manipulation methods
    void applyModification(size_t i) {
        if (i >= nodes.size()) return;

        curve->setControlPoint(i, nodes[i].position);

        // Handle NURBS-specific properties
        if (auto* nurbs = dynamic_cast<INURBSCurve*>(curve.get())) {
            nurbs->setWeight(i, nodes[i].weight);
            nurbs->setPinned(i, nodes[i].pinned);
        }

        update();
    }

    void addNextSegment() {
        if (!curve) return;

        // Extend curve first
        if (nodes.size() >= 2) {
            glm::vec3 lastDir = nodes.back().position - nodes[nodes.size() - 2].position;
            glm::vec3 newPos = nodes.back().position + lastDir;
            nodes.emplace_back(newPos, nodes.back().roll, 1.0f, false);
        }
        else {
            nodes.emplace_back(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, 1.0f, false);
        }

        curve->appendControlPoint(nodes.back().position);
        update();
    }

    void removeLastSegment() {
        if (nodes.empty()) return;

        nodes.pop_back();
        if (curve && curve->getNumControlPoints() > 0) {
            curve->removeControlPoint(curve->getNumControlPoints() - 1);
        }
        update();
    }

    glm::vec3 tangentAtArcLength(float s) {
        float u = arcLengthToParameter(s);
        return curve->tangent(u);
    }

private:
    void syncToMathCurve() {
        if (!curve) return;

        // Clear curve and rebuild from nodes
        while (curve->getNumControlPoints() > 0) {
            curve->removeControlPoint(0);
        }

        for (const auto& node : nodes) {
            curve->appendControlPoint(node.position);
        }

        // Set NURBS-specific properties
        if (auto* nurbs = dynamic_cast<INURBSCurve*>(curve.get())) {
            for (size_t i = 0; i < nodes.size(); i++) {
                nurbs->setWeight(i, nodes[i].weight);
                nurbs->setPinned(i, nodes[i].pinned);
            }
        }
    }

    float interpolateRoll(float s) {
        if (nodes.size() < 2) return 0.0f;

        float totalLength = this->totalLength();
        if (totalLength <= 0) return 0.0f;

        // For NURBS: use parameter space instead of arc length for roll
        float u = arcLengthToParameter(s);

        // Map parameter to node segments
        float nodeFloat = u * (nodes.size() - 1);
        int nodeIndex = (int)nodeFloat;
        float localT = nodeFloat - nodeIndex;

        nodeIndex = glm::clamp(nodeIndex, 0, (int)nodes.size() - 2);

        return glm::mix(nodes[nodeIndex].roll, nodes[nodeIndex + 1].roll, localT);
    }

    void buildArcLengthTable() {
        if (!curve) return;

        arcLengthTable.clear();
        arcLengthTable.resize(ARC_LENGTH_SAMPLES + 1);

        float totalLength = 0.0f;
        glm::vec3 prevPoint = curve->evaluate(0.0f);
        arcLengthTable[0] = 0.0f;

        for (int i = 1; i <= ARC_LENGTH_SAMPLES; i++) {
            float u = (float)i / ARC_LENGTH_SAMPLES;
            glm::vec3 currentPoint = curve->evaluate(u);
            totalLength += glm::distance(prevPoint, currentPoint);
            arcLengthTable[i] = totalLength;
            prevPoint = currentPoint;
        }
    }

    float arcLengthToParameter(float s) {
        if (arcLengthTable.empty()) {
            std::cerr << "ERROR: Arc length table is empty!" << std::endl;
            return 0.0f;
        }

        float totalLength = arcLengthTable.back();
        if (totalLength <= 0.0f || std::isnan(totalLength)) {
            std::cerr << "ERROR: Invalid total length in arc table: " << totalLength << std::endl;
            return 0.0f;
        }

        s = glm::clamp(s, 0.0f, totalLength);

        auto it = std::lower_bound(arcLengthTable.begin(), arcLengthTable.end(), s);
        size_t index = it - arcLengthTable.begin();

        if (index == 0) return 0.0f;
        if (index >= arcLengthTable.size()) return 0.999999f; // Avoid u=1.0

        float s1 = arcLengthTable[index - 1];
        float s2 = arcLengthTable[index];

        if (std::abs(s2 - s1) < 1e-9f) {
            return (float)(index - 1) / ARC_LENGTH_SAMPLES;
        }

        float u1 = (float)(index - 1) / ARC_LENGTH_SAMPLES;
        float u2 = (float)index / ARC_LENGTH_SAMPLES;

        float t = (s - s1) / (s2 - s1);
        return glm::mix(u1, u2, t);
    }
};

} // namespace osp