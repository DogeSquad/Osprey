#include "transform_component.h"

namespace osp {
	glm::mat4 TransformComponent::GetTransformaMatrix()
	{
		if (transformDirty) {
			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

			transformMatrix = translationMatrix * rotationMatrix * scaleMatrix;
			transformDirty = false;
		}
		return transformMatrix;
	}
} // namespace osp;