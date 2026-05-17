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

	// Math interface
	virtual glm::vec3 evaluate(float u) = 0;
	virtual glm::vec3 tangent(float u) = 0;
	virtual float length() const = 0;

	// Editing interface
	virtual size_t getNumControlPoints() const = 0;
	virtual glm::vec3 getControlPoint(size_t i) const = 0;
	virtual void setControlPoint(size_t i, glm::vec3 value) = 0;
	virtual void appendControlPoint() = 0;
	virtual void appendControlPoint(glm::vec3 value) = 0;
	virtual void removeControlPoint() = 0;
	virtual void removeControlPoint(size_t i) = 0;

	virtual void update() = 0;
};

} // namespace osp