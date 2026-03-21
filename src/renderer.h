#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <thread>

// File Loading
#include <nfd.h>
#include <yaml-cpp/yaml.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#	include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "constants.h"
#include "camera.h"
#include "track.h"
#include "track_mesh.h"
#include "vk_context.h"
#include "swapchain.h"
#include "image.h"
#include "memory_utils.h"
#include "render_attachments.h"
#include "pipeline.h"
#include "frame.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int      MAX_FRAMES_IN_FLIGHT = 2;

class OspreyApp
{
public:
	void run()
	{
		NFD_Init();
		initWindow();
		initVulkan();
		initImGUI();
		initWorld();
		mainLoop();
		cleanup();
		NFD_Quit();
	}

	osp::Camera* getCamera()
	{
		return &camera;
	}

private:
	GLFWwindow* window = nullptr;
	osp::VkContext context;
	osp::Swapchain swapChain;
	bool swapChainRebuild = true;

	vk::raii::DescriptorPool imguiPool = nullptr;
	
	osp::RenderAttachments renderAttachments;
	osp::Pipeline mainPipeline;
	osp::Pipeline backgroundPipeline;

	vk::raii::DescriptorPool             descriptorPool = nullptr;
	vk::raii::CommandPool                commandPool = nullptr;
	std::array<osp::Frame, MAX_FRAMES_IN_FLIGHT> frames;
	uint32_t                         currentFrame = 0;

	bool framebufferResized = false;

	ImGui_ImplVulkanH_Window g_MainWindowData;
	uint32_t                 g_MinImageCount = 2;
	bool                     g_SwapChainRebuild = false;

	glm::vec2 screenCursorPos = { 0.0f, 0.0f };
	uint32_t lastHoveredControlPointIndex = 0;
	bool isShift;

	osp::Camera camera;
	osp::Track track;
	std::unique_ptr<osp::TrackMesh> trackMesh;
	std::unique_ptr<osp::Mesh> groundGridMesh;

	std::string currentTrackFilePath = "F:\\Dev\\_VulkanProjects\\Osprey\\tracks\\coolCircuit.yaml";
	std::string currentTrackFileName = "coolCircuit.yaml";

	bool showAbout = false;

	// PHYSICS
	float dt = 0.016666;
	float u = 0.0f;
	float s = 0.0f;
	float v = 0.0f;

	bool doSimulate = true;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = static_cast<OspreyApp*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (ImGui::GetIO().WantCaptureMouse) {
			ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
			return;
		}

		auto app = static_cast<OspreyApp*>(glfwGetWindowUserPointer(window));
		if (app->showAbout) return;

		app->getCamera()->onMouseButton(window, button, action, mods);
	}
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

		auto app = static_cast<OspreyApp*>(glfwGetWindowUserPointer(window));
		app->getCamera()->onCursor(window, xpos, ypos);

		app->screenCursorPos[0] = xpos;
		app->screenCursorPos[1] = ypos;
	}
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		if (ImGui::GetIO().WantCaptureMouse) 
		{
			ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
			//return;
		}
		auto app = static_cast<OspreyApp*>(glfwGetWindowUserPointer(window));
		if (app->showAbout) return;

		if (app->lastHoveredControlPointIndex != -1)
		{
			float unclampedRoll = app->track.roll[app->lastHoveredControlPointIndex] + (float)yoffset * 1.5f;
			app->track.roll[app->lastHoveredControlPointIndex] = unclampedRoll;
			if (unclampedRoll > 180.0f) {
				app->track.roll[app->lastHoveredControlPointIndex] -= 360.0f;
			}
			else if (unclampedRoll < -180.0f) {
				app->track.roll[app->lastHoveredControlPointIndex] += 360.0f;
			}

			app->track.update();
			app->createTrack();
			return;
		}

		app->getCamera()->onScroll(window, xoffset, yoffset);
	}
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = static_cast<OspreyApp*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) 
		{
			if (app->showAbout)
			{
				app->showAbout = false;
			}
			else
			{
				glfwSetWindowShouldClose(window, true);
			}
		}
		if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
		{
			if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
			{
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
				glfwRestoreWindow(window);
			}
			else
			{
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
				glfwMaximizeWindow(window);
			}
		}
		if (app->showAbout) return;


		if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
		{
			app->loadTrack(app->currentTrackFilePath);
		}
		if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
		{
			app->camera.toggleViewMode();
		}
		if (key == GLFW_KEY_LEFT_SHIFT)
		{
			if (action == GLFW_PRESS)
				app->isShift = true;
			if (action == GLFW_RELEASE)
				app->isShift = false;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			if (!app->track.roll.empty())
			{
				app->track.addNextSegment();
				app->createTrack();
			}
		}
		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
		{
			if (!app->track.roll.empty())
			{
				app->track.removeLastSegment();
				app->createTrack();
			}
		}
	}

	void initVulkan()
	{
		context.initContext(window);
		swapChain = osp::Swapchain(context, *context.surface, window);
		mainPipeline = osp::Pipeline(context, swapChain.surfaceFormat.format, osp::findDepthFormat(context), {
			.shaderPath = "shaders/main_shader.spv",
			.polygonMode = vk::PolygonMode::eLine}
		);
		backgroundPipeline = osp::Pipeline(context, swapChain.surfaceFormat.format, osp::findDepthFormat(context), {
			.shaderPath = "shaders/horizon_gradient.spv",
			.polygonMode = vk::PolygonMode::eFill,
			.hasVertexInput = false,
			.depthTest = false,
			.depthWrite = false }
		);
		createCommandPool();
		renderAttachments = osp::RenderAttachments(context, swapChain.extent, swapChain.surfaceFormat.format);
		//createTrack();
		createGroundGrid();
		createDescriptorPool();
		createFrameResources();
	}

	void initImGUI()
	{

		vk::DescriptorPoolSize poolSizes[] =
		{
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo poolInfo = {
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = 1000,
			.poolSizeCount = std::size(poolSizes),
			.pPoolSizes = poolSizes };

		imguiPool = context.device.createDescriptorPool(poolInfo, nullptr);

		// 2: initialize imgui library

		//this initializes the core structures of imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(window, false);

		VkFormat colorFormat = static_cast<VkFormat>(swapChain.surfaceFormat.format);

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info{
			.Instance = static_cast<VkInstance>(*context.instance),
			.PhysicalDevice = static_cast<VkPhysicalDevice>(*context.physicalDevice),
			.Device = static_cast<VkDevice>(*context.device),
			.Queue = static_cast<VkQueue>(*context.queue),
			.DescriptorPool = static_cast<VkDescriptorPool>(*imguiPool),
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			.UseDynamicRendering = true,
			.PipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &colorFormat
			}
		};
		ImGui_ImplVulkan_Init(&init_info);

		////execute a gpu command to upload imgui font textures
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	void loadTrack(std::string filePath)
	{
		if (filePath.empty())
		{
			return;
		}
		if (currentTrackFilePath != filePath)
		{
			currentTrackFilePath = filePath; // TODO: Assumes that it is valid
			currentTrackFileName = std::filesystem::path(filePath).filename().string();
		}
		//device.waitIdle();
		track = osp::Track();
		track.load(std::string(filePath));
		createTrack();

		u = 0.0f;
		s = 0.0f;
		v = 0.001f;
	}

	void saveTrack(std::string filePath)
	{
		if (filePath.empty())
		{
			return;
		}
		track.save(filePath);
	}

	void initWorld() 
	{
		camera = osp::Camera(window);

		glfwSetCursorPosCallback(window, cursorPosCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
		glfwSetScrollCallback(window, scrollCallback);
		glfwSetKeyCallback(window, keyCallback);

		camera.updateProj(window, 0.0f);
		camera.updateView(window, 0.0f);
	}

	void doPhysics()
	{
		if (track.curve.cumulativeLengths.empty())
			return;
		float remainingTime = dt;
		float dtSub = 0.001f; // max substep

		while (remainingTime > 0) {
			float step = std::min(dtSub, remainingTime);

			glm::vec3 pos = track.curve.evaluate(s);
			int segIndex = track.curve.getSegmentAtLength(s);

			// Clamp end
			if (segIndex >= track.curve.controlPoints.size() - 1)
			{
				v = 0;
				return;
			}

			float segLen = track.curve.segmentLengths[segIndex];

			glm::vec3 p0 = track.curve.controlPoints[segIndex];
			glm::vec3 p1 = track.curve.controlPoints[segIndex + 1];
			glm::vec3 tangent = glm::normalize(p1 - p0);

			float a = 0.001f * 9.81f * glm::dot(GRAVITY, tangent);
			float rollingFriction = 0.01;
			float frictionAccel = -rollingFriction * 0.001f * 9.81f * (v > 0 ? 1.0f : -1.0f);
			a += frictionAccel;
			// Euler integration
			s += v * step + 0.5f * a * step * step;
			v += a * step;

			if (segIndex < 12) 
			{
				v = 0.01f;
			}

			// Clamp s to track
			if (s < 0) { s = 0; v = 0; }
			if (s > track.curve.cumulativeLengths.back()) { s = track.curve.cumulativeLengths.back(); v = 0; }

			remainingTime -= step;
		}
	}

	void doPhysics2()
	{
		const float eps = 1e-5f;
		if (track.curve.cumulativeLengths.empty())
			return;
		float remainingTime = dt;

		while (remainingTime > 0.0f)
		{
			// Clamp s inside track
			s = glm::clamp(s, 0.0f, track.curve.cumulativeLengths.back());

			// ----------- Locate current segment -----------
			float dist = s;
			int segIndex = track.curve.getSegmentAtLength(s);

			// Clamp end
			if (segIndex >= track.curve.controlPoints.size() - 1)
			{
				v = 0;
				return;
			}

			float segLen = track.curve.segmentLengths[segIndex];

			glm::vec3 p0 = track.curve.controlPoints[segIndex];
			glm::vec3 p1 = track.curve.controlPoints[segIndex+1];
			glm::vec3 tangent = glm::normalize(p1 - p0);

			// Gravity projection onto tangent
			float a = 0.01f * glm::dot(GRAVITY, tangent);

			float v0 = v;
			float tMax = remainingTime;

			// Predicted velocity
			float v1 = v0 + a * tMax;

			// Predicted displacement along tangent
			float ds = v0 * tMax + 0.5f * a * tMax * tMax;

			// ------------------ FORWARD MOTION ------------------
			if (ds > 0.0f)
			{
				float segRemaining = segLen - dist;

				if (ds < segRemaining)
				{
					// Does not reach boundary -> simple update
					s += ds;
					v = v1;
					return;
				}

				// Reaches next boundary -> compute exact hit time
				float A = 0.5f * a;
				float B = v0;
				float C = -segRemaining;

				float dtHit;
				if (fabs(a) < 1e-6f)     // linear motion
					dtHit = segRemaining / v0;
				else
				{
					float disc = B * B - 4 * A * C;
					dtHit = (-B + sqrt(disc)) / (2 * A);
				}

				// Move exactly to segment end
				s += segRemaining;
				v += a * dtHit;

				remainingTime -= dtHit;
				continue;
			}

			// ------------------ BACKWARD MOTION ------------------
			if (ds < 0.0f)
			{
				float segRemainingBack = dist;  // distance to previous segment

				if (-ds < segRemainingBack)
				{
					// Does not reach previous boundary
					s += ds;  // ds is negative
					v = v1;
					return;
				}

				// Hits previous boundary -> compute exact hit time
				float A = 0.5f * a;
				float B = v0;
				float C = segRemainingBack; // positive number

				float dtHit;
				if (fabs(a) < 1e-6f)
					dtHit = segRemainingBack / fabs(v0);
				else
				{
					float disc = B * B - 4 * A * C;
					dtHit = (-B - sqrt(disc)) / (2 * A); // minus sign for backward
				}

				dtHit = fabs(dtHit);

				// Move exactly to previous boundary
				s -= segRemainingBack;
				v += a * dtHit;

				remainingTime -= dtHit;
				continue;
			}

			// ------------------ No movement (ds=0), gravity aligned? ------------------
			return;
		}
	}

	void mainLoop()
	{
		ImGuizmo::AllowAxisFlip(false);
		double startTime, endTime;
		while (!glfwWindowShouldClose(window))
		{
			startTime = glfwGetTime();
			glfwPollEvents();

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			camera.updateView(window, 0.0f);

			ImGuizmo::SetRect(0, 0, swapChain.extent.width, swapChain.extent.height);
			showTranslateOnHover();
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Open"))
					{
						nfdu8char_t* outPath = NULL;
						nfdu8filteritem_t filters[1] = { { "YAML", "yaml" } };
						nfdopendialogu8args_t args = { 0 };
						args.filterList = filters;
						args.filterCount = 1;
						nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
						if (result == NFD_OKAY) {
							loadTrack(outPath);
							NFD_FreePathU8(outPath);
						}
					}
					if (ImGui::MenuItem("Save As..."))
					{
						nfdu8char_t* savePath = NULL;
						nfdu8filteritem_t filters[1] = { { "YAML", "yaml" } };
						nfdsavedialogu8args_t args = { 0 };
						args.filterList = filters;
						args.filterCount = 1;
						nfdresult_t result = NFD_SaveDialogU8_With(&savePath, &args);
						if (result == NFD_OKAY) {
							saveTrack(savePath);
							NFD_FreePathU8(savePath);
						}
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Exit"))
					{
						glfwSetWindowShouldClose(window, true);
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Help"))
				{
					if (ImGui::MenuItem("About")) {
						showAbout = true;
					}
					ImGui::EndMenu();
				}

				ImGui::Separator();
				ImGui::Text(currentTrackFileName.c_str());

				ImGui::EndMainMenuBar();
			}

			ImGuiIO& io = ImGui::GetIO();
			ImVec2 winSize(400.0f, 50.0f);
			ImVec2 pos = ImVec2(0.0f, io.DisplaySize.y - winSize.y);

			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(winSize);

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoBringToFrontOnFocus;

			ImGui::Begin("Track Controls", nullptr, flags);

			if (ImGui::SliderFloat("Arc Length", &u, 0.0f, 1.0f))
			{
				s = track.curve.normalizedToArcLength(u);
				v = 0;
			}
			else 
			{
				if (doSimulate)
				{
					doPhysics();
				}
				s = glm::clamp(s, 0.0f, track.curve.cumulativeLengths.empty() ? 0.0f : track.curve.cumulativeLengths.back());
				u = track.curve.arcLengthToNormalized(s);
			}
			ImGui::Checkbox("Simulate Physics", &doSimulate);

			ImGui::End();

			if (trackMesh)
			{
				ImDrawList* drawList = ImGui::GetForegroundDrawList();

				glm::vec3 curvePos = track.curve.evaluate(s);
				glm::vec2 screenPos = camera.projectPositionToScreen(curvePos, swapChain.extent.width, swapChain.extent.height);
				float scale = 1.0f / (1.0f + camera.depthOfPoint(curvePos) * 0.1f);
				//drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), 20.0f * scale, IM_COL32(255, 0, 0, 255));

				glm::mat4 proj(camera.proj);
				proj[1][1] *= -1;
				glm::mat4 frenet = track.evaluateFrenetInterpolated(s);
				glm::mat4 model = 0.17f * glm::identity<glm::mat4>();
				model[3][3] = 1.0f;
				model[3][1] += 0.05f;
				model = frenet * model;
				glm::mat4 id = glm::identity<glm::mat4>();
				ImGuizmo::DrawCubes(glm::value_ptr(camera.view), glm::value_ptr(proj), glm::value_ptr(model), 1);
				//ImGuizmo::Manipulate(glm::value_ptr(camera.view), glm::value_ptr(proj), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, glm::value_ptr(frenet), glm::value_ptr(id));
			}

			if (showAbout) {
				ImGuiIO& io = ImGui::GetIO();
				ImVec2 viewportSize = io.DisplaySize;

				// Draw dark overlay
				ImDrawList* drawList = ImGui::GetBackgroundDrawList();
				drawList->AddRectFilled(
					ImVec2(0, 0), viewportSize, IM_COL32(0, 0, 0, 120) // semi-transparent black
				);

				// Centered window
				ImVec2 windowSize(viewportSize.x * 0.35f, viewportSize.y * 0.35f);
				ImVec2 center = ImVec2(viewportSize.x * 0.5f - windowSize.x * 0.5f,
					viewportSize.y * 0.5f - windowSize.y * 0.5f);
				ImGui::SetNextWindowPos(center);
				ImGui::SetNextWindowSize(windowSize);

				ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize
					| ImGuiWindowFlags_NoCollapse
					| ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoSavedSettings
					| ImGuiWindowFlags_NoDecoration;

				// Make window slightly transparent
				ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(255 * col.x, 255 * col.y, 255 * col.z, 255));

				if (ImGui::Begin("About", &showAbout, flags)) {
					ImGui::Text("Osprey v0.1");
					ImGui::Separator();

					const char* text = 
						"Copyright [2026] Lennart S\n"
						"\n"
						"Licensed under the Apache License, Version 2.0 (the \"License\");\n"
						"you may not use this file except in compliance with the License.\n"
						"You may obtain a copy of the License at\n"
						"\n"
						"  http ://www.apache.org/licenses/LICENSE-2.0\n"
						"\n"
						"Unless required by applicable law or agreed to in writing, software\n"
						"distributed under the License is distributed on an \"AS IS\" BASIS,\n"
						"WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
						"See the License for the specific language governing permissions and \n"
						"limitations under the License.\n";
					ImVec2 textSize = ImGui::CalcTextSize(text);
					ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
					ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
					ImGui::Text(text);
					ImGui::Dummy(ImVec2(0, 20));
				}
				ImGui::End();

				ImGui::PopStyleColor();
			}
			ImGui::EndFrame();

			ImGui::Render();

			drawFrame();
		}

		context.device.waitIdle();

		endTime = glfwGetTime();
		double timeDiff = endTime - startTime;
		if (timeDiff < dt)
		{
			std::this_thread::sleep_for(std::chrono::microseconds((int)glm::round(timeDiff * 1000000)));
		}
	}

	void cleanup() const
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		context.device.waitIdle();

		swapChain = {};
		swapChain = osp::Swapchain(context, *context.surface, window);
		swapChainRebuild = true;

		renderAttachments = {};
		renderAttachments = osp::RenderAttachments(context, swapChain.extent, swapChain.surfaceFormat.format);

		camera.updateProj(window, 0.0f);
	}

	void showTranslateOnHover()
	{
		if (showAbout) return;
		std::vector<glm::vec3>& controlPoints = track.curve.controlPoints;
		if (ImGuizmo::IsUsing() && lastHoveredControlPointIndex != -1)
		{
			glm::mat4 proj(camera.proj);
			proj[1][1] *= -1;
			glm::mat4 id = glm::identity<glm::mat4>();
			id[3][0] = controlPoints[lastHoveredControlPointIndex][0];
			id[3][1] = controlPoints[lastHoveredControlPointIndex][1];
			id[3][2] = controlPoints[lastHoveredControlPointIndex][2];
			glm::mat4 delta = glm::identity<glm::mat4>();

			ImGuizmo::Manipulate(glm::value_ptr(camera.view), glm::value_ptr(proj), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, glm::value_ptr(id), glm::value_ptr(delta));
			controlPoints[lastHoveredControlPointIndex][0] += delta[3][0];
			controlPoints[lastHoveredControlPointIndex][1] += delta[3][1];
			controlPoints[lastHoveredControlPointIndex][2] += delta[3][2];

			createTrack();
			return;
		}
		lastHoveredControlPointIndex = -1;
		float minDist = 100000000.0f;
		for (int i = 0; i < controlPoints.size(); i++)
		{
			glm::vec2 screenPos = camera.projectPositionToScreen(controlPoints[i], static_cast<uint32_t>(swapChain.extent.width), static_cast<uint32_t>(swapChain.extent.height));
			float dist = glm::distance(screenPos, screenCursorPos);
			if (dist < minDist)
			{
				lastHoveredControlPointIndex = i;
				minDist = dist;
			}
		}
		if (lastHoveredControlPointIndex != -1 && minDist < 50.0f)
		{
			glm::mat4 proj(camera.proj);
			proj[1][1] *= -1;
			glm::mat4 id = glm::identity<glm::mat4>();
			id[3][0] = controlPoints[lastHoveredControlPointIndex][0];
			id[3][1] = controlPoints[lastHoveredControlPointIndex][1];
			id[3][2] = controlPoints[lastHoveredControlPointIndex][2];
			ImGuizmo::Manipulate(glm::value_ptr(camera.view), glm::value_ptr(proj), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, glm::value_ptr(id));
		}
		else
		{
			lastHoveredControlPointIndex = -1;
		}
	}

	void createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = context.queueIndex };
		commandPool = vk::raii::CommandPool(context.device, poolInfo);
	}

	static bool hasStencilComponent(vk::Format format)
	{
		return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
	}

	void generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if image format supports linear blit-ing
		vk::FormatProperties formatProperties = context.physicalDevice.getFormatProperties(imageFormat);

		if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
		{
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();

		vk::ImageMemoryBarrier barrier = { .srcAccessMask = vk::AccessFlagBits::eTransferWrite, .dstAccessMask = vk::AccessFlagBits::eTransferRead, .oldLayout = vk::ImageLayout::eTransferDstOptimal, .newLayout = vk::ImageLayout::eTransferSrcOptimal, .srcQueueFamilyIndex = vk::QueueFamilyIgnored, .dstQueueFamilyIndex = vk::QueueFamilyIgnored, .image = image };
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

			vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dstOffsets;
			offsets[0] = vk::Offset3D(0, 0, 0);
			offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
			dstOffsets[0] = vk::Offset3D(0, 0, 0);
			dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
			vk::ImageBlit blit = { .srcSubresource = {}, .srcOffsets = offsets, .dstSubresource = {}, .dstOffsets = dstOffsets };
			blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
			blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);

			commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

		endSingleTimeCommands(*commandBuffer);
	}

	void copyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width, uint32_t height)
	{
		std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();
		vk::BufferImageCopy                      region{
								 .bufferOffset = 0,
								 .bufferRowLength = 0,
								 .bufferImageHeight = 0,
								 .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
								 .imageOffset = {0, 0, 0},
								 .imageExtent = {width, height, 1} };
		commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
		endSingleTimeCommands(*commandBuffer);
	}

	void createTrack()
	{
		context.device.waitIdle();
		trackMesh = std::make_unique<osp::TrackMesh>(context.device, context.physicalDevice, context.queue, commandPool);
		trackMesh->track = track;
		track.update();
		trackMesh->generateMesh();
	}

	void createGroundGrid()
	{
		groundGridMesh = std::make_unique<osp::Mesh>(context.device, context.physicalDevice, context.queue, commandPool);

		std::vector<osp::Vertex>& vertices = groundGridMesh->data.vertices;
		std::vector<uint32_t>& indices = groundGridMesh->data.indices;

		glm::vec3 color{ 10.0f, 10.0f, 10.0f };
		color /= 256.0f;
		float gridHalfSize = 1000.0f;
		float spacing = 1.0f;
		const int lineCount = static_cast<int>((gridHalfSize * 2.0f) / spacing) + 1;

		uint32_t index = 0;

		for (int i = 0; i < lineCount; ++i)
		{
			float pos = -gridHalfSize + i * spacing;

			// Line parallel to X axis (along Z)
			vertices.push_back({{ -gridHalfSize, 0.0f, pos }, color, { 0.0f, 0.0f }, UP_DIR});
			vertices.push_back({{  gridHalfSize, 0.0f, pos }, color, { 0.0f, 0.0f }, UP_DIR });

			indices.push_back(index++);
			indices.push_back(index++);
			indices.push_back(index-1);

			// Line parallel to Z axis (along X)
			vertices.push_back({ { pos, 0.0f, -gridHalfSize }, color, { 0.0f, 0.0f }, UP_DIR });
			vertices.push_back({ { pos, 0.0f, gridHalfSize }, color, { 0.0f, 0.0f }, UP_DIR });

			indices.push_back(index++);
			indices.push_back(index++);
			indices.push_back(index-1);
		}

		groundGridMesh->upload();
	}

	void createDescriptorPool()
	{
		std::array poolSize{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT) };
		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data() };
		descriptorPool = vk::raii::DescriptorPool(context.device, poolInfo);
	}

	void createFrameResources()
	{
		for (auto& frame : frames) {
			frame = osp::Frame(context, commandPool, descriptorPool, *mainPipeline.descriptorSetLayout);
		}
	}

	std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands()
	{
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1 };
		std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(vk::raii::CommandBuffers(context.device, allocInfo).front()));

		vk::CommandBufferBeginInfo beginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		commandBuffer->begin(beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(const vk::raii::CommandBuffer& commandBuffer) const
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
		context.queue.submit(submitInfo, nullptr);
		context.queue.waitIdle();
	}

	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::raii::CommandBuffer       commandCopyBuffer = std::move(context.device.allocateCommandBuffers(allocInfo).front());
		commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{ .size = size });
		commandCopyBuffer.end();
		context.queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, nullptr);
		context.queue.waitIdle();
	}

	void recordCommandBuffer(uint32_t imageIndex)
	{
		auto& cmd = frames[currentFrame].cmd;

		cmd.begin({});
		// Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
		swapChain.transitionToMain(cmd, imageIndex);
		renderAttachments.transitionToMain(cmd);

		vk::ClearValue clearColor = vk::ClearColorValue(0.01f, 0.01f, 0.012f, 1.0f);
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

		// Color attachment (multisampled) with resolve attachment
		vk::RenderingAttachmentInfo colorAttachment = {
			.imageView = renderAttachments.color.view,
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.resolveMode = vk::ResolveModeFlagBits::eAverage,
			.resolveImageView = swapChain.imageViews[imageIndex],
			.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = clearColor };
		// Depth attachment
		vk::RenderingAttachmentInfo depthAttachment = {
			.imageView = renderAttachments.depth.view,
			.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eDontCare,
			.clearValue = clearDepth };
		vk::RenderingInfo renderingInfo = {
			.renderArea = {.offset = {0, 0}, .extent = swapChain.extent},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = &depthAttachment };

		cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.extent.width), static_cast<float>(swapChain.extent.height), 0.0f, 1.0f));
		cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.extent));

		cmd.beginRendering(renderingInfo);
		// Draw Background
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *backgroundPipeline.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *backgroundPipeline.pipelineLayout, 0, *frames[currentFrame].descriptorSet, nullptr);
		cmd.setDepthTestEnable(false);
		cmd.draw(3, 1, 0, 0);


		// Draw Ground
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mainPipeline.pipeline);
		cmd.bindVertexBuffers(0, *groundGridMesh->vertexBuffer.buffer, { 0 });
		cmd.setDepthTestEnable(false);
		cmd.bindIndexBuffer(*groundGridMesh->indexBuffer.buffer, 0, vk::IndexType::eUint32);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mainPipeline.pipelineLayout, 0, *frames[currentFrame].descriptorSet, nullptr);
		cmd.drawIndexed(groundGridMesh->data.indices.size(), 1, 0, 0, 0);

		// Draw Track Mesh
		if (trackMesh != nullptr && trackMesh->mesh.data.indices.size() > 0)
		{
			cmd.bindVertexBuffers(0, *trackMesh->mesh.vertexBuffer.buffer, { 0 });
			cmd.setDepthTestEnable(true);
			cmd.bindIndexBuffer(*trackMesh->mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
			//cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);
			cmd.drawIndexed(trackMesh->mesh.data.indices.size(), 1, 0, 0, 0);
		}

		cmd.endRendering();

		// Draw ImGui
		drawImGui(cmd, swapChain.imageViews[imageIndex]);

		// After rendering, transition the swapchain image to PRESENT_SRC
		swapChain.transitionToPresent(cmd, imageIndex);
		cmd.end();
	}

	void updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto  currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float>(currentTime - startTime).count();

		frames[currentImage].updateUBO(glm::identity<glm::mat4>(), camera.view, camera.proj, glm::normalize(glm::vec3(0.0, 0.3, 1.0)));
	}

	void drawFrame()
	{
		auto& frame = frames[currentFrame];
		while (vk::Result::eTimeout == context.device.waitForFences(*frame.inFlight, vk::True, UINT64_MAX));
		auto [result, imageIndex] = swapChain.swapChainKHR.acquireNextImage(UINT64_MAX, *frame.imageAvailable, nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapChain();
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		updateUniformBuffer(currentFrame);

		context.device.resetFences(*frame.inFlight);
		frame.cmd.reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*frame.imageAvailable, .pWaitDstStageMask = &waitDestinationStageMask, .commandBufferCount = 1, .pCommandBuffers = &*frame.cmd, .signalSemaphoreCount = 1, .pSignalSemaphores = &*swapChain.renderFinished[imageIndex] };
		context.queue.submit(submitInfo, *frame.inFlight);

		try
		{
			const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*swapChain.renderFinished[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain.swapChainKHR, .pImageIndices = &imageIndex};
			result = context.queue.presentKHR(presentInfoKHR);
			if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
			{
				framebufferResized = false;
				recreateSwapChain();
			}
			else if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("failed to present swap chain image!");
			}
		}
		catch (const vk::SystemError& e)
		{
			if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR))
			{
				recreateSwapChain();
				return;
			}
			else
			{
				throw;
			}
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void drawImGui(vk::CommandBuffer cmd, vk::ImageView targetImageView)
	{
		auto drawData = ImGui::GetDrawData();
		if (!drawData) return;
		vk::RenderingAttachmentInfo colorAttachment{
			.imageView = targetImageView,
			.imageLayout = vk::ImageLayout::eGeneral,
			.loadOp = vk::AttachmentLoadOp::eLoad,
			.storeOp = vk::AttachmentStoreOp::eStore
		};
		vk::RenderingInfo renderInfo{
			.renderArea = {.offset = {0, 0}, .extent = swapChain.extent},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr
		};

		cmd.beginRendering(renderInfo);
		ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(cmd));
		cmd.endRendering();
	}
};