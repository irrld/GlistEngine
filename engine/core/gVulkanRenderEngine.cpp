/*
 * gVulkanRenderEngine.cpp
 *
 *  Created on: 2026-02-28
 *      Author: Metehan Gezer
 */

#if defined(GLIST_VULKAN)

#define VMA_IMPLEMENTATION
#include "gVulkanRenderEngine.h"
#include "gUbo.h"
#include "gGLFWWindow.h"
#include "gAppManager.h"
#include "gImage.h"
#include "gVbo.h"
#include "gTextureVk.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <limits>

// SPIR-V shader headers (generated at build time from shaders_vk/*.vert/*.frag)
#include "graphics/shaders_vk/color_vert.h"
#include "graphics/shaders_vk/color_frag.h"
#include "graphics/shaders_vk/image_vert.h"
#include "graphics/shaders_vk/image_frag.h"
#include "graphics/shaders_vk/texture_vert.h"
#include "graphics/shaders_vk/texture_frag.h"
#include "graphics/shaders_vk/font_vert.h"
#include "graphics/shaders_vk/font_frag.h"
#include "graphics/shaders_vk/grid_vert.h"
#include "graphics/shaders_vk/grid_frag.h"
#include "graphics/shaders_vk/skybox_vert.h"
#include "graphics/shaders_vk/skybox_frag.h"
#include "graphics/shaders_vk/shadowmap_vert.h"
#include "graphics/shaders_vk/shadowmap_frag.h"
#include "graphics/shaders_vk/pbr_vert.h"
#include "graphics/shaders_vk/pbr_frag.h"
#include "graphics/shaders_vk/cubemap_vert.h"
#include "graphics/shaders_vk/equirectangular_frag.h"
#include "graphics/shaders_vk/irradiance_frag.h"
#include "graphics/shaders_vk/prefilter_frag.h"
#include "graphics/shaders_vk/brdf_vert.h"
#include "graphics/shaders_vk/brdf_frag.h"
#include "graphics/shaders_vk/fbo_vert.h"
#include "graphics/shaders_vk/fbo_frag.h"
#include "graphics/shaders_vk/ssao_vert.h"
#include "graphics/shaders_vk/ssao_frag.h"
#include "graphics/shaders_vk/ssao_blur_frag.h"

// SPIR-V lookup table: maps @vk: identifiers to embedded SPIR-V data
struct SpirvEntry {
	const char* id;
	const char* data;
	size_t size;
};

static const SpirvEntry spirventries[] = {
	{"color_vert", reinterpret_cast<const char*>(shader_color_vert.data()), shader_color_vert.size()},
	{"color_frag", reinterpret_cast<const char*>(shader_color_frag.data()), shader_color_frag.size()},
	{"image_vert", reinterpret_cast<const char*>(shader_image_vert.data()), shader_image_vert.size()},
	{"image_frag", reinterpret_cast<const char*>(shader_image_frag.data()), shader_image_frag.size()},
	{"texture_vert", reinterpret_cast<const char*>(shader_texture_vert.data()), shader_texture_vert.size()},
	{"texture_frag", reinterpret_cast<const char*>(shader_texture_frag.data()), shader_texture_frag.size()},
	{"font_vert", reinterpret_cast<const char*>(shader_font_vert.data()), shader_font_vert.size()},
	{"font_frag", reinterpret_cast<const char*>(shader_font_frag.data()), shader_font_frag.size()},
	{"grid_vert", reinterpret_cast<const char*>(shader_grid_vert.data()), shader_grid_vert.size()},
	{"grid_frag", reinterpret_cast<const char*>(shader_grid_frag.data()), shader_grid_frag.size()},
	{"skybox_vert", reinterpret_cast<const char*>(shader_skybox_vert.data()), shader_skybox_vert.size()},
	{"skybox_frag", reinterpret_cast<const char*>(shader_skybox_frag.data()), shader_skybox_frag.size()},
	{"shadowmap_vert", reinterpret_cast<const char*>(shader_shadowmap_vert.data()), shader_shadowmap_vert.size()},
	{"shadowmap_frag", reinterpret_cast<const char*>(shader_shadowmap_frag.data()), shader_shadowmap_frag.size()},
	{"pbr_vert", reinterpret_cast<const char*>(shader_pbr_vert.data()), shader_pbr_vert.size()},
	{"pbr_frag", reinterpret_cast<const char*>(shader_pbr_frag.data()), shader_pbr_frag.size()},
	{"cubemap_vert", reinterpret_cast<const char*>(shader_cubemap_vert.data()), shader_cubemap_vert.size()},
	{"equirectangular_frag", reinterpret_cast<const char*>(shader_equirectangular_frag.data()), shader_equirectangular_frag.size()},
	{"irradiance_frag", reinterpret_cast<const char*>(shader_irradiance_frag.data()), shader_irradiance_frag.size()},
	{"prefilter_frag", reinterpret_cast<const char*>(shader_prefilter_frag.data()), shader_prefilter_frag.size()},
	{"brdf_vert", reinterpret_cast<const char*>(shader_brdf_vert.data()), shader_brdf_vert.size()},
	{"brdf_frag", reinterpret_cast<const char*>(shader_brdf_frag.data()), shader_brdf_frag.size()},
	{"fbo_vert", reinterpret_cast<const char*>(shader_fbo_vert.data()), shader_fbo_vert.size()},
	{"fbo_frag", reinterpret_cast<const char*>(shader_fbo_frag.data()), shader_fbo_frag.size()},
	{"ssao_vert", reinterpret_cast<const char*>(shader_ssao_vert.data()), shader_ssao_vert.size()},
	{"ssao_frag", reinterpret_cast<const char*>(shader_ssao_frag.data()), shader_ssao_frag.size()},
	{"ssao_blur_frag", reinterpret_cast<const char*>(shader_ssao_blur_frag.data()), shader_ssao_blur_frag.size()},
	{nullptr, nullptr, 0}, // sentinel
};

// ============================================================================
// Debug callback
// ============================================================================
VKAPI_ATTR VkBool32 VKAPI_CALL gVulkanRenderEngine::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		gLoge("Vulkan") << "Validation ERROR: " << pCallbackData->pMessage;
	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		gLogw("Vulkan") << "Validation WARN: " << pCallbackData->pMessage;
	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		gLogi("Vulkan") << "Validation INFO: " << pCallbackData->pMessage;
	}
	return VK_FALSE;
}

// ============================================================================
// Lifecycle
// ============================================================================
gVulkanRenderEngine::~gVulkanRenderEngine() {
}

void gVulkanRenderEngine::init() {
	createInstance();
	if (vkinstance == VK_NULL_HANDLE) {
		gLoge("Vulkan") << "init: Instance creation failed, aborting Vulkan init";
		return;
	}
#ifdef DEBUG
	setupDebugMessenger();
#endif
	createSurface();
	pickPhysicalDevice();
	if (vkphysicaldevice == VK_NULL_HANDLE) {
		gLoge("Vulkan") << "init: No physical device selected, aborting Vulkan init";
		return;
	}
	createLogicalDevice();
	if (vkdevice == VK_NULL_HANDLE) {
		gLoge("Vulkan") << "init: Device creation failed, aborting Vulkan init";
		return;
	}
	createVmaAllocator();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();

	// Reserve slot 0 in the buffer pool as "null"
	bufferpool.push_back({});
	bufferpoolused.push_back(true); // slot 0 is reserved

	// Reserve slot 0 in the VAO pool as "null"
	vaopool.push_back({});
	vaopoolused.push_back(true);

	// Reserve slot 0 in shader programs as "null"
	shaderprograms.push_back({});

	// Create pipeline infrastructure
	createDummyResources();
	createDescriptorSetLayouts();
	createPipelineLayout();
	createDescriptorPool();

	// Compute aligned stride for material UBO entries
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(vkphysicaldevice, &props);
		VkDeviceSize minAlignment = props.limits.minUniformBufferOffsetAlignment;
		materialubostride = sizeof(MaterialUBOData);
		if (minAlignment > 0) {
			materialubostride = (materialubostride + minAlignment - 1) & ~(minAlignment - 1);
		}
		gLogi("Vulkan") << "Material UBO stride: " << materialubostride << " (alignment: " << minAlignment << ")";
	}

	// Create per-frame material UBO ring buffers (one MaterialUBOData per draw call)
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkBufferCreateInfo bufinfo{};
		bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufinfo.size = materialubostride * MAX_MATERIAL_DRAWS_PER_FRAME;
		bufinfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocinfo{};
		allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocationInfo;
		vmaCreateBuffer(vmaallocator, &bufinfo, &allocinfo,
			&materialubobuffers[i].buffer, &materialubobuffers[i].allocation, &allocationInfo);
		materialubobuffers[i].size = materialubostride * MAX_MATERIAL_DRAWS_PER_FRAME;
		materialubobuffers[i].mapped = allocationInfo.pMappedData;
	}

	createDescriptorSets();

	gRenderer::init();
}

void gVulkanRenderEngine::cleanup() {
	if (vkdevice != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(vkdevice);
	}

	// Flush deferred deletion queues
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		for (auto& del : deletionqueues[i]) {
			if (del.imageview != VK_NULL_HANDLE) {
				vkDestroyImageView(vkdevice, del.imageview, nullptr);
			}
			if (del.sampler != VK_NULL_HANDLE) {
				vkDestroySampler(vkdevice, del.sampler, nullptr);
			}
			if (del.image != VK_NULL_HANDLE) {
				vmaDestroyImage(vmaallocator, del.image, del.allocation);
			} else if (del.buffer != VK_NULL_HANDLE) {
				vmaDestroyBuffer(vmaallocator, del.buffer, del.allocation);
			}
		}
		deletionqueues[i].clear();
	}

	// Destroy pipeline cache
	for (auto& entry : pipelinecache) {
		vkDestroyPipeline(vkdevice, entry.second, nullptr);
	}
	pipelinecache.clear();

	// Destroy shader programs
	for (size_t i = 1; i < shaderprograms.size(); i++) {
		if (shaderprograms[i].vertmodule != VK_NULL_HANDLE) vkDestroyShaderModule(vkdevice, shaderprograms[i].vertmodule, nullptr);
		if (shaderprograms[i].fragmodule != VK_NULL_HANDLE) vkDestroyShaderModule(vkdevice, shaderprograms[i].fragmodule, nullptr);
	}
	shaderprograms.clear();

	// Destroy material UBO buffers
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (materialubobuffers[i].buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(vmaallocator, materialubobuffers[i].buffer, materialubobuffers[i].allocation);
		}
	}

	// Destroy dummy resources
	if (dummysampler != VK_NULL_HANDLE) vkDestroySampler(vkdevice, dummysampler, nullptr);
	if (dummyimageview != VK_NULL_HANDLE) vkDestroyImageView(vkdevice, dummyimageview, nullptr);
	if (dummyimage != VK_NULL_HANDLE) vmaDestroyImage(vmaallocator, dummyimage, dummyimagealloc);

	// Destroy descriptor pools (automatically frees descriptor sets)
	if (scenedescriptorpool != VK_NULL_HANDLE) vkDestroyDescriptorPool(vkdevice, scenedescriptorpool, nullptr);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (materialdescriptorpools[i] != VK_NULL_HANDLE) vkDestroyDescriptorPool(vkdevice, materialdescriptorpools[i], nullptr);
	}

	// Destroy pipeline layout and descriptor set layouts
	if (pipelinelayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(vkdevice, pipelinelayout, nullptr);
	if (descriptorsetlayout0 != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(vkdevice, descriptorsetlayout0, nullptr);
	if (descriptorsetlayout1 != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(vkdevice, descriptorsetlayout1, nullptr);

	// Clear per-frame pools (actual buffers freed below)
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		rendercommands[i].clear();
		dyndrawpool[i].clear();
		dyndrawindex[i] = 0;
	}

	// Destroy buffer pool
	for (size_t i = 1; i < bufferpool.size(); i++) {
		if (bufferpoolused[i] && bufferpool[i].buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(vmaallocator, bufferpool[i].buffer, bufferpool[i].allocation);
		}
	}
	bufferpool.clear();
	bufferpoolused.clear();

	// Destroy VAO pool (no Vulkan resources to free, just state)
	vaopool.clear();
	vaopoolused.clear();

	cleanupSwapchain();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (renderfinishedsemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(vkdevice, renderfinishedsemaphores[i], nullptr);
		if (imageavailablesemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore(vkdevice, imageavailablesemaphores[i], nullptr);
		if (inflightfences[i] != VK_NULL_HANDLE) vkDestroyFence(vkdevice, inflightfences[i], nullptr);
	}

	if (vkcommandpool != VK_NULL_HANDLE) vkDestroyCommandPool(vkdevice, vkcommandpool, nullptr);
	if (vmaallocator != VK_NULL_HANDLE) vmaDestroyAllocator(vmaallocator);
	if (vkdevice != VK_NULL_HANDLE) vkDestroyDevice(vkdevice, nullptr);

#ifdef DEBUG
	if (vkdebugmessenger != VK_NULL_HANDLE) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkinstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func) func(vkinstance, vkdebugmessenger, nullptr);
	}
#endif

	if (vksurface != VK_NULL_HANDLE) vkDestroySurfaceKHR(vkinstance, vksurface, nullptr);
	if (vkinstance != VK_NULL_HANDLE) vkDestroyInstance(vkinstance, nullptr);

	gRenderer::cleanup();
}

// ============================================================================
// Vulkan initialization helpers
// ============================================================================
void gVulkanRenderEngine::createInstance() {
	VkApplicationInfo appinfo{};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "GlistEngine";
	appinfo.applicationVersion = VK_MAKE_VERSION(0, 1, 2);
	appinfo.pEngineName = "GlistEngine";
	appinfo.engineVersion = VK_MAKE_VERSION(0, 1, 2);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createinfo.pApplicationInfo = &appinfo;
	createinfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createinfo.ppEnabledExtensionNames = extensions.data();

#ifdef DEBUG
	const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
	// Check if validation layer is available
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	bool validationAvailable = false;
	for (const auto& layer : availableLayers) {
		if (strcmp(layer.layerName, validationLayers[0]) == 0) {
			validationAvailable = true;
			break;
		}
	}
	if (validationAvailable) {
		createinfo.enabledLayerCount = 1;
		createinfo.ppEnabledLayerNames = validationLayers;
		gLogi("Vulkan") << "Validation layers enabled";
	} else {
		gLogw("Vulkan") << "Validation layers not available";
		createinfo.enabledLayerCount = 0;
	}
#else
	createinfo.enabledLayerCount = 0;
#endif

#ifdef __APPLE__
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	createinfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createinfo.ppEnabledExtensionNames = extensions.data();
	createinfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkResult result = vkCreateInstance(&createinfo, nullptr, &vkinstance);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create Vulkan instance! Error: " << result;
		return;
	}
	gLogi("Vulkan") << "Vulkan instance created successfully";
}

void gVulkanRenderEngine::setupDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT debugcreateinfo{};
	debugcreateinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugcreateinfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugcreateinfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugcreateinfo.pfnUserCallback = debugCallback;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkinstance, "vkCreateDebugUtilsMessengerEXT");
	if (func) {
		func(vkinstance, &debugcreateinfo, nullptr, &vkdebugmessenger);
	}
}

void gVulkanRenderEngine::createSurface() {
	gGLFWWindow* glfwwindow = static_cast<gGLFWWindow*>(appmanager->getWindow());
	VkResult result = glfwCreateWindowSurface(vkinstance, glfwwindow->getGLFWwindow(), nullptr, &vksurface);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create window surface! Error: " << result;
	}
}

void gVulkanRenderEngine::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkinstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		gLoge("Vulkan") << "No GPUs with Vulkan support found!";
		return;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkinstance, &deviceCount, devices.data());

	// Pick the first device that has a graphics queue and supports the surface
	for (const auto& device : devices) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		bool foundGraphics = false, foundPresent = false;
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsqueuefamily = i;
				foundGraphics = true;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vksurface, &presentSupport);
			if (presentSupport) {
				presentqueuefamily = i;
				foundPresent = true;
			}
			if (foundGraphics && foundPresent) break;
		}

		if (foundGraphics && foundPresent) {
			vkphysicaldevice = device;
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);
			gLogi("Vulkan") << "Selected GPU: " << props.deviceName;
			// Prefer discrete GPU
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) break;
		}
	}

	if (vkphysicaldevice == VK_NULL_HANDLE) {
		gLoge("Vulkan") << "Failed to find a suitable GPU!";
	}
}

void gVulkanRenderEngine::createLogicalDevice() {
	std::set<uint32_t> uniqueQueueFamilies = {graphicsqueuefamily, presentqueuefamily};
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Query supported features and only enable what the GPU actually provides
	VkPhysicalDeviceFeatures supportedFeatures{};
	vkGetPhysicalDeviceFeatures(vkphysicaldevice, &supportedFeatures);

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
	deviceFeatures.fillModeNonSolid = supportedFeatures.fillModeNonSolid;
	deviceFeatures.wideLines = supportedFeatures.wideLines;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
		"VK_KHR_portability_subset",
#endif
	};

	VkDeviceCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createinfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createinfo.pQueueCreateInfos = queueCreateInfos.data();
	createinfo.pEnabledFeatures = &deviceFeatures;
	createinfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createinfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkResult result = vkCreateDevice(vkphysicaldevice, &createinfo, nullptr, &vkdevice);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create logical device! Error: " << result;
		return;
	}

	vkGetDeviceQueue(vkdevice, graphicsqueuefamily, 0, &vkgraphicsqueue);
	vkGetDeviceQueue(vkdevice, presentqueuefamily, 0, &vkpresentqueue);
}

void gVulkanRenderEngine::createVmaAllocator() {
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorInfo.physicalDevice = vkphysicaldevice;
	allocatorInfo.device = vkdevice;
	allocatorInfo.instance = vkinstance;

	VkResult result = vmaCreateAllocator(&allocatorInfo, &vmaallocator);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create VMA allocator! Error: " << result;
	}
}

VkSurfaceFormatKHR gVulkanRenderEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR gVulkanRenderEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& mode : availablePresentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D gVulkanRenderEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	gGLFWWindow* glfwwindow = static_cast<gGLFWWindow*>(appmanager->getWindow());
	int w, h;
	glfwGetFramebufferSize(glfwwindow->getGLFWwindow(), &w, &h);
	VkExtent2D actual = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
	actual.width = glm::clamp(actual.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actual.height = glm::clamp(actual.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return actual;
}

void gVulkanRenderEngine::createSwapchain() {
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkphysicaldevice, vksurface, &capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkphysicaldevice, vksurface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkphysicaldevice, vksurface, &formatCount, formats.data());

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkphysicaldevice, vksurface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkphysicaldevice, vksurface, &presentModeCount, presentModes.data());

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
	VkExtent2D extent = chooseSwapExtent(capabilities);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createinfo.surface = vksurface;
	createinfo.minImageCount = imageCount;
	createinfo.imageFormat = surfaceFormat.format;
	createinfo.imageColorSpace = surfaceFormat.colorSpace;
	createinfo.imageExtent = extent;
	createinfo.imageArrayLayers = 1;
	createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {graphicsqueuefamily, presentqueuefamily};
	if (graphicsqueuefamily != presentqueuefamily) {
		createinfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createinfo.queueFamilyIndexCount = 2;
		createinfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createinfo.preTransform = capabilities.currentTransform;
	createinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createinfo.presentMode = presentMode;
	createinfo.clipped = VK_TRUE;
	createinfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(vkdevice, &createinfo, nullptr, &vkswapchain);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create swapchain! Error: " << result;
		return;
	}

	vkGetSwapchainImagesKHR(vkdevice, vkswapchain, &imageCount, nullptr);
	swapchainimages.resize(imageCount);
	vkGetSwapchainImagesKHR(vkdevice, vkswapchain, &imageCount, swapchainimages.data());

	swapchainimageformat = surfaceFormat.format;
	swapchainextent = extent;
}

void gVulkanRenderEngine::createImageViews() {
	swapchainimageviews.resize(swapchainimages.size());
	for (size_t i = 0; i < swapchainimages.size(); i++) {
		VkImageViewCreateInfo viewinfo{};
		viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewinfo.image = swapchainimages[i];
		viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewinfo.format = swapchainimageformat;
		viewinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewinfo.subresourceRange.baseMipLevel = 0;
		viewinfo.subresourceRange.levelCount = 1;
		viewinfo.subresourceRange.baseArrayLayer = 0;
		viewinfo.subresourceRange.layerCount = 1;

		vkCreateImageView(vkdevice, &viewinfo, nullptr, &swapchainimageviews[i]);
	}
}

VkFormat gVulkanRenderEngine::findDepthFormat() {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat gVulkanRenderEngine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vkphysicaldevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
	}
	gLoge("Vulkan") << "Failed to find supported depth format!";
	return candidates[0];
}

void gVulkanRenderEngine::createRenderPass() {
	depthformat = findDepthFormat();

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainimageformat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthformat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef{};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	vkCreateRenderPass(vkdevice, &renderPassInfo, nullptr, &vkrenderpass);
}

void gVulkanRenderEngine::createDepthResources() {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = swapchainextent.width;
	imageInfo.extent.height = swapchainextent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = depthformat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(vmaallocator, &imageInfo, &allocInfo, &depthimage, &depthallocation, nullptr);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = depthimage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = depthformat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(vkdevice, &viewInfo, nullptr, &depthimageview);
}

void gVulkanRenderEngine::createFramebuffers() {
	swapchainframebuffers.resize(swapchainimageviews.size());
	for (size_t i = 0; i < swapchainimageviews.size(); i++) {
		std::array<VkImageView, 2> attachments = {swapchainimageviews[i], depthimageview};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = vkrenderpass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainextent.width;
		framebufferInfo.height = swapchainextent.height;
		framebufferInfo.layers = 1;

		vkCreateFramebuffer(vkdevice, &framebufferInfo, nullptr, &swapchainframebuffers[i]);
	}
}

void gVulkanRenderEngine::createCommandPool() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = graphicsqueuefamily;

	vkCreateCommandPool(vkdevice, &poolInfo, nullptr, &vkcommandpool);
}

void gVulkanRenderEngine::createCommandBuffers() {
	commandbuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vkcommandpool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandbuffers.size());

	vkAllocateCommandBuffers(vkdevice, &allocInfo, commandbuffers.data());
}

void gVulkanRenderEngine::createSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkCreateSemaphore(vkdevice, &semaphoreInfo, nullptr, &imageavailablesemaphores[i]);
		vkCreateSemaphore(vkdevice, &semaphoreInfo, nullptr, &renderfinishedsemaphores[i]);
		vkCreateFence(vkdevice, &fenceInfo, nullptr, &inflightfences[i]);
	}
}

void gVulkanRenderEngine::cleanupSwapchain() {
	if (depthimageview != VK_NULL_HANDLE) vkDestroyImageView(vkdevice, depthimageview, nullptr);
	if (depthimage != VK_NULL_HANDLE) vmaDestroyImage(vmaallocator, depthimage, depthallocation);
	depthimageview = VK_NULL_HANDLE;
	depthimage = VK_NULL_HANDLE;

	for (auto fb : swapchainframebuffers) {
		vkDestroyFramebuffer(vkdevice, fb, nullptr);
	}
	swapchainframebuffers.clear();

	for (auto iv : swapchainimageviews) {
		vkDestroyImageView(vkdevice, iv, nullptr);
	}
	swapchainimageviews.clear();

	if (vkrenderpass != VK_NULL_HANDLE) vkDestroyRenderPass(vkdevice, vkrenderpass, nullptr);
	vkrenderpass = VK_NULL_HANDLE;

	if (vkswapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(vkdevice, vkswapchain, nullptr);
	vkswapchain = VK_NULL_HANDLE;
}

void gVulkanRenderEngine::recreateSwapchain() {
	gLogw("Vulkan") << "recreateSwapchain() called";
	// Handle minimized window
	gGLFWWindow* glfwwindow = static_cast<gGLFWWindow*>(appmanager->getWindow());
	int w = 0, h = 0;
	glfwGetFramebufferSize(glfwwindow->getGLFWwindow(), &w, &h);
	while (w == 0 || h == 0) {
		glfwGetFramebufferSize(glfwwindow->getGLFWwindow(), &w, &h);
		glfwWaitEvents();
	}

	// Wait for GPU to finish before destroying pipelines
	vkDeviceWaitIdle(vkdevice);

	// Destroy cached pipelines — they reference the old render pass
	for (auto& entry : pipelinecache) {
		vkDestroyPipeline(vkdevice, entry.second, nullptr);
	}
	pipelinecache.clear();

	cleanupSwapchain();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	framebufferresized = false;
}

// ============================================================================
// SPIR-V and shader module helpers
// ============================================================================
std::string gVulkanRenderEngine::parseVkIdentifier(const char* source) {
	if (!source) return "";
	std::string s(source);
	// Strip leading/trailing whitespace and newlines
	size_t start = s.find("@vk:");
	if (start == std::string::npos) return "";
	start += 4; // skip "@vk:"
	size_t end = s.find_first_of(" \n\r\t", start);
	if (end == std::string::npos) end = s.size();
	return s.substr(start, end - start);
}

bool gVulkanRenderEngine::getSpirvData(const std::string& id, const char** outdata, size_t* outsize) {
	for (const auto& entry : spirventries) {
		if (!entry.id) break; // sentinel
		if (id == entry.id) {
			*outdata = entry.data;
			*outsize = entry.size;
			return true;
		}
	}
	return false;
}

VkShaderModule gVulkanRenderEngine::createShaderModule(const char* spirvdata, size_t spirvsize) {
	VkShaderModuleCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createinfo.codeSize = spirvsize;
	createinfo.pCode = reinterpret_cast<const uint32_t*>(spirvdata);

	VkShaderModule module;
	VkResult result = vkCreateShaderModule(vkdevice, &createinfo, nullptr, &module);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create shader module! Error: " << result;
		return VK_NULL_HANDLE;
	}
	return module;
}

// ============================================================================
// Pipeline infrastructure
// ============================================================================
void gVulkanRenderEngine::createDescriptorSetLayouts() {
	// Set 0: Scene + Lights UBOs
	VkDescriptorSetLayoutBinding bindings0[2] = {};
	bindings0[0].binding = 0;
	bindings0[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings0[0].descriptorCount = 1;
	bindings0[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings0[1].binding = 1;
	bindings0[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings0[1].descriptorCount = 1;
	bindings0[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutinfo0{};
	layoutinfo0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutinfo0.bindingCount = 2;
	layoutinfo0.pBindings = bindings0;
	vkCreateDescriptorSetLayout(vkdevice, &layoutinfo0, nullptr, &descriptorsetlayout0);

	// Set 1: Material UBO + textures
	VkDescriptorSetLayoutBinding bindings1[3] = {};
	bindings1[0].binding = 0;
	bindings1[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings1[0].descriptorCount = 1;
	bindings1[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings1[1].binding = 1;
	bindings1[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings1[1].descriptorCount = 1;
	bindings1[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings1[2].binding = 2;
	bindings1[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings1[2].descriptorCount = 1;
	bindings1[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutinfo1{};
	layoutinfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutinfo1.bindingCount = 3;
	layoutinfo1.pBindings = bindings1;
	vkCreateDescriptorSetLayout(vkdevice, &layoutinfo1, nullptr, &descriptorsetlayout1);
}

void gVulkanRenderEngine::createPipelineLayout() {
	VkDescriptorSetLayout setLayouts[] = {descriptorsetlayout0, descriptorsetlayout1};

	VkPushConstantRange pushConstRange{};
	pushConstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstRange.offset = 0;
	pushConstRange.size = PUSH_CONSTANT_SIZE;

	VkPipelineLayoutCreateInfo layoutinfo{};
	layoutinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutinfo.setLayoutCount = 2;
	layoutinfo.pSetLayouts = setLayouts;
	layoutinfo.pushConstantRangeCount = 1;
	layoutinfo.pPushConstantRanges = &pushConstRange;

	vkCreatePipelineLayout(vkdevice, &layoutinfo, nullptr, &pipelinelayout);
}

void gVulkanRenderEngine::createDescriptorPool() {
	// Scene descriptor pool: small, static (set 0 — Lights + Scene UBOs)
	{
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
		};

		VkDescriptorPoolCreateInfo poolinfo{};
		poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolinfo.maxSets = MAX_FRAMES_IN_FLIGHT;
		poolinfo.poolSizeCount = 1;
		poolinfo.pPoolSizes = poolSizes;

		vkCreateDescriptorPool(vkdevice, &poolinfo, nullptr, &scenedescriptorpool);
	}

	// Material descriptor pools: one per frame-in-flight, reset each frame (set 1 — Material UBO + 2 textures)
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_MATERIAL_DRAWS_PER_FRAME)},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(2 * MAX_MATERIAL_DRAWS_PER_FRAME)},
		};

		VkDescriptorPoolCreateInfo poolinfo{};
		poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolinfo.maxSets = MAX_MATERIAL_DRAWS_PER_FRAME;
		poolinfo.poolSizeCount = 2;
		poolinfo.pPoolSizes = poolSizes;

		vkCreateDescriptorPool(vkdevice, &poolinfo, nullptr, &materialdescriptorpools[i]);
	}
}

void gVulkanRenderEngine::createDescriptorSets() {
	// Allocate scene descriptor sets (set 0) per frame — these are static
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorSetAllocateInfo allocinfo{};
		allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocinfo.descriptorPool = scenedescriptorpool;
		allocinfo.descriptorSetCount = 1;
		allocinfo.pSetLayouts = &descriptorsetlayout0;
		vkAllocateDescriptorSets(vkdevice, &allocinfo, &scenedescriptorsets[i]);
	}
	// Material descriptor sets (set 1) are allocated per-draw from materialdescriptorpools[currentframe]
}

void gVulkanRenderEngine::createDummyResources() {
	// Create a 1x1 white dummy texture for unused sampler bindings
	VkImageCreateInfo imginfo{};
	imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imginfo.imageType = VK_IMAGE_TYPE_2D;
	imginfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imginfo.extent = {1, 1, 1};
	imginfo.mipLevels = 1;
	imginfo.arrayLayers = 1;
	imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imginfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocinfo{};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(vmaallocator, &imginfo, &allocinfo, &dummyimage, &dummyimagealloc, nullptr);

	// Transition dummy image to SHADER_READ_ONLY_OPTIMAL
	VkCommandBufferAllocateInfo cmdallocinfo{};
	cmdallocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdallocinfo.commandPool = vkcommandpool;
	cmdallocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdallocinfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(vkdevice, &cmdallocinfo, &cmd);

	VkCommandBufferBeginInfo begininfo{};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begininfo);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = dummyimage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitinfo{};
	submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitinfo.commandBufferCount = 1;
	submitinfo.pCommandBuffers = &cmd;
	vkQueueSubmit(vkgraphicsqueue, 1, &submitinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkgraphicsqueue);

	vkFreeCommandBuffers(vkdevice, vkcommandpool, 1, &cmd);

	VkImageViewCreateInfo viewinfo{};
	viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewinfo.image = dummyimage;
	viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewinfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewinfo.subresourceRange.levelCount = 1;
	viewinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(vkdevice, &viewinfo, nullptr, &dummyimageview);

	VkSamplerCreateInfo samplerinfo{};
	samplerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerinfo.magFilter = VK_FILTER_NEAREST;
	samplerinfo.minFilter = VK_FILTER_NEAREST;
	samplerinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vkCreateSampler(vkdevice, &samplerinfo, nullptr, &dummysampler);
}

bool gVulkanRenderEngine::updateSceneDescriptorSet(uint32_t frameindex) {
	VkWriteDescriptorSet writes[2] = {};

	// Binding 0: Lights UBO
	VkDescriptorBufferInfo lightsinfo{};
	if (lightsubohandle > 0 && lightsubohandle < bufferpool.size() && bufferpool[lightsubohandle].buffer != VK_NULL_HANDLE) {
		lightsinfo.buffer = bufferpool[lightsubohandle].buffer;
		lightsinfo.offset = 0;
		lightsinfo.range = bufferpool[lightsubohandle].size;
	} else {
		static bool logged = false;
		if (!logged) {
			gLogi("VkDraw") << "updateSceneDescriptor: lights UBO not ready, handle=" << lightsubohandle << " poolsize=" << bufferpool.size();
			logged = true;
		}
		return false;
	}

	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = scenedescriptorsets[frameindex];
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &lightsinfo;

	// Binding 1: Scene UBO
	VkDescriptorBufferInfo sceneinfo{};
	if (sceneubohandle > 0 && sceneubohandle < bufferpool.size() && bufferpool[sceneubohandle].buffer != VK_NULL_HANDLE) {
		sceneinfo.buffer = bufferpool[sceneubohandle].buffer;
		sceneinfo.offset = 0;
		sceneinfo.range = bufferpool[sceneubohandle].size;
	} else {
		static bool logged = false;
		if (!logged) {
			gLogi("VkDraw") << "updateSceneDescriptor: scene UBO not ready, handle=" << sceneubohandle << " poolsize=" << bufferpool.size();
			logged = true;
		}
		return false;
	}

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = scenedescriptorsets[frameindex];
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[1].pBufferInfo = &sceneinfo;

	vkUpdateDescriptorSets(vkdevice, 2, writes, 0, nullptr);

	// DIAGNOSTIC: Log Lights UBO content
	static int scenediagcount = 0;
	if (scenediagcount < 20) {
		auto& lslot = bufferpool[lightsubohandle];
		if (lslot.mapped) {
			int lightnum = *reinterpret_cast<int*>(lslot.mapped);
			int enabledlights = *reinterpret_cast<int*>(static_cast<uint8_t*>(lslot.mapped) + 4);
			float* gac = reinterpret_cast<float*>(static_cast<uint8_t*>(lslot.mapped) + 16);
			gLogi("VkDiag") << "Lights UBO: lightnum=" << lightnum
				<< " enabledlights=0x" << std::hex << enabledlights << std::dec
				<< " globalambient=(" << gac[0] << "," << gac[1] << "," << gac[2] << "," << gac[3] << ")"
				<< " bufsize=" << lslot.size;
			if (lightnum > 0) {
				// Log first light data (at offset 32)
				uint8_t* l0 = static_cast<uint8_t*>(lslot.mapped) + 32;
				int type = *reinterpret_cast<int*>(l0);
				float* pos = reinterpret_cast<float*>(l0 + 16);
				float* dir = reinterpret_cast<float*>(l0 + 32);
				gLogi("VkDiag") << "  Light[0]: type=" << type
					<< " pos=(" << pos[0] << "," << pos[1] << "," << pos[2] << ")"
					<< " dir=(" << dir[0] << "," << dir[1] << "," << dir[2] << ")";
			}
		}
		auto& sslot = bufferpool[sceneubohandle];
		if (sslot.mapped) {
			float* rc = reinterpret_cast<float*>(sslot.mapped);
			int* flags = reinterpret_cast<int*>(static_cast<uint8_t*>(sslot.mapped) + 96);
			gLogi("VkDiag") << "Scene UBO: renderColor=(" << rc[0] << "," << rc[1] << "," << rc[2] << "," << rc[3] << ")"
				<< " flags=" << *flags << " bufsize=" << sslot.size;
		}
		scenediagcount++;
	}

	return true;
}

VkDescriptorSet gVulkanRenderEngine::updateMaterialDescriptorSet(uint32_t frameindex) {
	// Allocate a fresh descriptor set from the per-frame material pool
	VkDescriptorSet matset = VK_NULL_HANDLE;
	if (materialdrawindex >= static_cast<uint32_t>(MAX_MATERIAL_DRAWS_PER_FRAME)) {
		gLoge("Vulkan") << "Material draw limit reached (" << static_cast<int>(MAX_MATERIAL_DRAWS_PER_FRAME) << ")";
		return VK_NULL_HANDLE;
	}

	VkDescriptorSetAllocateInfo allocinfo{};
	allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocinfo.descriptorPool = materialdescriptorpools[frameindex];
	allocinfo.descriptorSetCount = 1;
	allocinfo.pSetLayouts = &descriptorsetlayout1;
	VkResult allocresult = vkAllocateDescriptorSets(vkdevice, &allocinfo, &matset);
	if (allocresult != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to allocate material descriptor set! Error: " << allocresult;
		return VK_NULL_HANDLE;
	}

	// Upload material data to the ring buffer at the current draw index offset (aligned)
	VkDeviceSize ubooffset = materialdrawindex * materialubostride;
	if (materialubobuffers[frameindex].mapped) {
		memcpy(static_cast<uint8_t*>(materialubobuffers[frameindex].mapped) + ubooffset, &materialdata, sizeof(MaterialUBOData));
	}

	VkDescriptorBufferInfo matbufinfo{};
	matbufinfo.buffer = materialubobuffers[frameindex].buffer;
	matbufinfo.offset = ubooffset;
	matbufinfo.range = sizeof(MaterialUBOData);

	VkDescriptorImageInfo dummyimginfo{};
	dummyimginfo.sampler = dummysampler;
	dummyimginfo.imageView = dummyimageview;
	dummyimginfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Use bound textures if available, otherwise fall back to dummy
	VkDescriptorImageInfo texinfo0 = dummyimginfo;
	VkDescriptorImageInfo texinfo1 = dummyimginfo;
	if (boundtextures[0]) {
		VkImageView iv = boundtextures[0]->getImageView();
		VkSampler samp = boundtextures[0]->getSampler();
		if (iv != VK_NULL_HANDLE && samp != VK_NULL_HANDLE) {
			texinfo0.imageView = iv;
			texinfo0.sampler = samp;
		}
	}
	if (boundtextures[1]) {
		VkImageView iv = boundtextures[1]->getImageView();
		VkSampler samp = boundtextures[1]->getSampler();
		if (iv != VK_NULL_HANDLE && samp != VK_NULL_HANDLE) {
			texinfo1.imageView = iv;
			texinfo1.sampler = samp;
		}
	}

	VkWriteDescriptorSet writes[3] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = matset;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &matbufinfo;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = matset;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = &texinfo0;

	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].dstSet = matset;
	writes[2].dstBinding = 2;
	writes[2].descriptorCount = 1;
	writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[2].pImageInfo = &texinfo1;

	vkUpdateDescriptorSets(vkdevice, 3, writes, 0, nullptr);

	materialdrawindex++;
	return matset;
}

VkPrimitiveTopology gVulkanRenderEngine::glTopologyToVk(int drawMode) {
	switch (drawMode) {
		case GL_POINTS:         return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case GL_LINES:          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case GL_LINE_LOOP:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; // Vulkan has no line loop; use strip
		case GL_LINE_STRIP:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case GL_TRIANGLES:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case GL_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case GL_TRIANGLE_FAN:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		default:                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkPipeline gVulkanRenderEngine::getOrCreatePipeline(uint32_t program, VkPrimitiveTopology topology, bool depthtest, bool alphablend) {
	PipelineKey key{program, topology, depthtest, alphablend};
	auto it = pipelinecache.find(key);
	if (it != pipelinecache.end()) return it->second;

	if (program == 0 || program >= shaderprograms.size()) return VK_NULL_HANDLE;
	auto& sp = shaderprograms[program];
	if (sp.vertmodule == VK_NULL_HANDLE || sp.fragmodule == VK_NULL_HANDLE) return VK_NULL_HANDLE;

	// Shader stages
	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = sp.vertmodule;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = sp.fragmodule;
	stages[1].pName = "main";

	// Vertex input: determined by shader program's vertex layout
	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription gvertexAttrDescs[6] = {};
	VkVertexInputAttributeDescription imagequadAttrDescs[1] = {};

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDesc;

	if (sp.vertexlayout == VERTEX_LAYOUT_IMAGE_QUAD) {
		// Image quad: single vec4 attribute (pos.xy, tex.zw), 16 bytes
		bindingDesc.stride = 4 * sizeof(float);
		imagequadAttrDescs[0] = {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0};
		vertexInput.vertexAttributeDescriptionCount = 1;
		vertexInput.pVertexAttributeDescriptions = imagequadAttrDescs;
	} else {
		// gVertex layout: 6 attributes, 68 bytes
		bindingDesc.stride = sizeof(gVertex);
		gvertexAttrDescs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gVertex, position)};
		gvertexAttrDescs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gVertex, normal)};
		gvertexAttrDescs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gVertex, texcoords)};
		gvertexAttrDescs[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gVertex, tangent)};
		gvertexAttrDescs[4] = {4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gVertex, bitangent)};
		gvertexAttrDescs[5] = {5, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gVertex, color)};
		vertexInput.vertexAttributeDescriptionCount = 6;
		vertexInput.pVertexAttributeDescriptions = gvertexAttrDescs;
	}

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Dynamic state
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Viewport (dynamic)
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = depthtest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (alphablend) {
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	} else {
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &colorBlendAttachment;

	// Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelinelayout;
	pipelineInfo.renderPass = vkrenderpass;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines(vkdevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
	if (result != VK_SUCCESS) {
		gLoge("Vulkan") << "Failed to create graphics pipeline! Error: " << result;
		return VK_NULL_HANDLE;
	}

	pipelinecache[key] = pipeline;
	return pipeline;
}

// ============================================================================
// Buffer pool
// ============================================================================
uint32_t gVulkanRenderEngine::allocBufferSlot() {
	for (uint32_t i = 1; i < bufferpool.size(); i++) {
		if (!bufferpoolused[i]) {
			bufferpoolused[i] = true;
			bufferpool[i] = {};
			return i;
		}
	}
	bufferpool.push_back({});
	bufferpoolused.push_back(true);
	return static_cast<uint32_t>(bufferpool.size() - 1);
}

void gVulkanRenderEngine::freeBufferSlot(uint32_t handle) {
	if (handle > 0 && handle < bufferpool.size()) {
		if (bufferpool[handle].buffer != VK_NULL_HANDLE) {
			DeferredDeletion del;
			del.buffer = bufferpool[handle].buffer;
			del.allocation = bufferpool[handle].allocation;
			deferDeletion(del);
		}
		bufferpool[handle] = {};
		bufferpoolused[handle] = false;
	}
}

void gVulkanRenderEngine::deferDeletion(const DeferredDeletion& del) {
	deletionqueues[currentframe].push_back(del);
}

uint32_t gVulkanRenderEngine::allocVAOSlot() {
	for (uint32_t i = 1; i < vaopool.size(); i++) {
		if (!vaopoolused[i]) {
			vaopoolused[i] = true;
			vaopool[i] = {};
			return i;
		}
	}
	vaopool.push_back({});
	vaopoolused.push_back(true);
	return static_cast<uint32_t>(vaopool.size() - 1);
}

void gVulkanRenderEngine::freeVAOSlot(uint32_t handle) {
	if (handle > 0 && handle < vaopool.size()) {
		vaopool[handle] = {};
		vaopoolused[handle] = false;
	}
}

// ============================================================================
// Dynamic draw pool (per-frame, for 2D primitives)
// ============================================================================
gVulkanRenderEngine::DynDrawSlot gVulkanRenderEngine::acquireDynDrawSlot() {
	uint32_t fi = currentframe;
	uint32_t idx = dyndrawindex[fi];

	if (idx >= dyndrawpool[fi].size()) {
		DynDrawSlot slot;
		slot.vaoslot = allocVAOSlot();
		slot.vboslot = allocBufferSlot();
		slot.eboslot = allocBufferSlot();
		dyndrawpool[fi].push_back(slot);
	}

	dyndrawindex[fi] = idx + 1;
	return dyndrawpool[fi][idx];
}

void gVulkanRenderEngine::resetDynDrawPool(uint32_t frameindex) {
	dyndrawindex[frameindex] = 0;
}

void gVulkanRenderEngine::ensureBufferCapacity(uint32_t bufferSlot,
                                                VkDeviceSize requiredSize,
                                                VkBufferUsageFlagBits usage) {
	auto& slot = bufferpool[bufferSlot];
	if (slot.buffer != VK_NULL_HANDLE && slot.size >= requiredSize) {
		return;
	}

	if (slot.buffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(vmaallocator, slot.buffer, slot.allocation);
		slot = {};
	}

	VkDeviceSize allocSize = requiredSize + (requiredSize / 2);
	if (allocSize < 256) allocSize = 256;

	VkBufferCreateInfo bufinfo{};
	bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufinfo.size = allocSize;
	bufinfo.usage = usage;
	bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocinfo{};
	allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(vmaallocator, &bufinfo, &allocinfo,
		&slot.buffer, &slot.allocation, &allocationInfo);
	slot.size = allocSize;
	slot.mapped = allocationInfo.pMappedData;
}

uint32_t gVulkanRenderEngine::addShaderProgram(const VulkanShaderProgram& sp) {
	VulkanShaderProgram program = sp;
	// Detect vertex layout from the vertex shader identifier
	if (program.vertid == "image_vert"
			|| program.vertid == "font_vert"
			|| program.vertid == "fbo_vert"
			|| program.vertid == "ssao_vert"
			|| program.vertid == "brdf_vert") {
		program.vertexlayout = VERTEX_LAYOUT_IMAGE_QUAD;
	}
	shaderprograms.push_back(program);
	return static_cast<uint32_t>(shaderprograms.size() - 1);
}

void gVulkanRenderEngine::setUBOBindingHandle(int bindingPoint, uint32_t bufferSlot) {
	gLogi("VkDraw") << "setUBOBindingHandle: bindingPoint=" << bindingPoint << " bufferSlot=" << bufferSlot;
	if (bindingPoint == 0) {
		lightsubohandle = bufferSlot; // binding 0 = Lights UBO
	} else if (bindingPoint == 1) {
		sceneubohandle = bufferSlot;  // binding 1 = Scene UBO
	}
}

void gVulkanRenderEngine::bindTexture(int slot, gTextureVk* tex) {
	if (slot >= 0 && slot < 2) {
		boundtextures[slot] = tex;
	}
}

void gVulkanRenderEngine::unbindTexture(int slot) {
	if (slot >= 0 && slot < 2) {
		boundtextures[slot] = nullptr;
	}
}

// ============================================================================
// Frame lifecycle
// ============================================================================
void gVulkanRenderEngine::beginFrame() {
	if (vkdevice == VK_NULL_HANDLE) return;

	// DIAGNOSTIC: Force full GPU serialization to test if flickering is a sync issue
	// If flickering stops with this line, the cause is a cross-frame resource race
	vkDeviceWaitIdle(vkdevice);

	vkWaitForFences(vkdevice, 1, &inflightfences[currentframe], VK_TRUE, UINT64_MAX);

	// Process deferred deletions — GPU finished with this frame slot's previous work
	for (auto& del : deletionqueues[currentframe]) {
		if (del.imageview != VK_NULL_HANDLE) {
			vkDestroyImageView(vkdevice, del.imageview, nullptr);
		}
		if (del.sampler != VK_NULL_HANDLE) {
			vkDestroySampler(vkdevice, del.sampler, nullptr);
		}
		if (del.image != VK_NULL_HANDLE) {
			vmaDestroyImage(vmaallocator, del.image, del.allocation);
		} else if (del.buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(vmaallocator, del.buffer, del.allocation);
		}
	}
	deletionqueues[currentframe].clear();

	// Reset per-frame pools
	resetDynDrawPool(currentframe);
	rendercommands[currentframe].clear();

	VkResult result = vkAcquireNextImageKHR(vkdevice, vkswapchain, UINT64_MAX,
		imageavailablesemaphores[currentframe], VK_NULL_HANDLE, &currentimageindex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	}

	vkResetFences(vkdevice, 1, &inflightfences[currentframe]);
	vkResetCommandBuffer(commandbuffers[currentframe], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(commandbuffers[currentframe], &beginInfo);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = clearcolorvalue;
	clearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = vkrenderpass;
	renderPassInfo.framebuffer = swapchainframebuffers[currentimageindex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapchainextent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandbuffers[currentframe], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set dynamic viewport and scissor
	// Use negative height to flip Y-axis (Vulkan Y is top-down, OpenGL Y is bottom-up)
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(swapchainextent.height);
	viewport.width = static_cast<float>(swapchainextent.width);
	viewport.height = -static_cast<float>(swapchainextent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandbuffers[currentframe], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchainextent;
	vkCmdSetScissor(commandbuffers[currentframe], 0, 1, &scissor);

	// Reset this frame's material descriptor pool — sets from the previous use of this frame index are freed
	vkResetDescriptorPool(vkdevice, materialdescriptorpools[currentframe], 0);
	materialdrawindex = 0;

	frameinprogress = true;
}

void gVulkanRenderEngine::flushRenderCommands() {
	auto& commands = rendercommands[currentframe];
	if (commands.empty()) return;

	static int flushlogcount = 0;
	static int prevdrawcount = 0;
	static bool everHadCamera = false;
	{
		int drawcount = 0, vpcount = 0, clearcount = 0;
		int dyncount = 0, staticcount = 0;
		int depthcount = 0;
		bool hasNonIdentityView = false;
		bool hasIdentityView = false;
		for (auto& c : commands) {
			if (c.type == RenderCommand::CMD_DRAW) {
				drawcount++;
				if (c.isdynamic) dyncount++; else staticcount++;
				if (c.depthtestenabled) depthcount++;
				if (c.materialdata.viewmatrix == glm::mat4(1.0f)) hasIdentityView = true;
				else hasNonIdentityView = true;
			} else if (c.type == RenderCommand::CMD_SET_VIEWPORT) vpcount++;
			else clearcount++;
		}
		bool stateChanged = (drawcount != prevdrawcount) || (hasNonIdentityView && !everHadCamera);
		if (hasNonIdentityView) everHadCamera = true;
		prevdrawcount = drawcount;
		if (flushlogcount < 10 || stateChanged || (everHadCamera && flushlogcount < 60)) {
			gLogi("VkFlush") << "Frame " << currentframe << ": " << commands.size() << " commands ("
				<< drawcount << " draws [" << dyncount << " dyn, " << staticcount << " static, " << depthcount << " depth], "
				<< vpcount << " viewports, " << clearcount << " clears)"
				<< " renderpassno=" << renderpassno << " renderpassnum=" << renderpassnum
				<< " views=[identity:" << hasIdentityView << " camera:" << hasNonIdentityView << "]";
			flushlogcount++;
		}
	}

	VkCommandBuffer cmd = commandbuffers[currentframe];
	bool scenedescriptorupdated = false;
	VkDescriptorSet sceneset = VK_NULL_HANDLE;

	for (auto& rc : commands) {
		switch (rc.type) {
		case RenderCommand::CMD_SET_VIEWPORT: {
			vkCmdSetViewport(cmd, 0, 1, &rc.viewport);
			vkCmdSetScissor(cmd, 0, 1, &rc.scissor);
			break;
		}
		case RenderCommand::CMD_CLEAR_SCREEN: {
			VkClearAttachment attachments[2] = {};
			int count = 0;
			if (rc.clearcolor) {
				attachments[count].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				attachments[count].colorAttachment = 0;
				attachments[count].clearValue.color = rc.clearcolorvalue;
				count++;
			}
			if (rc.cleardepth) {
				attachments[count].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				attachments[count].clearValue.depthStencil = {1.0f, 0};
				count++;
			}
			if (count > 0) {
				VkClearRect rect{};
				rect.rect.offset = {0, 0};
				rect.rect.extent = swapchainextent;
				rect.baseArrayLayer = 0;
				rect.layerCount = 1;
				vkCmdClearAttachments(cmd, count, attachments, 1, &rect);
			}
			break;
		}
		case RenderCommand::CMD_DRAW: {
			// Debug label for RenderDoc
			{
				std::string label = "Draw:";
				if (rc.shaderprogram < shaderprograms.size()) {
					label += " " + shaderprograms[rc.shaderprogram].vertid + "/" + shaderprograms[rc.shaderprogram].fragid;
				}
				label += rc.isdynamic ? " [dyn]" : " [static]";
				label += rc.depthtestenabled ? " depth" : "";
				label += rc.alphablendingenabled ? " blend" : "";
				VkDebugUtilsLabelEXT labelInfo{};
				labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				labelInfo.pLabelName = label.c_str();
				labelInfo.color[0] = rc.depthtestenabled ? 0.0f : 1.0f;
				labelInfo.color[1] = 1.0f;
				labelInfo.color[2] = rc.isdynamic ? 1.0f : 0.0f;
				labelInfo.color[3] = 1.0f;
				static auto vkCmdBeginLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(vkinstance, "vkCmdBeginDebugUtilsLabelEXT");
				static auto vkCmdEndLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(vkinstance, "vkCmdEndDebugUtilsLabelEXT");
				if (vkCmdBeginLabel) vkCmdBeginLabel(cmd, &labelInfo);
			}

			// Get or create pipeline
			VkPipeline pipeline = getOrCreatePipeline(rc.shaderprogram, rc.topology, rc.depthtestenabled, rc.alphablendingenabled);
			if (pipeline == VK_NULL_HANDLE) {
				static auto vkCmdEndLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(vkinstance, "vkCmdEndDebugUtilsLabelEXT");
				if (vkCmdEndLabel) vkCmdEndLabel(cmd);
				continue;
			}

			// Update scene descriptor set once per flush
			if (!scenedescriptorupdated) {
				if (!updateSceneDescriptorSet(currentframe)) continue;
				sceneset = scenedescriptorsets[currentframe];
				scenedescriptorupdated = true;
			}

			// Allocate material descriptor set for this draw
			if (materialdrawindex >= static_cast<uint32_t>(MAX_MATERIAL_DRAWS_PER_FRAME)) continue;

			VkDescriptorSet matset = VK_NULL_HANDLE;
			VkDescriptorSetAllocateInfo allocinfo{};
			allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocinfo.descriptorPool = materialdescriptorpools[currentframe];
			allocinfo.descriptorSetCount = 1;
			allocinfo.pSetLayouts = &descriptorsetlayout1;
			if (vkAllocateDescriptorSets(vkdevice, &allocinfo, &matset) != VK_SUCCESS) continue;

			// Upload material data
			VkDeviceSize ubooffset = materialdrawindex * materialubostride;
			if (materialubobuffers[currentframe].mapped) {
				memcpy(static_cast<uint8_t*>(materialubobuffers[currentframe].mapped) + ubooffset,
					&rc.materialdata, sizeof(MaterialUBOData));
			}

			// Build descriptor writes
			VkDescriptorBufferInfo matbufinfo{};
			matbufinfo.buffer = materialubobuffers[currentframe].buffer;
			matbufinfo.offset = ubooffset;
			matbufinfo.range = sizeof(MaterialUBOData);

			VkDescriptorImageInfo dummyimginfo{};
			dummyimginfo.sampler = dummysampler;
			dummyimginfo.imageView = dummyimageview;
			dummyimginfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo texinfo0 = dummyimginfo;
			VkDescriptorImageInfo texinfo1 = dummyimginfo;
			if (rc.diffuseview != VK_NULL_HANDLE && rc.diffusesampler != VK_NULL_HANDLE) {
				texinfo0.imageView = rc.diffuseview;
				texinfo0.sampler = rc.diffusesampler;
			}
			if (rc.specularview != VK_NULL_HANDLE && rc.specularsampler != VK_NULL_HANDLE) {
				texinfo1.imageView = rc.specularview;
				texinfo1.sampler = rc.specularsampler;
			}

			VkWriteDescriptorSet writes[3] = {};
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = matset;
			writes[0].dstBinding = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].pBufferInfo = &matbufinfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = matset;
			writes[1].dstBinding = 1;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].pImageInfo = &texinfo0;

			writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[2].dstSet = matset;
			writes[2].dstBinding = 2;
			writes[2].descriptorCount = 1;
			writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[2].pImageInfo = &texinfo1;

			vkUpdateDescriptorSets(vkdevice, 3, writes, 0, nullptr);
			materialdrawindex++;

			// Bind pipeline and descriptors
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			VkDescriptorSet sets[] = {sceneset, matset};
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 2, sets, 0, nullptr);

			// Push constants
			vkCmdPushConstants(cmd, pipelinelayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, PUSH_CONSTANT_SIZE, rc.pushconstants);

			// Bind geometry and draw
			if (rc.isdynamic) {
				// Allocate pool slot and upload vertex data
				auto dynslot = acquireDynDrawSlot();
				ensureBufferCapacity(dynslot.vboslot, rc.vertexdata.size(),
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
				auto& vbuf = bufferpool[dynslot.vboslot];
				memcpy(vbuf.mapped, rc.vertexdata.data(), rc.vertexdata.size());

				VkBuffer vertbufs[] = {vbuf.buffer};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(cmd, 0, 1, vertbufs, offsets);

				if (rc.drawtype == RenderCommand::DRAW_INDEXED && !rc.indexdata.empty()) {
					ensureBufferCapacity(dynslot.eboslot, rc.indexdata.size(),
						VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
					auto& ebuf = bufferpool[dynslot.eboslot];
					memcpy(ebuf.mapped, rc.indexdata.data(), rc.indexdata.size());
					vkCmdBindIndexBuffer(cmd, ebuf.buffer, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(cmd, rc.indexcount, 1, 0, 0, 0);
				} else {
					vkCmdDraw(cmd, rc.vertexcount, 1, 0, 0);
				}
			} else {
				// Static geometry — use existing GPU buffers
				bool vbobound = false;
				if (rc.staticvboslot > 0 && rc.staticvboslot < bufferpool.size()
					&& bufferpool[rc.staticvboslot].buffer != VK_NULL_HANDLE) {
					VkBuffer vertbufs[] = {bufferpool[rc.staticvboslot].buffer};
					VkDeviceSize offsets[] = {0};
					vkCmdBindVertexBuffers(cmd, 0, 1, vertbufs, offsets);
					vbobound = true;
				}

				if (!vbobound) {
					static int staticwarncount = 0;
					if (staticwarncount < 10) {
						gLogi("VkFlush") << "Static draw: no VBO! vboslot=" << rc.staticvboslot
							<< " program=" << rc.shaderprogram
							<< " type=" << (rc.drawtype == RenderCommand::DRAW_INDEXED ? "indexed" : "arrays");
						staticwarncount++;
					}
				}

				if (rc.drawtype == RenderCommand::DRAW_INDEXED) {
					if (rc.staticeboslot > 0 && rc.staticeboslot < bufferpool.size()
						&& bufferpool[rc.staticeboslot].buffer != VK_NULL_HANDLE) {
						vkCmdBindIndexBuffer(cmd, bufferpool[rc.staticeboslot].buffer, 0, VK_INDEX_TYPE_UINT32);
					}
					vkCmdDrawIndexed(cmd, rc.indexcount, 1, 0, 0, 0);
				} else {
					vkCmdDraw(cmd, rc.vertexcount, 1, 0, 0);
				}
			}
			{
				static auto vkCmdEndLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(vkinstance, "vkCmdEndDebugUtilsLabelEXT");
				if (vkCmdEndLabel) vkCmdEndLabel(cmd);
			}
			break;
		}
		}
	}

	commands.clear();
}

void gVulkanRenderEngine::endFrame() {
	if (!frameinprogress || vkdevice == VK_NULL_HANDLE) return;

	flushRenderCommands();

	vkCmdEndRenderPass(commandbuffers[currentframe]);
	vkEndCommandBuffer(commandbuffers[currentframe]);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {imageavailablesemaphores[currentframe]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandbuffers[currentframe];

	VkSemaphore signalSemaphores[] = {renderfinishedsemaphores[currentframe]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult submitResult = vkQueueSubmit(vkgraphicsqueue, 1, &submitInfo, inflightfences[currentframe]);
	if (submitResult != VK_SUCCESS) {
		gLoge("Vulkan") << "vkQueueSubmit failed! Error: " << submitResult;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {vkswapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &currentimageindex;

	VkResult result = vkQueuePresentKHR(vkpresentqueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferresized) {
		gLogw("Vulkan") << "Present triggered swapchain recreate: result=" << result
			<< " (OUT_OF_DATE=" << VK_ERROR_OUT_OF_DATE_KHR
			<< " SUBOPTIMAL=" << VK_SUBOPTIMAL_KHR
			<< ") resized=" << framebufferresized;
		recreateSwapchain();
	}

	currentframe = (currentframe + 1) % MAX_FRAMES_IN_FLIGHT;
	frameinprogress = false;
}

// ============================================================================
// Clear
// ============================================================================
void gVulkanRenderEngine::clear() {
	// In Vulkan, clearing happens via the render pass load op (VK_ATTACHMENT_LOAD_OP_CLEAR).
	// The clear color is set via clearColor() and applied when beginFrame() starts the render pass.
}

void gVulkanRenderEngine::clearColor(int r, int g, int b, int a) {
	clearcolorvalue = {{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}};
}

void gVulkanRenderEngine::clearColor(gColor color) {
	clearcolorvalue = {{color.r, color.g, color.b, color.a}};
}

// ============================================================================
// Depth / Alpha / State -- Stubs (Phase 2 will implement with pipelines)
// ============================================================================
void gVulkanRenderEngine::enableDepthTest() { isdepthtestenabled = true; }
void gVulkanRenderEngine::enableDepthTest(int depthTestType) { isdepthtestenabled = true; depthtesttype = depthTestType; }
void gVulkanRenderEngine::setDepthTestFunc(int depthTestType) { depthtesttype = depthTestType; }
void gVulkanRenderEngine::disableDepthTest() { isdepthtestenabled = false; }
bool gVulkanRenderEngine::isDepthTestEnabled() { return isdepthtestenabled; }
int gVulkanRenderEngine::getDepthTestType() { return depthtesttype; }
void gVulkanRenderEngine::enableAlphaBlending() { isalphablendingenabled = true; }
void gVulkanRenderEngine::disableAlphaBlending() { isalphablendingenabled = false; }
bool gVulkanRenderEngine::isAlphaBlendingEnabled() { return isalphablendingenabled; }
void gVulkanRenderEngine::enableAlphaTest() { isalphatestenabled = true; }
void gVulkanRenderEngine::disableAlphaTest() { isalphatestenabled = false; }
bool gVulkanRenderEngine::isAlphaTestEnabled() { return isalphatestenabled; }

void gVulkanRenderEngine::drawArrays(int drawMode, int count) {
	if (!frameinprogress || vkdevice == VK_NULL_HANDLE || activeshaderprogram == 0) return;
	if (currentvao == 0 || currentvao >= vaopool.size()) return;
	if (renderpassnum > 1 && renderpassno == 0) return; // Skip shadowmap pass — off-screen FBO not yet supported

	RenderCommand rc;
	rc.type = RenderCommand::CMD_DRAW;
	rc.drawtype = RenderCommand::DRAW_ARRAYS;
	rc.vertexcount = count;
	rc.shaderprogram = activeshaderprogram;
	rc.topology = glTopologyToVk(drawMode);
	rc.depthtestenabled = isdepthtestenabled;
	rc.alphablendingenabled = isalphablendingenabled;

	// Snapshot push constants with Vulkan depth correction
	memcpy(rc.pushconstants, pushconstantdata, PUSH_CONSTANT_SIZE);
	glm::mat4* proj = reinterpret_cast<glm::mat4*>(rc.pushconstants + 64);
	for (int j = 0; j < 4; j++) {
		(*proj)[j][2] = 0.5f * ((*proj)[j][2] + (*proj)[j][3]);
	}

	// Snapshot material
	rc.materialdata = materialdata;
	rc.materialdata.rendercolor = glm::vec4(rendercolor->r, rendercolor->g, rendercolor->b, rendercolor->a);
	rc.materialdata.viewmatrix = viewmatrix;
	rc.materialdata.viewpos = glm::vec4(cameraposition, 0.0f);

	// Snapshot textures
	if (boundtextures[0]) {
		rc.diffuseview = boundtextures[0]->getImageView();
		rc.diffusesampler = boundtextures[0]->getSampler();
	}
	if (boundtextures[1]) {
		rc.specularview = boundtextures[1]->getImageView();
		rc.specularsampler = boundtextures[1]->getSampler();
	}

	// Snapshot geometry
	auto& vao = vaopool[currentvao];
	if (vao.isdynamic && vao.cachedvertexptr && vao.cachedvertexsize > 0) {
		rc.isdynamic = true;
		rc.vertexdata.assign(vao.cachedvertexptr, vao.cachedvertexptr + vao.cachedvertexsize);
		rc.vertexstride = vao.vertexstride;
	} else {
		rc.isdynamic = false;
		rc.staticvboslot = vao.vbo;
		rc.staticvertexstride = vao.vertexstride;
	}

	rendercommands[currentframe].push_back(std::move(rc));
}

void gVulkanRenderEngine::drawElements(int drawMode, int count) {
	if (!frameinprogress || vkdevice == VK_NULL_HANDLE || activeshaderprogram == 0) return;
	if (currentvao == 0 || currentvao >= vaopool.size()) return;
	if (renderpassnum > 1 && renderpassno == 0) return; // Skip shadowmap pass — off-screen FBO not yet supported

	RenderCommand rc;
	rc.type = RenderCommand::CMD_DRAW;
	rc.drawtype = RenderCommand::DRAW_INDEXED;
	rc.indexcount = count;
	rc.shaderprogram = activeshaderprogram;
	rc.topology = glTopologyToVk(drawMode);
	rc.depthtestenabled = isdepthtestenabled;
	rc.alphablendingenabled = isalphablendingenabled;

	// Snapshot push constants with Vulkan depth correction
	memcpy(rc.pushconstants, pushconstantdata, PUSH_CONSTANT_SIZE);
	glm::mat4* proj = reinterpret_cast<glm::mat4*>(rc.pushconstants + 64);
	for (int j = 0; j < 4; j++) {
		(*proj)[j][2] = 0.5f * ((*proj)[j][2] + (*proj)[j][3]);
	}

	// Snapshot material
	rc.materialdata = materialdata;
	rc.materialdata.rendercolor = glm::vec4(rendercolor->r, rendercolor->g, rendercolor->b, rendercolor->a);
	rc.materialdata.viewmatrix = viewmatrix;
	rc.materialdata.viewpos = glm::vec4(cameraposition, 0.0f);

	// Snapshot textures
	if (boundtextures[0]) {
		rc.diffuseview = boundtextures[0]->getImageView();
		rc.diffusesampler = boundtextures[0]->getSampler();
	}
	if (boundtextures[1]) {
		rc.specularview = boundtextures[1]->getImageView();
		rc.specularsampler = boundtextures[1]->getSampler();
	}

	// Snapshot geometry
	auto& vao = vaopool[currentvao];
	if (vao.isdynamic && vao.cachedvertexptr && vao.cachedvertexsize > 0) {
		rc.isdynamic = true;
		rc.vertexdata.assign(vao.cachedvertexptr, vao.cachedvertexptr + vao.cachedvertexsize);
		rc.vertexstride = vao.vertexstride;
		if (vao.cachedindexptr && vao.cachedindexsize > 0) {
			rc.indexdata.assign(vao.cachedindexptr, vao.cachedindexptr + vao.cachedindexsize);
		}
	} else {
		rc.isdynamic = false;
		rc.staticvboslot = vao.vbo;
		rc.staticeboslot = vao.ebo;
		rc.staticvertexstride = vao.vertexstride;
	}

	rendercommands[currentframe].push_back(std::move(rc));
}

void gVulkanRenderEngine::setViewport(int x, int y, int w, int h) {
	if (!frameinprogress) return;
	if (renderpassnum > 1 && renderpassno == 0) return;

	RenderCommand rc;
	rc.type = RenderCommand::CMD_SET_VIEWPORT;
	rc.viewport.x = static_cast<float>(x);
	rc.viewport.y = static_cast<float>(y + h);
	rc.viewport.width = static_cast<float>(w);
	rc.viewport.height = -static_cast<float>(h);
	rc.viewport.minDepth = 0.0f;
	rc.viewport.maxDepth = 1.0f;
	rc.scissor.offset = {x, y};
	rc.scissor.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};

	rendercommands[currentframe].push_back(std::move(rc));
}

// ============================================================================
// Post-process -- Stubs (Phase 4)
// ============================================================================
void gVulkanRenderEngine::clearScreen(bool color, bool depth) {
	if (!frameinprogress) return;
	if (!color && !depth) return;
	if (renderpassnum > 1 && renderpassno == 0) return;

	RenderCommand rc;
	rc.type = RenderCommand::CMD_CLEAR_SCREEN;
	rc.clearcolor = color;
	rc.cleardepth = depth;
	rc.clearcolorvalue = clearcolorvalue;

	rendercommands[currentframe].push_back(std::move(rc));
}
void gVulkanRenderEngine::bindDefaultFramebuffer() {}

// ============================================================================
// Depth / Skybox
// ============================================================================
void gVulkanRenderEngine::enableDepthTestEqual() {}
void gVulkanRenderEngine::enableCubeMap() {}

// ============================================================================
// RenderObject
// ============================================================================
void gVulkanRenderEngine::pushMatrix() {}
void gVulkanRenderEngine::popMatrix() {}

// ============================================================================
// Screenshot -- Stubs (Phase 3)
// ============================================================================
void gVulkanRenderEngine::takeScreenshot(gImage& img, int x, int y, int w, int h) {
	// TODO: Copy from swapchain image to staging buffer
}

void gVulkanRenderEngine::takeScreenshot(gImage& img) {
	takeScreenshot(img, 0, 0, swapchainextent.width, swapchainextent.height);
}

// ============================================================================
// Utility
// ============================================================================
void gVulkanRenderEngine::updatePackUnpackAlignment(int i) {
	// No-op in Vulkan (alignment is handled explicitly)
}

#endif // GLIST_VULKAN
