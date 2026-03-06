/*
 * gVulkanRenderEngine.h
 *
 *  Created on: 2026-02-28
 *      Author: Metehan Gezer
 */

#pragma once

#ifndef CORE_GVULKANRENDERENGINE_H
#define CORE_GVULKANRENDERENGINE_H

#if defined(GLIST_VULKAN)

#include "gRenderer.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <unordered_map>

class gGLFWWindow;
class gTextureVk;

class gVulkanRenderEngine : public gRenderer {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	gVulkanRenderEngine() = default;
	~gVulkanRenderEngine() override;

	void beginFrame() override;
	void endFrame() override;

	void clear() override;
	void clearColor(int r, int g, int b, int a = 255) override;
	void clearColor(gColor color) override;

	void takeScreenshot(gImage& img, int x, int y, int width, int height) override;
	void takeScreenshot(gImage& img) override;

	void enableDepthTest() override;
	void enableDepthTest(int depthTestType) override;
	void setDepthTestFunc(int depthTestType) override;
	void disableDepthTest() override;
	bool isDepthTestEnabled() override;
	int getDepthTestType() override;

	void enableAlphaBlending() override;
	void disableAlphaBlending() override;
	bool isAlphaBlendingEnabled() override;
	void enableAlphaTest() override;
	void disableAlphaTest() override;
	bool isAlphaTestEnabled() override;

	/* -------------- Drawing ------------- */
	void drawArrays(int drawMode, int count) override;
	void drawElements(int drawMode, int count) override;
	void setViewport(int x, int y, int width, int height) override;

	/* ------------ Screen / Framebuffer ------------- */
	void clearScreen(bool color, bool depth) override;
	void bindDefaultFramebuffer() override;

	/* -------------- Depth / Skybox --------------- */
	void enableDepthTestEqual() override;
	void enableCubeMap() override;

	/* ---------------- gRenderObject ---------------- */
	void pushMatrix() override;
	void popMatrix() override;

	void cleanup();

	// ---- Public types (used by Vulkan handle classes) ----
	struct VulkanBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		void* mapped = nullptr; // for host-visible buffers
	};

	struct VulkanVAOState {
		uint32_t vbo = 0;
		uint32_t ebo = 0;
		int vertexstride = 0;
		bool hasindices = false;
		// Dynamic VBO support: cached CPU-side data pointers (set by gVboVk::bind)
		bool isdynamic = false;
		const uint8_t* cachedvertexptr = nullptr;
		size_t cachedvertexsize = 0;
		const uint8_t* cachedindexptr = nullptr;
		size_t cachedindexsize = 0;
	};

	enum VertexLayoutType {
		VERTEX_LAYOUT_GVERTEX = 0,    // 6 attributes, 68 bytes (gVertex)
		VERTEX_LAYOUT_IMAGE_QUAD = 1, // 1 vec4 attribute, 16 bytes
	};

	struct VulkanShaderProgram {
		VkShaderModule vertmodule = VK_NULL_HANDLE;
		VkShaderModule fragmodule = VK_NULL_HANDLE;
		std::string vertid;
		std::string fragid;
		VertexLayoutType vertexlayout = VERTEX_LAYOUT_GVERTEX;
	};

	struct MaterialUBOData {
		glm::vec4 ambient = glm::vec4(1.0f);
		glm::vec4 diffuse = glm::vec4(1.0f);
		glm::vec4 specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		float shininess = 32.0f;
		int usediffusemap = 0;
		int usespecularmap = 0;
		int usenormalmap = 0;
		glm::vec4 rendercolor = glm::vec4(1.0f);
		glm::mat4 viewmatrix = glm::mat4(1.0f);
		glm::vec4 viewpos = glm::vec4(0.0f);
	};

	struct DeferredDeletion {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkImageView imageview = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
	};

	static constexpr uint32_t PUSH_CONSTANT_SIZE = 128;

	// ---- Collected render commands (deferred dispatch) ----
	struct RenderCommand {
		enum Type : uint8_t { CMD_DRAW, CMD_SET_VIEWPORT, CMD_CLEAR_SCREEN };
		enum DrawType : uint8_t { DRAW_ARRAYS, DRAW_INDEXED };
		Type type = CMD_DRAW;

		// CMD_DRAW fields
		DrawType drawtype = DRAW_ARRAYS;
		int vertexcount = 0;
		int indexcount = 0;
		uint32_t shaderprogram = 0;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool depthtestenabled = false;
		bool alphablendingenabled = false;
		uint8_t pushconstants[PUSH_CONSTANT_SIZE] = {};
		MaterialUBOData materialdata;
		VkImageView diffuseview = VK_NULL_HANDLE;
		VkSampler diffusesampler = VK_NULL_HANDLE;
		VkImageView specularview = VK_NULL_HANDLE;
		VkSampler specularsampler = VK_NULL_HANDLE;
		// Static geometry (existing GPU buffer)
		bool isdynamic = false;
		uint32_t staticvboslot = 0;
		uint32_t staticeboslot = 0;
		int staticvertexstride = 0;
		// Dynamic geometry (CPU-side copy)
		std::vector<uint8_t> vertexdata;
		int vertexstride = 0;
		std::vector<uint8_t> indexdata;

		// CMD_SET_VIEWPORT fields
		VkViewport viewport = {};
		VkRect2D scissor = {};

		// CMD_CLEAR_SCREEN fields
		bool clearcolor = false;
		bool cleardepth = false;
		VkClearColorValue clearcolorvalue = {};
	};

	void flushRenderCommands();

	// ---- Dynamic draw pool (per-frame, for 2D primitives) ----
	struct DynDrawSlot {
		uint32_t vaoslot = 0;
		uint32_t vboslot = 0;
		uint32_t eboslot = 0;
	};
	DynDrawSlot acquireDynDrawSlot();
	void resetDynDrawPool(uint32_t frameindex);
	void ensureBufferCapacity(uint32_t bufferSlot, VkDeviceSize requiredSize,
	                          VkBufferUsageFlagBits usage);

	// ---- Uniform location encoding constants ----
	static constexpr uint32_t ULOC_TARGET_MASK  = 0xF0000000;
	static constexpr uint32_t ULOC_OFFSET_MASK  = 0x0FFFFFFF;
	static constexpr uint32_t ULOC_PUSH_CONST   = 0x00000000;
	static constexpr uint32_t ULOC_MATERIAL      = 0x10000000;
	static constexpr uint32_t ULOC_IGNORE        = 0xF0000000;

	// ---- Public accessors (used by Vulkan handle classes) ----
	VkDevice getVkDevice() const { return vkdevice; }
	VmaAllocator getVmaAllocator() const { return vmaallocator; }
	uint32_t getCurrentFrame() const { return currentframe; }

	// Deferred deletion
	void deferDeletion(const DeferredDeletion& del);

	// Buffer pool
	uint32_t allocBufferSlot();
	void freeBufferSlot(uint32_t handle);
	VulkanBuffer& getBufferSlot(uint32_t handle) { return bufferpool[handle]; }

	// VAO pool
	uint32_t allocVAOSlot();
	void freeVAOSlot(uint32_t handle);
	VulkanVAOState& getVAOSlot(uint32_t handle) { return vaopool[handle]; }
	void setCurrentVAO(uint32_t vao) { currentvao = vao; }

	// Shader programs
	VkShaderModule createShaderModule(const char* spirvdata, size_t spirvsize);
	bool getSpirvData(const std::string& id, const char** outdata, size_t* outsize);
	std::string parseVkIdentifier(const char* source);
	void setActiveShaderProgram(uint32_t program) { activeshaderprogram = program; }
	uint32_t addShaderProgram(const VulkanShaderProgram& sp);
	VulkanShaderProgram& getShaderProgram(uint32_t index) { return shaderprograms[index]; }

	// Uniform staging
	uint8_t* getPushConstantData() { return pushconstantdata; }
	MaterialUBOData& getMaterialData() { return materialdata; }

	// UBO binding
	void setUBOBindingHandle(int bindingPoint, uint32_t bufferSlot);

	// Texture binding
	void bindTexture(int slot, gTextureVk* tex);
	void unbindTexture(int slot);
	VkCommandPool getCommandPool() const { return vkcommandpool; }
	VkQueue getGraphicsQueue() const { return vkgraphicsqueue; }

protected:
	void init() override;
	void updatePackUnpackAlignment(int i) override;

private:
	// ---- Vulkan core objects ----
	VkInstance vkinstance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT vkdebugmessenger = VK_NULL_HANDLE;
	VkPhysicalDevice vkphysicaldevice = VK_NULL_HANDLE;
	VkDevice vkdevice = VK_NULL_HANDLE;
	VkQueue vkgraphicsqueue = VK_NULL_HANDLE;
	VkQueue vkpresentqueue = VK_NULL_HANDLE;
	uint32_t graphicsqueuefamily = 0;
	uint32_t presentqueuefamily = 0;

	// ---- Surface & swapchain ----
	VkSurfaceKHR vksurface = VK_NULL_HANDLE;
	VkSwapchainKHR vkswapchain = VK_NULL_HANDLE;
	std::vector<VkImage> swapchainimages;
	std::vector<VkImageView> swapchainimageviews;
	VkFormat swapchainimageformat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapchainextent = {0, 0};

	// ---- Render pass & framebuffers ----
	VkRenderPass vkrenderpass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> swapchainframebuffers;

	// ---- Depth ----
	VkImage depthimage = VK_NULL_HANDLE;
	VmaAllocation depthallocation = VK_NULL_HANDLE;
	VkImageView depthimageview = VK_NULL_HANDLE;
	VkFormat depthformat = VK_FORMAT_D32_SFLOAT;

	// ---- Command pool & buffers ----
	VkCommandPool vkcommandpool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandbuffers;

	// ---- Synchronization ----
	VkSemaphore imageavailablesemaphores[MAX_FRAMES_IN_FLIGHT] = {};
	VkSemaphore renderfinishedsemaphores[MAX_FRAMES_IN_FLIGHT] = {};
	VkFence inflightfences[MAX_FRAMES_IN_FLIGHT] = {};
	uint32_t currentframe = 0;
	uint32_t currentimageindex = 0;

	// ---- VMA ----
	VmaAllocator vmaallocator = VK_NULL_HANDLE;

	// ---- Frame state ----
	bool framebufferresized = false;
	bool frameinprogress = false;
	VkClearColorValue clearcolorvalue = {{0.0f, 0.0f, 0.0f, 1.0f}};

	// ---- Deferred deletion queues ----
	std::vector<DeferredDeletion> deletionqueues[MAX_FRAMES_IN_FLIGHT];

	// ---- Resource pools ----
	std::vector<VulkanBuffer> bufferpool;
	std::vector<bool> bufferpoolused;

	// ---- Collected render commands (per-frame) ----
	std::vector<RenderCommand> rendercommands[MAX_FRAMES_IN_FLIGHT];

	// ---- Dynamic draw pool (per-frame) ----
	std::vector<DynDrawSlot> dyndrawpool[MAX_FRAMES_IN_FLIGHT];
	uint32_t dyndrawindex[MAX_FRAMES_IN_FLIGHT] = {};

	// ---- Shader programs ----
	std::vector<VulkanShaderProgram> shaderprograms;
	mutable uint32_t activeshaderprogram = 0;

	// ---- Pipeline management ----
	VkPipelineLayout pipelinelayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorsetlayout0 = VK_NULL_HANDLE; // Scene + Lights UBOs
	VkDescriptorSetLayout descriptorsetlayout1 = VK_NULL_HANDLE; // Material UBO + textures

	struct PipelineKey {
		uint32_t shaderprogram;
		VkPrimitiveTopology topology;
		bool depthtest;
		bool alphablending;
		bool operator==(const PipelineKey& other) const {
			return shaderprogram == other.shaderprogram
				&& topology == other.topology
				&& depthtest == other.depthtest
				&& alphablending == other.alphablending;
		}
	};
	struct PipelineKeyHash {
		size_t operator()(const PipelineKey& k) const {
			size_t h = std::hash<uint32_t>()(k.shaderprogram);
			h ^= std::hash<uint32_t>()(static_cast<uint32_t>(k.topology)) << 16;
			h ^= std::hash<bool>()(k.depthtest) << 1;
			h ^= std::hash<bool>()(k.alphablending) << 2;
			return h;
		}
	};
	std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHash> pipelinecache;
	VkPipeline getOrCreatePipeline(uint32_t program, VkPrimitiveTopology topology, bool depthtest, bool alphablend);

	void createPipelineLayout();
	void createDescriptorSetLayouts();

	// ---- Descriptor sets ----
	static constexpr int MAX_MATERIAL_DRAWS_PER_FRAME = 4096;
	VkDescriptorPool scenedescriptorpool = VK_NULL_HANDLE;
	VkDescriptorPool materialdescriptorpools[MAX_FRAMES_IN_FLIGHT] = {};
	VkDescriptorSet scenedescriptorsets[MAX_FRAMES_IN_FLIGHT] = {};
	uint32_t materialdrawindex = 0;
	void createDescriptorPool();
	void createDescriptorSets();
	bool updateSceneDescriptorSet(uint32_t frameindex);
	VkDescriptorSet updateMaterialDescriptorSet(uint32_t frameindex);

	// ---- VAO state tracking ----
	std::vector<VulkanVAOState> vaopool;
	std::vector<bool> vaopoolused;
	uint32_t currentvao = 0;
	uint32_t currentboundvbo = 0;

	// ---- Uniform staging ----
	alignas(16) uint8_t pushconstantdata[PUSH_CONSTANT_SIZE] = {};
	MaterialUBOData materialdata;
	VulkanBuffer materialubobuffers[MAX_FRAMES_IN_FLIGHT] = {};
	VkDeviceSize materialubostride = 0; // aligned to minUniformBufferOffsetAlignment

	// ---- UBO binding tracking ----
	uint32_t sceneubohandle = 0;
	uint32_t lightsubohandle = 0;

	// ---- Bound textures ----
	gTextureVk* boundtextures[2] = {nullptr, nullptr};

	// ---- Dummy resources ----
	VkSampler dummysampler = VK_NULL_HANDLE;
	VkImage dummyimage = VK_NULL_HANDLE;
	VmaAllocation dummyimagealloc = VK_NULL_HANDLE;
	VkImageView dummyimageview = VK_NULL_HANDLE;
	void createDummyResources();

	VkPrimitiveTopology glTopologyToVk(int drawMode);

	// ---- Initialization helpers ----
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createVmaAllocator();
	void createSwapchain();
	void createImageViews();
	void createRenderPass();
	void createDepthResources();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapchain();
	void cleanupSwapchain();

	// ---- Utility ----
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
};

#endif // GLIST_VULKAN
#endif // CORE_GVULKANRENDERENGINE_H
