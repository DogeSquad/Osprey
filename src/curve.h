#pragma once

#include <glm/glm.hpp>

namespace osp {

struct ICurve {
	virtual ~ICurve() = default;

	virtual glm::vec3 evaluate(float s, size_t* i = nullptr) = 0;
	virtual glm::mat4 evaluateFrenet(float s, const std::vector<float>& roll) = 0;
	virtual glm::mat4 evaluateFrenetInterpolated(float s, const std::vector<float>& roll) = 0;
	virtual size_t getSegmentAtLength(float s) = 0;

	virtual void extendBack() = 0;
	virtual void removeBack() = 0;

	virtual void update() = 0;

	virtual float totalLength() const = 0;
	virtual float normalizedToArcLength(float u) = 0;
	virtual float arcLengthToNormalized(float s) = 0;
};

} // namespace osp