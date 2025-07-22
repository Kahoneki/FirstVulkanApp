#include "VulkanDevice.h"

#include <iostream>


namespace Neki
{

VulkanDevice::VulkanDevice(const bool _debug,
				   const std::uint32_t _apiVer,
				   const char* _appName,
				   std::vector<const char*>* _desiredInstanceLayers,
				   std::vector<const char*>* _desiredInstanceExtensions,
				   std::vector<const char*>* _desiredDeviceLayers,
				   std::vector<const char*>* _desiredDeviceExtensions)
{
	debug = _debug;
	CreateInstance(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions);
	SelectPhysicalDevice();
	CreateLogicalDevice(_desiredDeviceLayers, _desiredDeviceExtensions);
}



void VulkanDevice::CreateInstance(const std::uint32_t _apiVer, const char* _appName, std::vector<const char*>* _desiredInstanceLayers, std::vector<const char*>* _desiredInstanceExtensions)
{
	
}



VulkanDevice::~VulkanDevice()
{
	std::cout << "Shutting Down VulkanDevice\n";
	
	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);
		vkDestroyDevice(device, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		device = VK_NULL_HANDLE;
		std::cout << "\tDevice Destroyed\n";
	}

	if (inst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(inst, debug ? static_cast<const VkAllocationCallbacks*>(instDebugAllocator) : nullptr);
		inst = VK_NULL_HANDLE;
		std::cout << "\tInstance Destroyed\n";
	}
}


}