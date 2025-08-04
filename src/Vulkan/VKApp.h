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

struct VKAppCreationDescription
{
	VkExtent2D windowSize;
	VkRenderPassCreateInfo* renderPassDesc;
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
	std::unique_ptr<VulkanRenderManager> vulkanRenderManager;
	std::unique_ptr<VulkanGraphicsPipeline> vulkanGraphicsPipeline;

	//Init sub-functions
	void InitialiseVertexBuffer();
	void InitialiseIndexBuffer();
	void InitialiseUBO();
	void InitialiseSampler();
	void InitialiseImage();
	void CreateDescriptorSet();
	void BindDescriptorSet();
	void CreatePipeline();

	//Per-frame functions
	void UpdateUBO(PlayerCamera& _playerCamera);
	void DrawFrame(PlayerCamera& _playerCamera);
	
	//Raw vulkan resources
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	VkBuffer ubo;
	VkSampler sampler;
	VkImage image;
	VkImageView imageView;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	//Persistent buffer maps
	void* vertexBufferMap;
	void* uboMap;

	glm::mat4 cubeModelMatrix;
	UBOData cameraData;
};

}


#endif
