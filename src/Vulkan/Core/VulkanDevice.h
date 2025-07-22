#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

#include "../Memory/VKDebugAllocator.h"


namespace Neki
{
	class VulkanDevice
	{
	public:
		explicit VulkanDevice(const bool _debug=false,
				   const std::uint32_t _apiVer=VK_MAKE_API_VERSION(0,1,0,0),
				   const char* _appName="Vulkan App",
				   std::vector<const char*>* _desiredInstanceLayers=nullptr,
				   std::vector<const char*>* _desiredInstanceExtensions=nullptr,
				   std::vector<const char*>* _desiredDeviceLayers=nullptr,
				   std::vector<const char*>* _desiredDeviceExtensions=nullptr);
		~VulkanDevice();
		
	private:
		bool debug;
		
		VkInstance inst;
		VkPhysicalDeviceType physicalDeviceType;
		VkPhysicalDevice physicalDevice;
		VkDevice device;

		//Queue family index that has graphics support
		std::size_t graphicsQueueFamilyIndex;

		VKDebugAllocator instDebugAllocator;
		VKDebugAllocator deviceDebugAllocator;


		void CreateInstance(const std::uint32_t _apiVer, const char* _appName, std::vector<const char*>* _desiredInstanceLayers, std::vector<const char*>* _desiredInstanceExtensions);
		void SelectPhysicalDevice();
		void CreateLogicalDevice(std::vector<const char*>* _desiredDeviceLayers, std::vector<const char*>* _desiredDeviceExtensions);
	};
}

#endif