#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

#include "../Debug/VKDebugAllocator.h"
#include "../Debug/VKLogger.h"


//Responsible for the initialisation, ownership, and clean shutdown of a VkInstance, VkDevice, and VkQueue

namespace Neki
{

	class VulkanDevice
	{
	public:
		explicit VulkanDevice(const VKLogger& _logger,
					VKDebugAllocator& _instDebugAllocator,
					VKDebugAllocator& _deviceDebugAllocator,
					const std::uint32_t _apiVer=VK_MAKE_API_VERSION(0,1,0,0),
					const char* _appName="Vulkan App",
					std::vector<const char*>* _desiredInstanceLayers=nullptr,
					std::vector<const char*>* _desiredInstanceExtensions=nullptr,
					std::vector<const char*>* _desiredDeviceLayers=nullptr,
					std::vector<const char*>* _desiredDeviceExtensions=nullptr);

		~VulkanDevice();

		[[nodiscard]] const VkInstance& GetInstance() const;
		[[nodiscard]] const VkPhysicalDevice& GetPhysicalDevice() const;
		[[nodiscard]] const VkDevice& GetDevice() const;
		[[nodiscard]] const VkQueue& GetGraphicsQueue() const;
		[[nodiscard]] const std::size_t& GetGraphicsQueueFamilyIndex() const;
		
	private:
		//Dependency injections from VKApp
		const VKLogger& logger;
		VKDebugAllocator& instDebugAllocator;
		VKDebugAllocator& deviceDebugAllocator;
		
		VkInstance inst;
		VkPhysicalDeviceType physicalDeviceType;
		std::size_t physicalDeviceIndex; //Used for logging purposes only
		VkPhysicalDevice physicalDevice;
		VkDevice device;

		//Queue that has graphics support
		std::size_t graphicsQueueFamilyIndex;
		VkQueue graphicsQueue;


		void CreateInstance(const std::uint32_t _apiVer, const char* _appName, std::vector<const char*>* _desiredInstanceLayers, std::vector<const char*>* _desiredInstanceExtensions);
		void SelectPhysicalDevice();
		void CreateLogicalDevice(std::vector<const char*>* _desiredDeviceLayers, std::vector<const char*>* _desiredDeviceExtensions);
	};
}

#endif