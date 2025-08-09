#ifndef VKAPP_H
#define VKAPP_H

#include <vulkan/vulkan.h>
#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../Camera/PlayerCamera.h"
#include "Core/VulkanDevice.h"
#include "Core/VulkanCommandPool.h"
#include "Core/VulkanRenderManager.h"
#include "Core/VulkanDescriptorPool.h"
#include "Core/VulkanGraphicsPipeline.h"

#include "Debug/VKLogger.h"
#include "Debug/VKLoggerConfig.h"
#include "Debug/VKDebugAllocator.h"
#include "Memory/BufferFactory.h"
#include "Memory/ImageFactory.h"

namespace Neki
{

struct UBOData
{
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 texCoord;
};

struct GraphicsPipelineShaderFilepaths
{
	GraphicsPipelineShaderFilepaths() = delete;
	GraphicsPipelineShaderFilepaths(const char* _vert, const char* _frag, const char* _tessCtrl=nullptr, const char* _tessEval=nullptr)
								   : vert(_vert), frag(_frag), tessCtrl(_tessCtrl), tessEval(_tessEval) {}
	const char* vert;
	const char* frag;
	const char* tessCtrl;
	const char* tessEval;
};

struct VKAppCreationDescription
{
	VkExtent2D windowSize;
	VKRenderPassCleanDesc renderPassDesc;
	GraphicsPipelineShaderFilepaths* subpassPipelines;
	std::uint32_t clearValueCount;
	VkClearValue* clearValues;
	std::uint32_t descriptorPoolSizeCount;
	VkDescriptorPoolSize* descriptorPoolSizes;
	std::uint32_t apiVer;
	const char* appName;
	VKLoggerConfig* loggerConfig;
	VK_ALLOCATOR_TYPE allocatorType;
	std::uint32_t desiredInstanceLayerCount;
	const char** desiredInstanceLayers;
	std::uint32_t desiredInstanceExtensionCount;
	const char** desiredInstanceExtensions;
	std::uint32_t desiredDeviceLayerCount;
	const char** desiredDeviceLayers;
	std::uint32_t desiredDeviceExtensionCount;
	const char** desiredDeviceExtensions;
};

class VKApp final
{

	friend class Application;
	
public:
	explicit VKApp(VKAppCreationDescription _creationDescription);

	~VKApp();
	
	
private:
	VKLogger logger;
	VKDebugAllocator instDebugAllocator;
	VKDebugAllocator deviceDebugAllocator;

	//Sub-classes
	std::unique_ptr<VulkanDevice> vulkanDevice;
	std::unique_ptr<VulkanCommandPool> vulkanCommandPool;
	std::unique_ptr<VulkanDescriptorPool> vulkanDescriptorPool;
	std::unique_ptr<BufferFactory> bufferFactory;
	std::unique_ptr<ImageFactory> imageFactory;
	std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
	std::unique_ptr<VulkanRenderManager> vulkanRenderManager;
	std::unique_ptr<VulkanGraphicsPipeline> vulkanGraphicsPipeline;
	std::unique_ptr<VulkanGraphicsPipeline> vulkanPostprocessPipeline;

	//Init sub-functions
	void InitialiseCubeVertexBuffer();
	void InitialiseCubeIndexBuffer();
	void InitialiseQuadVertexBuffer();
	void InitialiseQuadIndexBuffer();
	void InitialiseUBO();
	void InitialiseSampler();
	void InitialiseImage();
	void CreateDescriptorSet();
	void BindDescriptorSet();
	void CreatePostprocessDescriptorSet();
	void BindPostprocessDescriptorSet();
	void CreatePipeline();
	void CreatePostprocessPipeline();

	//Per-frame functions
	void UpdateUBO(PlayerCamera& _playerCamera);
	void DrawFrame(PlayerCamera& _playerCamera);

	std::uint32_t clearValueCount;
	VkClearValue* clearValues;
	
	//Raw vulkan resources
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	VkBuffer quadVertexBuffer;
	VkBuffer quadIndexBuffer;
	VkBuffer ubo;
	VkSampler sampler;
	VkImage image;
	VkImageView imageView;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout postprocessDescriptorSetLayout;
	VkDescriptorSet postprocessDescriptorSet;

	//Persistent buffer maps
	void* vertexBufferMap;
	void* quadVertexBufferMap;
	void* uboMap;

	glm::mat4 cubeModelMatrix;
	UBOData cameraData;
};

}


#endif
