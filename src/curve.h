#pragma once

#include <glm/glm.hpp>

namespace osp {

static void setColumn(glm::mat4& mat, glm::vec3 colVec, size_t index)
{
	if (index < 0 || index > 3) return;

	mat[index][0] = colVec[0];
	mat[index][1] = colVec[1];
	mat[index][2] = colVec[2];
}

struct ICurve {
	virtual ~ICurve() = default;

	// --- math interface ---
	virtual glm::vec3 evaluate(float s, size_t* i = nullptr) = 0;
	virtual glm::mat4 evaluateFrenet(float s, const std::vector<float>& roll) = 0;
	virtual size_t getSegmentAtLength(float s) = 0;
	virtual glm::vec3 getTangentAtLength(float s) = 0;
	virtual float totalLength() const = 0;
	virtual void update() = 0;

	virtual float normalizedToArcLength(float u) = 0;
	virtual float arcLengthToNormalized(float s) = 0;
	virtual float normalizedInSegment(float s) = 0;

	// --- control point interface ---
	virtual glm::vec3 getControlPoint(size_t i) = 0;
	virtual size_t getNumControlPoints() = 0;
	virtual void setControlPoint(size_t i, glm::vec3 value) = 0;
	virtual void appendControlPoint(glm::vec3 value) = 0;
	virtual void extendBack() = 0;
	virtual void removeBack() = 0;

	// --- per-point weight (default 1.0 for non-NURBS) ---
	virtual float     getWeight(int i) const { return 1.0f; }
	virtual void      setWeight(int i, float w) {}
};

} // namespace osp