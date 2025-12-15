#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "constants.h"

// Adapted from Vulkanizer https://github.com/milkru/vulkanizer

namespace osp 
{

struct Camera {
	enum ViewMode {
		PERSPECTIVE,
		ORTHOGONAL
	} viewMode = ViewMode::PERSPECTIVE;
	float fov = 45.f;
	float near = 0.01f;

	float rotateSpeed = 0.005f;
	float panSpeed = 0.005f;

	float pitch = 0.0f;
	float maxPitch = glm::radians(89.0f);
	float yaw = 0.0f;
	float distance = 5.0f;
	float minDistance = 0.1f;
	glm::vec3 position{};
	glm::vec3 target{ 0.0f, 0.0f, 0.0f };

	glm::mat4 view{};
	glm::mat4 proj{};

	bool leftDown = false;
	bool rightDown = false;
	double lastX = 0.0;
	double lastY = 0.0;

	GLFWwindow* window;
	Camera() = default;
	Camera(GLFWwindow* window) : window(window)
	{}

	void updateView(GLFWwindow* window, float deltaTime);
	void updateProj(GLFWwindow* window, float deltaTime);
	
	glm::vec2 projectPositionToScreen(glm::vec3 position, uint32_t width, uint32_t height);

	void toggleViewMode();

	void onMouseButton(GLFWwindow* window, int button, int action, int mods);
	void onCursor(GLFWwindow* window, double xpos, double ypos);
	void onScroll(GLFWwindow* window, double xoffset, double yoffset);
};

}