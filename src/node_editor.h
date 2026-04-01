#pragma once

#include "track.h"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>

#include <string>

namespace osp {

struct NodeEditor {
	Camera* camera = nullptr;

	Track* track = nullptr;
	TrackMesh* trackMesh = nullptr;

	Track::Node* hovered = nullptr;
	float hoveringRadius = 70.0f;

	Track::Node* selected = nullptr;

	int screenSize[2] = { 0, 0 };

	// Lazily updated!!
	size_t selectedIndex = 0;
	size_t hoveredIndex = 0;
	
	bool* trackDirty;

	const char* selectedNodeWindowId = "###SelectedNodeWindow";

	void onCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (!track) return;
		glm::vec2 cursorPos(xpos, ypos);
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		size_t numNodes = track->nodes.size();
		for (size_t i = 0; i < numNodes; i++) {
			Track::Node& node = track->nodes[i];
			glm::vec3 controlPoint = node.position;
			glm::vec2 screenPos = camera->projectPositionToScreen(controlPoint, screenSize[0], screenSize[1]);
			if (screenPos.x == -1.0f || screenPos.y == -1.0f) {
				continue;
			}
			float scale = 1.0f / (1.0f + camera->depthOfPoint(controlPoint) * 0.1f);
			if (glm::distance2(cursorPos, screenPos) < scale * scale * hoveringRadius * hoveringRadius) {
				hovered = &node;
				hoveredIndex = i;
				return;
			}
		}
		hovered = nullptr;
	}

	void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) 
		{
			if (hovered) {
				selected = hovered;
				selectedIndex = hoveredIndex;
				hovered = nullptr;
			}
		}
	}

	void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			Track::Node* prevSelected = selected;
			if (track && !track->nodes.empty())
			{
				track->addNextSegment();
				if (prevSelected != nullptr) {
					selected = &track->nodes.back();
					selectedIndex = track->nodes.size() - 1;
				}
				*trackDirty = true;
			}
		}
		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
		{
			if (track && !track->nodes.empty())
			{
				size_t prevSelectedIndex = selectedIndex;
				track->removeLastSegment();
				if (prevSelectedIndex == track->nodes.size()) {
					selected = &track->nodes.back();
					selectedIndex = prevSelectedIndex - 1;
				}
				*trackDirty = true;
			}
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			if (selected) {
				selected = nullptr;
			}
		}
	}

	void update()
	{
		glfwGetWindowSize(camera->window, screenSize, screenSize + 1);
		if (!track) return;

		ImGui::Begin("Nodes");
		bool wasSelectedEnumerated = false;
		for (size_t i = 0; i < track->nodes.size(); i++) {
			Track::Node& node = track->nodes[i];

			ImDrawList* drawList = ImGui::GetForegroundDrawList();
			glm::vec3 controlPoint = node.position;
			glm::vec2 screenPos = camera->projectPositionToScreen(controlPoint, screenSize[0], screenSize[1]);

			if (screenPos.x != -1.0f && screenPos.y != -1.0f) {
				float scale = 1.0f / (1.0f + camera->depthOfPoint(controlPoint) * 0.1f);
				float alpha = (hovered == &node) ? 255 : 100;
				drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), scale * hoveringRadius, IM_COL32(230, 230, 230, alpha));
				if (selected == &node) {
					drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), scale * hoveringRadius, IM_COL32(0, 0, 0, 255), 0, scale * 13.0f);
				}
			}

			ImGuiSelectableFlags selectableFlags = (hovered == &node) ? ImGuiSelectableFlags_Highlight : 0;
			if (ImGui::Selectable(std::to_string(i).c_str(), selected == &node, selectableFlags)) {
				selected = &node;
				selectedIndex = i;
			}
			if (&node == selected) {
				wasSelectedEnumerated = true;
			}
		}
		if (!wasSelectedEnumerated) {
			selected = nullptr;
		}
		ImGui::End();

		if (selected) {
			ImGui::Begin(std::string("Node #" + std::to_string(selectedIndex) + selectedNodeWindowId).c_str());
			if (ImGui::DragFloat3("Position", glm::value_ptr(selected->position), 0.01f)
				|| ImGui::DragFloat("Roll", &selected->roll)) {
				track->applyModification(selectedIndex);
				*trackDirty = true;
			}
			ImGui::End();
		}
	}
};

} // namespace osp