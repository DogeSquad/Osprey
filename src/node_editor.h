#pragma once

#include "track.h"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>

#include <string>

namespace osp {

struct NodeEditor {
	Track* track = nullptr;
	TrackMesh* trackMesh = nullptr;

	Track::Node* hovered = nullptr;
	Track::Node* selected = nullptr;

	// Lazily updated!!
	size_t selectedIndex = 0;
	
	bool* trackDirty;

	const char* selectedNodeWindowId = "###SelectedNodeWindow";

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
	}

	void update()
	{
		if (!track) return;

		ImGui::Begin("Nodes");
		bool wasSelectedEnumerated = false;
		for (size_t i = 0; i < track->nodes.size(); i++) {
			Track::Node& node = track->nodes[i];
			if (ImGui::Selectable(std::to_string(i).c_str(), selected == &node)) {
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