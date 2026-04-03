#pragma once

#include "curve.h"

namespace osp {

    struct NURBSCurve : public ICurve {
        std::vector<glm::vec3> controlPoints;
        std::vector<float> weights;
        std::vector<bool> pinned;
        std::vector<float> knots;
        std::vector<float> segmentLengths;
        std::vector<float> cumulativeLengths;
        int degree = 1;

        NURBSCurve() = default;

        void generateKnots() {
            if (controlPoints.empty()) return;

            int n = controlPoints.size();
            knots.clear();

            // Simple uniform clamped knot vector
            // For n control points and degree p: m = n + p + 1 knots
            int m = n + degree + 1;
            knots.resize(m);

            // Clamped: first (degree+1) knots are 0, last (degree+1) knots are 1
            for (int i = 0; i <= degree; i++) {
                knots[i] = 0.0f;
                knots[m - 1 - i] = 1.0f;
            }

            // Interior knots uniformly spaced
            for (int i = degree + 1; i < n; i++) {
                knots[i] = (float)(i - degree) / (n - degree);
            }
        }

        float basisFunction(int i, int p, float u) const {
            if (p == 0) {
                return (u >= knots[i] && u < knots[i + 1]) ? 1.0f : 0.0f;
            }

            float left = 0.0f, right = 0.0f;

            float denom1 = knots[i + p] - knots[i];
            if (denom1 > 1e-7f) {
                left = (u - knots[i]) / denom1 * basisFunction(i, p - 1, u);
            }

            float denom2 = knots[i + p + 1] - knots[i + 1];
            if (denom2 > 1e-7f) {
                right = (knots[i + p + 1] - u) / denom2 * basisFunction(i + 1, p - 1, u);
            }

            return left + right;
        }

        glm::vec3 evaluateNormalized(float u) {
            if (controlPoints.empty()) return glm::vec3(0.0f);

            // Clamp parameter
            u = glm::clamp(u, 0.0f, 1.0f);
            if (u == 1.0f) u = 0.999999f; // Avoid boundary issues

            glm::vec3 numerator(0.0f);
            float denominator = 0.0f;

            // NURBS formula: sum(Ni,p(u) * wi * Pi) / sum(Ni,p(u) * wi)
            for (int i = 0; i < controlPoints.size(); i++) {
                float basis = basisFunction(i, degree, u);
                float weight = (i < weights.size()) ? weights[i] : 1.0f;
                float weighted_basis = basis * weight;

                numerator += weighted_basis * controlPoints[i];
                denominator += weighted_basis;
            }

            return (denominator > 1e-7f) ? numerator / denominator : controlPoints[0];
        }

        void calculateLength() {
            if (controlPoints.size() <= 1) {
                segmentLengths.clear();
                cumulativeLengths.clear();
                return;
            }

            // Calculate length by uniform sampling
            const int samples = 100;
            float totalLength = 0.0f;

            segmentLengths.clear();
            cumulativeLengths.clear();

            // Create segments matching control point count - 1
            size_t numSegments = controlPoints.size() - 1;
            segmentLengths.resize(numSegments, 0.0f);

            glm::vec3 prevPoint = evaluateNormalized(0.0f);

            for (int i = 1; i <= samples; i++) {
                float u = (float)i / samples;
                glm::vec3 currentPoint = evaluateNormalized(u);
                float segmentLength = glm::distance(prevPoint, currentPoint);

                // Distribute length to appropriate segment
                int segIndex = std::min((int)((u * numSegments)), (int)numSegments - 1);
                segmentLengths[segIndex] += segmentLength;

                totalLength += segmentLength;
                prevPoint = currentPoint;
            }

            // Build cumulative lengths
            cumulativeLengths.resize(numSegments);
            float cumulative = 0.0f;
            for (size_t i = 0; i < numSegments; i++) {
                cumulative += segmentLengths[i];
                cumulativeLengths[i] = cumulative;
            }
        }

        void update() override {
            if (controlPoints.empty()) return;

            // Auto-adjust degree
            degree = std::min(3, (int)controlPoints.size() - 1);
            if (degree < 1) degree = 1;

            // Ensure arrays are properly sized
            weights.resize(controlPoints.size(), 1.0f);
            pinned.resize(controlPoints.size(), false);

            generateKnots();
            calculateLength();
        }

        // ICurve interface
        float totalLength() const override {
            return cumulativeLengths.empty() ? 0.0f : cumulativeLengths.back();
        }

        glm::vec3 evaluate(float s, size_t* segmentIndex = nullptr) override {
            if (cumulativeLengths.empty()) return glm::vec3(0.0f);

            s = glm::clamp(s, 0.0f, cumulativeLengths.back());
            float u = s / cumulativeLengths.back();

            if (segmentIndex) {
                *segmentIndex = getSegmentAtLength(s);
            }

            return evaluateNormalized(u);
        }

        size_t getSegmentAtLength(float s) override {
            if (cumulativeLengths.empty()) return 0;

            s = glm::clamp(s, 0.0f, cumulativeLengths.back());

            for (size_t i = 0; i < cumulativeLengths.size(); i++) {
                if (s <= cumulativeLengths[i]) {
                    return i;
                }
            }
            return cumulativeLengths.size() - 1;
        }

        glm::vec3 getTangentAtLength(float s) override {
            float u = s / (cumulativeLengths.empty() ? 1.0f : cumulativeLengths.back());
            const float eps = 0.001f;

            glm::vec3 p1 = evaluateNormalized(glm::clamp(u - eps, 0.0f, 1.0f));
            glm::vec3 p2 = evaluateNormalized(glm::clamp(u + eps, 0.0f, 1.0f));

            glm::vec3 tangent = p2 - p1;
            return glm::length(tangent) > 1e-6f ? glm::normalize(tangent) : glm::vec3(0, 0, 1);
        }

        float normalizedToArcLength(float u) override {
            return u * (cumulativeLengths.empty() ? 0.0f : cumulativeLengths.back());
        }

        float arcLengthToNormalized(float s) override {
            return cumulativeLengths.empty() ? 0.0f : s / cumulativeLengths.back();
        }

        float normalizedInSegment(float s) override {
            size_t seg = getSegmentAtLength(s);
            if (seg >= segmentLengths.size()) return 0.0f;

            float segStart = (seg == 0) ? 0.0f : cumulativeLengths[seg - 1];
            float segLength = segmentLengths[seg];

            return (segLength > 1e-6f) ? (s - segStart) / segLength : 0.0f;
        }

        // Control point interface
        glm::vec3 getControlPoint(size_t i) override {
            return (i < controlPoints.size()) ? controlPoints[i] : glm::vec3(0.0f);
        }

        size_t getNumControlPoints() override {
            return controlPoints.size();
        }

        void setControlPoint(size_t i, glm::vec3 value) override {
            if (i < controlPoints.size()) {
                controlPoints[i] = value;
            }
        }

        void appendControlPoint(glm::vec3 value) override {
            controlPoints.push_back(value);
            weights.push_back(1.0f);
            pinned.push_back(false);
        }

        void extendBack() override {
            if (controlPoints.size() >= 2) {
                glm::vec3 lastDir = controlPoints.back() - controlPoints[controlPoints.size() - 2];
                appendControlPoint(controlPoints.back() + lastDir);
            }
            else {
                appendControlPoint(glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        void removeBack() override {
            if (!controlPoints.empty()) {
                controlPoints.pop_back();
                if (!weights.empty()) weights.pop_back();
                if (!pinned.empty()) pinned.pop_back();
            }
        }

        // Weight interface
        float getWeight(int i) const override {
            return (i >= 0 && i < weights.size()) ? weights[i] : 1.0f;
        }

        void setWeight(int i, float w) override {
            if (i >= 0 && i < weights.size()) {
                weights[i] = w;
            }
        }

        // Simplified pinning (for now just stores the value)
        void setPinned(size_t i, bool isPinned) {
            if (i < pinned.size()) {
                pinned[i] = isPinned;
            }
        }

        bool getPinned(size_t i) const {
            return (i < pinned.size()) ? pinned[i] : false;
        }

        // Not implemented in this clean version
        glm::mat4 evaluateFrenet(float s, const std::vector<float>& roll) override {
            return glm::identity<glm::mat4>();
        }
    };
} // namespace osp