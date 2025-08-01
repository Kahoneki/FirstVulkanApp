#ifndef VKAPP_H
#define VKAPP_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Core/VulkanDevice.h"
#include "Core/VulkanCommandPool.h"
#include "Core/VulkanRenderManager.h"
#include "Core/VulkanDescriptorPool.h"
#include "Core/VulkanGraphicsPipeline.h"

#include "Debug/VKLogger.h"
#include "Debug/VKLoggerConfig.h"
#include "Debug/VKDebugAllocator.h"
#include "Memory/BufferFactory.h"

namespace Neki
{

class VKApp final
{
public:
	explicit VKApp(VkExtent2D _windowSize,
				   VkRenderPassCreateInfo _renderPassDesc,
				   VkDescriptorPoolSize _descriptorPoolSize,
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
	//Dependency injections to sub-classes
	VKLogger logger;
	VKDebugAllocator instDebugAllocator;
	VKDebugAllocator deviceDebugAllocator;
	
	//Sub-classes
	std::unique_ptr<VulkanDevice> vulkanDevice;
	std::unique_ptr<VulkanCommandPool> vulkanCommandPool;
	std::unique_ptr<VulkanRenderManager> vulkanRenderManager;
	std::unique_ptr<VulkanDescriptorPool> vulkanDescriptorPool;
	std::unique_ptr<BufferFactory> bufferFactory;
	std::unique_ptr<VulkanGraphicsPipeline> vulkanGraphicsPipeline;

	//Init subfunctions
	void InitialiseBuffer();
	void CreateDescriptorSet();
	void UpdateDescriptorSet();
	void CreatePipeline();

	void DrawFrame();
	
	//Raw vulkan resources
	VkBuffer buffer;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkDescriptorSet> descriptorSets;
};

}


#endif //VKAPP_H
