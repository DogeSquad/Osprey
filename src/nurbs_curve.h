#pragma once

#include "curve.h"

namespace osp {

    struct INURBSCurve : public ICurve {
        virtual float getWeight(size_t i) const = 0;
        virtual void setWeight(size_t i, float weight) = 0;
        virtual bool getPinned(size_t i) const = 0;
        virtual void setPinned(size_t i, bool pinned) = 0;
    };

    struct NURBSCurve : public INURBSCurve {
        std::vector<glm::vec3> controlPoints;
        std::vector<float> weights;
        std::vector<bool> pinned;
        std::vector<float> knots;
        int degree = 3;

        mutable float cachedLength = -1.0f;

        NURBSCurve() = default;

        glm::vec3 evaluate(float u) override {
            if (controlPoints.empty()) return glm::vec3(0.0f);

            u = glm::clamp(u, 0.0f, 1.0f);

            // Handle exact boundary case
            if (u >= 1.0f) {
                return controlPoints.back();
            }

            glm::vec3 numerator(0.0f);
            float denominator = 0.0f;

            for (int i = 0; i < controlPoints.size(); i++) {
                float basis = basisFunction(i, degree, u);
                if (std::isnan(basis) || std::isinf(basis)) {
                    std::cerr << "Invalid basis function at i=" << i << " u=" << u << std::endl;
                    continue;
                }

                float weight = weights[i];
                float weightedBasis = basis * weight;

                numerator += weightedBasis * controlPoints[i];
                denominator += weightedBasis;
            }

            if (denominator < 1e-7f) {
                return controlPoints[0];
            }

            glm::vec3 result = numerator / denominator;

            // Check for NaN in result
            if (std::isnan(result.x) || std::isnan(result.y) || std::isnan(result.z)) {
                std::cerr << "NaN in evaluate result at u=" << u << std::endl;
                return controlPoints[0];
            }

            return result;
        }

        glm::vec3 tangent(float u) override {
            const float eps = 0.001f;
            glm::vec3 p1 = evaluate(glm::clamp(u - eps, 0.0f, 1.0f));
            glm::vec3 p2 = evaluate(glm::clamp(u + eps, 0.0f, 1.0f));

            glm::vec3 tangentVec = p2 - p1;
            float tangentLength = glm::length(tangentVec);

            if (tangentLength > 1e-6f) {
                return tangentVec / tangentLength;
            }

            // Fallback for degenerate cases
            return glm::vec3(0, 0, 1);
        }

        float length() const override {
            if (cachedLength < 0) {
                cachedLength = calculateLength();
            }
            return cachedLength;
        }

        void update() override {
            cachedLength = -1.0f;

            // FIX: Don't reset existing data, just resize
            if (weights.size() != controlPoints.size()) {
                weights.resize(controlPoints.size(), 1.0f);
            }
            if (pinned.size() != controlPoints.size()) {
                pinned.resize(controlPoints.size(), false); // Only add new elements as false
            }

            generateKnots();
        }

        // Control point interface
        size_t getNumControlPoints() const override {
            return controlPoints.size();
        }

        glm::vec3 getControlPoint(size_t i) const override {
            return i < controlPoints.size() ? controlPoints[i] : glm::vec3(0.0f);
        }

        void setControlPoint(size_t i, glm::vec3 value) override {
            if (i >= controlPoints.size()) return;
            controlPoints[i] = value;
            cachedLength = -1.0f;
        }

        void appendControlPoint() override {
            glm::vec3 newControlPoint(1.0f, 0.0f, 0.0f); // Default position
            if (controlPoints.size() >= 2) {
                glm::vec3 forward = controlPoints.back() - controlPoints[controlPoints.size() - 2];
                newControlPoint = controlPoints.back() + forward;
            }
            appendControlPoint(newControlPoint);
        }

        void appendControlPoint(glm::vec3 value) override {
            controlPoints.push_back(value);
            weights.push_back(1.0f);
            pinned.push_back(false);
            cachedLength = -1.0f;
        }

        void removeControlPoint() override {
            if (!controlPoints.empty()) {
                removeControlPoint(controlPoints.size() - 1);
            }
        }

        void removeControlPoint(size_t i) override {
            if (i >= controlPoints.size()) return;

            controlPoints.erase(controlPoints.begin() + i);
            if (i < weights.size()) weights.erase(weights.begin() + i);
            if (i < pinned.size()) pinned.erase(pinned.begin() + i);
            cachedLength = -1.0f;
        }

        // NURBS-specific interface
        float getWeight(size_t i) const override {
            return i < weights.size() ? weights[i] : 1.0f;
        }

        void setWeight(size_t i, float weight) override {
            if (i < weights.size()) {
                weights[i] = weight;
                cachedLength = -1.0f;
            }
        }

        bool getPinned(size_t i) const override {
            return i < pinned.size() ? pinned[i] : false;
        }

        void setPinned(size_t i, bool isPinned) override {
            if (i < pinned.size()) {
                pinned[i] = isPinned;
                generateKnots();
                cachedLength = -1.0f;
            }
        }

    private:
        void generateKnots() {
            if (controlPoints.empty()) return;


            std::vector<float> parameterValues = { 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f };
            knots = parameterValues;
            //int n = controlPoints.size();
            //knots.clear();

            //// Force linear for 2 points
            //if (n <= 2) {
            //    degree = 1;
            //    knots = { 0.0f, 0.0f, 1.0f, 1.0f };
            //    return;
            //}

            //// Determine degree
            //degree = std::min(3, n - 1);
            //if (degree < 1) degree = 1;

            //// FIXED: Implement actual pinning
            //std::vector<float> parameterValues;

            //// Always pin first and last points (clamped curve)
            //for (int rep = 0; rep <= degree; rep++) {
            //    parameterValues.push_back(0.0f);
            //}

            //// Handle interior points
            //int numInterior = n - 2; // Exclude first and last
            //for (int i = 1; i <= numInterior; i++) {
            //    float t = (float)i / (numInterior + 1);

            //    // Check if this control point should be pinned
            //    if (i < pinned.size() && pinned[i]) {
            //        // Pinned: repeat the knot 'degree' times
            //        for (int rep = 0; rep < degree; rep++) {
            //            parameterValues.push_back(t);
            //        }
            //    }
            //    else {
            //        // Not pinned: single knot
            //        parameterValues.push_back(t);
            //    }
            //}

            //// Always pin last point
            //for (int rep = 0; rep <= degree; rep++) {
            //    parameterValues.push_back(1.0f);
            //}

            //knots = parameterValues;
        }



        float basisFunction(int i, int p, float u) const {
            // Bounds checking
            if (i < 0 || i >= knots.size() - p - 1) {
                return 0.0f;
            }

            if (p == 0) {
                // Handle boundary case for u = 1.0
                if (u == 1.0f && i == knots.size() - p - 2) {
                    return 1.0f;
                }
                return (u >= knots[i] && u < knots[i + 1]) ? 1.0f : 0.0f;
            }

            float left = 0.0f, right = 0.0f;

            // Check bounds for recursive calls
            if (i + p < knots.size()) {
                float denom1 = knots[i + p] - knots[i];
                if (denom1 > 1e-7f) {
                    left = (u - knots[i]) / denom1 * basisFunction(i, p - 1, u);
                }
            }

            if (i + p + 1 < knots.size()) {
                float denom2 = knots[i + p + 1] - knots[i + 1];
                if (denom2 > 1e-7f) {
                    right = (knots[i + p + 1] - u) / denom2 * basisFunction(i + 1, p - 1, u);
                }
            }

            return left + right;
        }

        float calculateLength() const {
            const int samples = 200;
            float totalLength = 0.0f;
            glm::vec3 prevPoint = const_cast<NURBSCurve*>(this)->evaluate(0.0f);

            for (int i = 1; i <= samples; i++) {
                float u = (float)i / samples;
                glm::vec3 currentPoint = const_cast<NURBSCurve*>(this)->evaluate(u);
                float segLength = glm::distance(prevPoint, currentPoint);

                if (std::isnan(segLength) || std::isinf(segLength)) {
                    std::cerr << "Invalid segment length at u=" << u << std::endl;
                    continue;
                }

                totalLength += segLength;
                prevPoint = currentPoint;
            }
            return totalLength;
        }
    };
} // namespace osp
