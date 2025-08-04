#ifndef VKAPP_H
#define VKAPP_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

class VKApp final
{
public:
	explicit VKApp(VkExtent2D _windowSize,
				   VkRenderPassCreateInfo _renderPassDesc,
				   std::uint32_t _descriptorPoolSizeCount,
				   VkDescriptorPoolSize* _descriptorPoolSizes,
				   const std::uint32_t _apiVer=VK_MAKE_API_VERSION(0,1,0,0),
				   const char* _appName="Vulkan App",
				   const VKLoggerConfig& _loggerConfig=VKLoggerConfig{},
				   const VK_ALLOCATOR_TYPE _allocatorType=VK_ALLOCATOR_TYPE::DEFAULT,
				   std::vector<const char*>* _desiredInstanceLayers=nullptr,
				   std::vector<const char*>* _desiredInstanceExtensions=nullptr,
				   std::vector<const char*>* _desiredDeviceLayers=nullptr,
				   std::vector<const char*>* _desiredDeviceExtensions=nullptr);

	~VKApp();

	void Run();
	
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
	void UpdateUBO();
	void DrawFrame();
	
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


#endif //VKAPP_H
