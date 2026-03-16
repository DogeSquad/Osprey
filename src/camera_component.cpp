#include "camera_component.h"

#include "entity.h"
#include "transform_component.h"

namespace osp {

void CameraComponent::SetPerspective(float fov, float aspect, float near, float far)
{
	fieldOfView = fov;
	aspectRatio = aspect;
	nearPlane = near;
	farPlane = far;
	projectionDirty = true;
}
glm::mat4 CameraComponent::GetViewMatrix() const
{
	auto transform = GetOwner()->GetComponent<TransformComponent>();
	if (!transform) {
		return glm::mat4(1.0f);
	}

	glm::vec3 position = transform->GetPosition();
	glm::quat rotation = transform->GetRotation();

	glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

	return glm::lookAt(position, position + forward, up);
}
glm::mat4 CameraComponent::GetProjectionMatrix() const
{
	if (projectionDirty) {
		projectionMatrix = glm::perspective(
			glm::radians(fieldOfView),
			aspectRatio,
			nearPlane,
			farPlane
		);
		projectionDirty = false;
	}
	return projectionMatrix;
}

} // namespace osp
