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

#define GLFW_INCLUDE_VULKAN        // REQUIRED only for GLFW CreateWindowSurface.
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

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int      MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const*> validationLayers = {
	"VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		return {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
			vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)) };
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};

template <>
struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const noexcept
	{
		auto h = std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec3>()(vertex.color) << 1);
		h = (h >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
		h = (h >> 1) ^ (std::hash<glm::vec3>()(vertex.normal) << 1);
		return h;
	}
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 lightDir;
};

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
	vk::raii::Context                context;
	vk::raii::Instance               instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::SurfaceKHR             surface = nullptr;
	vk::raii::PhysicalDevice         physicalDevice = nullptr;
	vk::SampleCountFlagBits          msaaSamples = vk::SampleCountFlagBits::e1;
	vk::raii::Device                 device = nullptr;
	uint32_t                         queueIndex = ~0;
	vk::raii::Queue                  queue = nullptr;
	vk::raii::SwapchainKHR           swapChain = nullptr;
	std::vector<vk::Image>           swapChainImages;
	vk::SurfaceFormatKHR             swapChainSurfaceFormat;
	vk::Extent2D                     swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	bool swapChainRebuild = true;

	vk::raii::DescriptorPool imguiPool = nullptr;

	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::raii::PipelineLayout      pipelineLayout = nullptr;
	vk::raii::Pipeline            graphicsPipeline = nullptr;

	vk::raii::Image        colorImage = nullptr;
	vk::raii::DeviceMemory colorImageMemory = nullptr;
	vk::raii::ImageView    colorImageView = nullptr;

	vk::raii::Image        depthImage = nullptr;
	vk::raii::DeviceMemory depthImageMemory = nullptr;
	vk::raii::ImageView    depthImageView = nullptr;

	std::vector<vk::raii::Buffer>       uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
	std::vector<void*>                 uniformBuffersMapped;

	vk::raii::DescriptorPool             descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;

	vk::raii::CommandPool                commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;

	std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
	std::vector<vk::raii::Fence>     inFlightFences;
	uint32_t                         semaphoreIndex = 0;
	uint32_t                         currentFrame = 0;

	bool framebufferResized = false;

	ImGui_ImplVulkanH_Window g_MainWindowData;
	uint32_t                 g_MinImageCount = 2;
	bool                     g_SwapChainRebuild = false;

	std::vector<const char*> requiredDeviceExtension = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName };

	glm::vec2 screenCursorPos = { 0.0f, 0.0f };
	uint32_t lastHoveredControlPointIndex = 0;
	bool isShift;

	osp::Camera camera;
	osp::Track track;
	std::unique_ptr<osp::TrackMesh> trackMesh;
	std::unique_ptr<osp::Mesh> groundGridMesh;

	std::string currentTrackFilePath = "F:\\Dev\\_VulkanProjects\\Osprey\\tracks\\track1.yaml";
	std::string currentTrackFileName = "track1.yaml";

	bool showAbout = false;

	// PHYSICS
	float dt = 0.016666;
	float u = 0.0f;
	float s = 0.0f;
	float v = 0.0f;

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
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		msaaSamples = getMaxUsableSampleCount();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createColorResources();
		createDepthResources();
		//createTrack();
		createGroundGrid();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
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

		imguiPool = device.createDescriptorPool(poolInfo, nullptr);

		// 2: initialize imgui library

		//this initializes the core structures of imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(window, false);

		VkFormat colorFormat = static_cast<VkFormat>(swapChainSurfaceFormat.format);

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info{
			.Instance = static_cast<VkInstance>(*instance),
			.PhysicalDevice = static_cast<VkPhysicalDevice>(*physicalDevice),
			.Device = static_cast<VkDevice>(*device),
			.Queue = static_cast<VkQueue>(*queue),
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

			float a = 0.001f * glm::dot(GRAVITY, tangent);

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

			ImGuizmo::SetRect(0, 0, swapChainExtent.width, swapChainExtent.height);
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

			ImGui::SetCursorPosY(winSize.y * 0.5f - 10.0f);
			if (ImGui::SliderFloat("Arc Length", &u, 0.0f, 1.0f))
			{
				s = track.curve.normalizedToArcLength(u);
				v = 0;
			}
			else 
			{
				doPhysics();
				s = glm::clamp(s, 0.0f, track.curve.cumulativeLengths.empty() ? 0.0f : track.curve.cumulativeLengths.back());
				u = track.curve.arcLengthToNormalized(s);
			}

			ImGui::End();

			if (trackMesh)
			{
				ImDrawList* drawList = ImGui::GetForegroundDrawList();

				glm::vec3 curvePos = track.curve.evaluate(s);
				glm::vec2 screenPos = camera.projectPositionToScreen(curvePos, swapChainExtent.width, swapChainExtent.height);
				float scale = 1.0f / (1.0f + camera.depthOfPoint(curvePos) * 0.1f);
				drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), 20.0f * scale, IM_COL32(255, 0, 0, 255));
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
						"Copyright [yyyy] Lennart S\n"
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

		device.waitIdle();

		endTime = glfwGetTime();
		double timeDiff = endTime - startTime;
		if (timeDiff < dt)
		{
			std::this_thread::sleep_for(std::chrono::microseconds((int)glm::round(timeDiff * 1000000)));
		}
	}

	void cleanupSwapChain()
	{
		swapChainImageViews.clear();
		swapChain = nullptr;
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

		device.waitIdle();

		cleanupSwapChain();
		createSwapChain();
		createImageViews();
		createColorResources();
		createDepthResources();

		camera.updateProj(window, 0.0f);
	}

	void createInstance()
	{
		constexpr vk::ApplicationInfo appInfo{ .pApplicationName = "Hello Triangle",
											  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
											  .pEngineName = "No Engine",
											  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
											  .apiVersion = vk::ApiVersion14 };

		// Get the required layers
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers)
		{
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

		// Check if the required layers are supported by the Vulkan implementation.
		auto layerProperties = context.enumerateInstanceLayerProperties();
		for (auto const& requiredLayer : requiredLayers)
		{
			if (std::ranges::none_of(layerProperties,
				[requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; }))
			{
				throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
			}
		}

		// Get the required extensions.
		auto requiredExtensions = getRequiredExtensions();

		// Check if the required extensions are supported by the Vulkan implementation.
		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (auto const& requiredExtension : requiredExtensions)
		{
			if (std::ranges::none_of(extensionProperties,
				[requiredExtension](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
			{
				throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
			}
		}

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data() };
		instance = vk::raii::Instance(context, createInfo);
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers)
			return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfoEXT{
			 .messageSeverity = severityFlags,
			 .messageType = messageTypeFlags,
			 .pfnUserCallback = &debugCallback };
		debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void createSurface()
	{
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
		{
			throw std::runtime_error("failed to create window surface!");
		}
		surface = vk::raii::SurfaceKHR(instance, _surface);
	}

	void pickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
		const auto                            devIter = std::ranges::find_if(
			devices,
			[&](auto const& device) {
				// Check if the device supports the Vulkan 1.3 API version
				bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

				// Check if any of the queue families support graphics operations
				auto queueFamilies = device.getQueueFamilyProperties();
				bool supportsGraphics =
					std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

				// Check if all required device extensions are available
				auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
				bool supportsAllRequiredExtensions =
					std::ranges::all_of(requiredDeviceExtension,
						[&availableDeviceExtensions](auto const& requiredDeviceExtension) {
							return std::ranges::any_of(availableDeviceExtensions,
								[requiredDeviceExtension](auto const& availableDeviceExtension) { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
						});

				auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
				bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
					features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
					features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

				return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
			});
		if (devIter != devices.end())
		{
			physicalDevice = *devIter;
		}
		else
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice()
	{
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		// get the first index into queueFamilyProperties which supports both graphics and present
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
		{
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
				physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
			{
				// found a queue family that supports both graphics and present
				queueIndex = qfpIndex;
				break;
			}
		}
		if (queueIndex == ~0)
		{
			throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
		}

		// query for Vulkan 1.3 features
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
			{.features = {.fillModeNonSolid = true, .wideLines = true, .samplerAnisotropy = true}},        // vk::PhysicalDeviceFeatures2
			{.synchronization2 = true, .dynamicRendering = true},        // vk::PhysicalDeviceVulkan13Features
			{.extendedDynamicState = true}                               // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
		};

		// create a Device
		float                     queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
		vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
												   .queueCreateInfoCount = 1,
												   .pQueueCreateInfos = &deviceQueueCreateInfo,
												   .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
												   .ppEnabledExtensionNames = requiredDeviceExtension.data() };

		device = vk::raii::Device(physicalDevice, deviceCreateInfo);
		queue = vk::raii::Queue(device, queueIndex, 0);
	}

	void showTranslateOnHover()
	{
		if (showAbout) return;
		std::vector<glm::vec3>& controlPoints = track.curve.controlPoints;
		if (ImGuizmo::IsUsing())
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
			glm::vec2 screenPos = camera.projectPositionToScreen(controlPoints[i], static_cast<uint32_t>(swapChainExtent.width), static_cast<uint32_t>(swapChainExtent.height));
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

	void createSwapChain()
	{
		auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
		swapChainExtent = chooseSwapExtent(surfaceCapabilities);
		swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *surface,
													   .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
													   .imageFormat = swapChainSurfaceFormat.format,
													   .imageColorSpace = swapChainSurfaceFormat.colorSpace,
													   .imageExtent = swapChainExtent,
													   .imageArrayLayers = 1,
													   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
													   .imageSharingMode = vk::SharingMode::eExclusive,
													   .preTransform = surfaceCapabilities.currentTransform,
													   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
													   .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface)),
													   .clipped = true };

		swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
		swapChainImages = swapChain.getImages();
		swapChainRebuild = true;
	}

	void createImageViews()
	{
		assert(swapChainImageViews.empty());

		vk::ImageViewCreateInfo imageViewCreateInfo{
			.viewType = vk::ImageViewType::e2D,
			.format = swapChainSurfaceFormat.format,
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
		for (auto image : swapChainImages)
		{
			imageViewCreateInfo.image = image;
			swapChainImageViews.emplace_back(device, imageViewCreateInfo);
		}
	}

	void createDescriptorSetLayout()
	{
		std::array bindings = {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr) };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
	}

	void createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"));

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto                                   bindingDescription = Vertex::getBindingDescription();
		auto                                   attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data() };
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False };
		vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1,
			.scissorCount = 1 };
		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eLine,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable = vk::False };
		rasterizer.lineWidth = 1.0f;
		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = msaaSamples,
			.sampleShadingEnable = vk::False };
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
			.depthTestEnable = vk::True,
			.depthWriteEnable = vk::True,
			.depthCompareOp = vk::CompareOp::eLess,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable = vk::False };
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = vk::False;

		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = vk::False,
			.logicOp = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment };

		std::vector dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eDepthTestEnable};
		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0 };

		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		vk::Format depthFormat = findDepthFormat();

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{.stageCount = 2,
			 .pStages = shaderStages,
			 .pVertexInputState = &vertexInputInfo,
			 .pInputAssemblyState = &inputAssembly,
			 .pViewportState = &viewportState,
			 .pRasterizationState = &rasterizer,
			 .pMultisampleState = &multisampling,
			 .pDepthStencilState = &depthStencil,
			 .pColorBlendState = &colorBlending,
			 .pDynamicState = &dynamicState,
			 .layout = pipelineLayout,
			 .renderPass = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format, .depthAttachmentFormat = depthFormat} };

		graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queueIndex };
		commandPool = vk::raii::CommandPool(device, poolInfo);
	}

	void createColorResources()
	{
		vk::Format colorFormat = swapChainSurfaceFormat.format;

		createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage, colorImageMemory);
		colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
	}

	void createDepthResources()
	{
		vk::Format depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
	}

	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
	{
		for (const auto format : candidates)
		{
			vk::FormatProperties props = physicalDevice.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	[[nodiscard]] vk::Format findDepthFormat() const
	{
		return findSupportedFormat(
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	}

	static bool hasStencilComponent(vk::Format format)
	{
		return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
	}

	void generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// Check if image format supports linear blit-ing
		vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);

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

	vk::SampleCountFlagBits getMaxUsableSampleCount()
	{
		vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

		vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & vk::SampleCountFlagBits::e64)
		{
			return vk::SampleCountFlagBits::e64;
		}
		if (counts & vk::SampleCountFlagBits::e32)
		{
			return vk::SampleCountFlagBits::e32;
		}
		if (counts & vk::SampleCountFlagBits::e16)
		{
			return vk::SampleCountFlagBits::e16;
		}
		if (counts & vk::SampleCountFlagBits::e8)
		{
			return vk::SampleCountFlagBits::e8;
		}
		if (counts & vk::SampleCountFlagBits::e4)
		{
			return vk::SampleCountFlagBits::e4;
		}
		if (counts & vk::SampleCountFlagBits::e2)
		{
			return vk::SampleCountFlagBits::e2;
		}

		return vk::SampleCountFlagBits::e1;
	}

	[[nodiscard]] vk::raii::ImageView createImageView(const vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) const
	{
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = {aspectFlags, 0, mipLevels, 0, 1} };
		return vk::raii::ImageView(device, viewInfo);
	}

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory)
	{
		vk::ImageCreateInfo imageInfo{
			.imageType = vk::ImageType::e2D,
			.format = format,
			.extent = {width, height, 1},
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = numSamples,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = vk::SharingMode::eExclusive,
			.initialLayout = vk::ImageLayout::eUndefined };
		image = vk::raii::Image(device, imageInfo);

		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) };
		imageMemory = vk::raii::DeviceMemory(device, allocInfo);
		image.bindMemory(imageMemory, 0);
	}

	void transitionImageLayout(const vk::raii::Image& image, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, uint32_t mipLevels)
	{
		const auto commandBuffer = beginSingleTimeCommands();

		vk::ImageMemoryBarrier barrier{
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.image = image,
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1} };

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
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
		device.waitIdle();
		trackMesh = std::make_unique<osp::TrackMesh>(device, physicalDevice, queue, commandPool);
		trackMesh->track = track;
		track.update();
		trackMesh->generateMesh();
	}

	void createGroundGrid()
	{
		groundGridMesh = std::make_unique<osp::Mesh>(device, physicalDevice, queue, commandPool);

		std::vector<osp::Vertex>& vertices = groundGridMesh->data.vertices;
		std::vector<uint32_t>& indices = groundGridMesh->data.indices;

		glm::vec3 color{ 10, 10, 10 };
		color /= 256.0;
		float gridHalfSize = 1000.0;
		float spacing = 0.5;
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

	void createUniformBuffers()
	{
		uniformBuffers.clear();
		uniformBuffersMemory.clear();
		uniformBuffersMapped.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DeviceSize         bufferSize = sizeof(UniformBufferObject);
			vk::raii::Buffer       buffer({});
			vk::raii::DeviceMemory bufferMem({});
			createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMem);
			uniformBuffers.emplace_back(std::move(buffer));
			uniformBuffersMemory.emplace_back(std::move(bufferMem));
			uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));
		}
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
		descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
	}

	void createDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo{
				   .descriptorPool = descriptorPool,
				   .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
				   .pSetLayouts = layouts.data() };

		descriptorSets.clear();
		descriptorSets = device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorBufferInfo bufferInfo{
				.buffer = uniformBuffers[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject) };
			std::array descriptorWrites{
				vk::WriteDescriptorSet{
					.dstSet = descriptorSets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pBufferInfo = &bufferInfo}
			};
			device.updateDescriptorSets(descriptorWrites, {});
		}
	}

	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
	{
		vk::BufferCreateInfo bufferInfo{
			.size = size,
			.usage = usage,
			.sharingMode = vk::SharingMode::eExclusive };
		buffer = vk::raii::Buffer(device, bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) };
		bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		buffer.bindMemory(bufferMemory, 0);
	}

	std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands()
	{
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1 };
		std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(vk::raii::CommandBuffers(device, allocInfo).front()));

		vk::CommandBufferBeginInfo beginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		commandBuffer->begin(beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(const vk::raii::CommandBuffer& commandBuffer) const
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
		queue.submit(submitInfo, nullptr);
		queue.waitIdle();
	}

	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::raii::CommandBuffer       commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
		commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{ .size = size });
		commandCopyBuffer.end();
		queue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, nullptr);
		queue.waitIdle();
	}

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createCommandBuffers()
	{
		commandBuffers.clear();
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT };
		commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
	}

	void recordCommandBuffer(uint32_t imageIndex)
	{
		commandBuffers[currentFrame].begin({});
		// Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
		transition_image_layout(
			swapChainImages[imageIndex],
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},                                                        // srcAccessMask (no need to wait for previous operations)
			vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // dstStage
			vk::ImageAspectFlagBits::eColor);
		// Transition the multisampled color image to COLOR_ATTACHMENT_OPTIMAL
		transition_image_layout(
			*colorImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::AccessFlagBits2::eColorAttachmentWrite,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::ImageAspectFlagBits::eColor);
		// Transition the depth image to DEPTH_ATTACHMENT_OPTIMAL
		transition_image_layout(
			*depthImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::ImageAspectFlagBits::eDepth);

		vk::ClearValue clearColor = vk::ClearColorValue(0.01f, 0.01f, 0.012f, 1.0f);
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

		// Color attachment (multisampled) with resolve attachment
		vk::RenderingAttachmentInfo colorAttachment = {
			.imageView = colorImageView,
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.resolveMode = vk::ResolveModeFlagBits::eAverage,
			.resolveImageView = swapChainImageViews[imageIndex],
			.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = clearColor };

		// Depth attachment
		vk::RenderingAttachmentInfo depthAttachment = {
			.imageView = depthImageView,
			.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eDontCare,
			.clearValue = clearDepth };

		vk::RenderingInfo renderingInfo = {
			.renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = &depthAttachment };

		commandBuffers[currentFrame].beginRendering(renderingInfo);
		commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
		commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

		// Draw Ground
		commandBuffers[currentFrame].bindVertexBuffers(0, *groundGridMesh->vertexBuffer.buffer, { 0 });
		commandBuffers[currentFrame].setDepthTestEnable(false);
		commandBuffers[currentFrame].bindIndexBuffer(*groundGridMesh->indexBuffer.buffer, 0, vk::IndexType::eUint32);
		commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);
		commandBuffers[currentFrame].drawIndexed(groundGridMesh->data.indices.size(), 1, 0, 0, 0);

		// Draw Track Mesh
		if (trackMesh != nullptr && trackMesh->mesh.data.indices.size() > 0)
		{
			commandBuffers[currentFrame].bindVertexBuffers(0, *trackMesh->mesh.vertexBuffer.buffer, { 0 });
			commandBuffers[currentFrame].setDepthTestEnable(true);
			commandBuffers[currentFrame].bindIndexBuffer(*trackMesh->mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
			commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[currentFrame], nullptr);
			commandBuffers[currentFrame].drawIndexed(trackMesh->mesh.data.indices.size(), 1, 0, 0, 0);
		}

		commandBuffers[currentFrame].endRendering();



		// Draw ImGui
		drawImGui(commandBuffers[currentFrame], swapChainImageViews[imageIndex]);

		// After rendering, transition the swapchain image to PRESENT_SRC
		transition_image_layout(
			swapChainImages[imageIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
			{},                                                        // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
			vk::PipelineStageFlagBits2::eBottomOfPipe,                 // dstStage
			vk::ImageAspectFlagBits::eColor);
		commandBuffers[currentFrame].end();
	}

	void transition_image_layout(
		vk::Image               image,
		vk::ImageLayout         old_layout,
		vk::ImageLayout         new_layout,
		vk::AccessFlags2        src_access_mask,
		vk::AccessFlags2        dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlags    image_aspect_flags)
	{
		vk::ImageMemoryBarrier2 barrier = {
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				   .aspectMask = image_aspect_flags,
				   .baseMipLevel = 0,
				   .levelCount = 1,
				   .baseArrayLayer = 0,
				   .layerCount = 1} };
		vk::DependencyInfo dependency_info = {
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier };
		commandBuffers[currentFrame].pipelineBarrier2(dependency_info);
	}

	void createSyncObjects()
	{
		presentCompleteSemaphore.clear();
		renderFinishedSemaphore.clear();
		inFlightFences.clear();

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
			renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			inFlightFences.emplace_back(device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
		}
	}

	void updateUniformBuffer(uint32_t currentImage) const
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto  currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::identity<glm::mat4>();
		ubo.view = camera.view;
		ubo.proj = camera.proj;
		ubo.lightDir = glm::normalize(glm::vec3(0.0, 0.3, 1.0));

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void drawFrame()
	{
		while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));
		auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore[semaphoreIndex], nullptr);

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

		device.resetFences(*inFlightFences[currentFrame]);
		commandBuffers[currentFrame].reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*presentCompleteSemaphore[semaphoreIndex], .pWaitDstStageMask = &waitDestinationStageMask, .commandBufferCount = 1, .pCommandBuffers = &*commandBuffers[currentFrame], .signalSemaphoreCount = 1, .pSignalSemaphores = &*renderFinishedSemaphore[imageIndex] };
		queue.submit(submitInfo, *inFlightFences[currentFrame]);

		try
		{
			const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphore[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex };
			result = queue.presentKHR(presentInfoKHR);
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
		semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
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
			.renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
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

	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size(), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
		vk::raii::ShaderModule     shaderModule{ device, createInfo };

		return shaderModule;
	}

	static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
	{
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		assert(!availableFormats.empty());
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
		return std::ranges::any_of(availablePresentModes,
			[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
			vk::PresentModeKHR::eMailbox :
			vk::PresentModeKHR::eFifo;
	}

	[[nodiscard]] vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
	{
		if (capabilities.currentExtent.width != 0xFFFFFFFF)
		{
			return capabilities.currentExtent;
		}
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
	}

	[[nodiscard]] std::vector<const char*> getRequiredExtensions() const
	{
		uint32_t glfwExtensionCount = 0;
		auto     glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
		{
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

		return extensions;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
	{
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
		}

		return vk::False;
	}

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();

		return buffer;
	}
};