#include "camera.h"

using osp::Camera;

void Camera::updateProj(GLFWwindow* window, float deltaTime)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	proj = glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near, 60.0f);
	proj[1][1] *= -1;
}

void Camera::updateView(GLFWwindow* window, float deltaTime)
{
	position = glm::vec3{ cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw) };
	position *= distance;
	position += target;

	view = glm::lookAt(position, target, UP_DIR);
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
	double dx = xpos - this->lastX;
	double dy = ypos - this->lastY;
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
	this->updateView(window, 0.0f);
}