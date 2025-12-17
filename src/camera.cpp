#include "camera.h"

using osp::Camera;

void Camera::updateProj(GLFWwindow* window, float deltaTime)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if (viewMode == ViewMode::PERSPECTIVE) 
	{
		proj = glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near, 60.0f);
		proj[1][1] *= -1;
	}
	else if (viewMode == ViewMode::ORTHOGONAL)
	{
		float aspect = float(width) / float(height);
		float fovRad = glm::radians(fov);

		float d = distance;

		float halfHeight = d * tanf(fovRad * 0.5f);
		float halfWidth = halfHeight * aspect;

		proj = glm::ortho(
			-halfWidth, halfWidth,
			-halfHeight, halfHeight,
			-100.0f, 100.0f
		);

		proj[1][1] *= -1;
	}
}

void Camera::updateView(GLFWwindow* window, float deltaTime)
{
	position = glm::vec3{ cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw) };
	position *= distance;
	position += target;

	view = glm::lookAt(position, target, UP_DIR);
}

glm::vec2 Camera::projectPositionToScreen(glm::vec3 position, uint32_t width, uint32_t height)
{
	glm::vec4 clip = proj * view * glm::vec4(position, 1.0f);

	if (clip.w <= 0.0f)
		return glm::vec2(-1.0, -1.0);

	glm::vec3 ndc = glm::vec3(clip) / clip.w;

	if (ndc.z < 0.0f || ndc.z > 1.0f)
		return glm::vec2(-1.0, -1.0);

	return glm::vec2((ndc.x * 0.5f + 0.5f) * width, (ndc.y * 0.5f + 0.5f) * height);
}

float Camera::depthOfPoint(glm::vec3 position)
{
	float viewZ = (view * glm::vec4(position, 1.0)).z;
	return -viewZ; // positive distance
}

void Camera::toggleViewMode()
{
	this->viewMode = (viewMode == ViewMode::PERSPECTIVE) ? ViewMode::ORTHOGONAL : ViewMode::PERSPECTIVE;
	updateProj(window, 0.0);
}

void Camera::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		this->leftDown = (action == GLFW_PRESS);

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		this->rightDown = (action == GLFW_PRESS);

	glfwGetCursorPos(window, &this->lastX, &this->lastY);
}
void Camera::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	float dx = xpos - this->lastX;
	float dy = ypos - this->lastY;
	this->lastX = xpos;
	this->lastY = ypos;

	// Orbit rotation
	if (this->leftDown) {
		this->yaw -= (float)dx * rotateSpeed;
		this->pitch += (float)dy * rotateSpeed;

		if (this->pitch > this->maxPitch)  this->pitch = this->maxPitch;
		if (this->pitch < -this->maxPitch) this->pitch = -this->maxPitch;

		this->updateView(window, 0.0f);
	}

	// Pan on XZ plane (Y-up)
	if (this->rightDown) {
		glm::vec3 forward = -glm::normalize(glm::vec3{
			sinf(this->yaw), 0, cosf(this->yaw)
			});

		glm::vec3 right = glm::normalize(glm::cross(forward, UP_DIR));

		this->target -= right * (float)dx * panSpeed;
		this->target += forward * (float)dy * panSpeed;

		this->updateView(window, 0.0f);
	}
}
void Camera::onScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	this->distance -= (float)yoffset * 0.5f;
	if (this->distance < this->minDistance)
		this->distance = this->minDistance;
	if (viewMode == ViewMode::ORTHOGONAL)
		updateProj(window, 0.0);
	this->updateView(window, 0.0f);
	
}