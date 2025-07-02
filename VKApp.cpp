#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>
#include "VKApp.h"

namespace Neki
{

VKApp::VKApp(const bool _debug,
			 const std::uint32_t _apiVer,
			 const char* _appName,
			 std::vector<const char*>* _desiredInstanceLayers,
			 std::vector<const char*>* _desiredInstanceExtensions,
			 std::vector<const char*>* _desiredDeviceLayers,
			 std::vector<const char*>* _desiredDeviceExtensions)
{
	inst = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	Init(_debug, _apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions, _desiredDeviceLayers, _desiredDeviceExtensions);
}



VKApp::~VKApp()
{
	Shutdown();
}



void VKApp::Init(const bool _debug,
				 const std::uint32_t _apiVer,
				 const char* _appName,
				 std::vector<const char*>* _desiredInstanceLayers,
				 std::vector<const char*>* _desiredInstanceExtensions,
				 std::vector<const char*>* _desiredDeviceLayers,
				 std::vector<const char*>* _desiredDeviceExtensions)
{
	//If a name is in a `desired...` vector and is available, it will be added to its corresponding `...ToBeAdded` vector
	std::vector<const char*> instanceLayerNamesToBeAdded;
	std::vector<const char*> instanceExtensionNamesToBeAdded;
	std::vector<const char*> deviceLayerNamesToBeAdded;
	std::vector<const char*> deviceExtensionNamesToBeAdded;

	
	//Enumerate available instance layers
	if (_debug) { std::cout << std::left << std::setw(debugW) << "Scanning for available instance-level layers"; }
	std::uint32_t instanceLayerCount{ 0 };
	VkResult result{ vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) };
	if (_debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (_debug) { std::cout << " (found: " << instanceLayerCount << ")\n"; }
	}
	else
	{
		if (_debug) { std::cout << " (" << result << ")\n"; }
		Shutdown();
	}
	std::vector<VkLayerProperties> instanceLayers;
	instanceLayers.resize(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

	if (_debug)
	{
		std::cout << std::left
				  << std::setw(indxW) << "Index"
				  << std::setw(nameW) << "Name"
				  << std::setw(specW) << "Spec Ver"
				  << std::setw(implW) << "Impl Ver"
				  << std::setw(descW) << "Description";
		std::cout << '\n';
		std::cout << std::setfill('-') << std::setw(indxW + nameW + specW + implW + descW) << "";
		std::cout << std::setfill(' ');
		std::cout << '\n';
	}
	
	for (std::size_t i{ 0 }; i < instanceLayerCount; ++i)
	{
		if (_debug)
		{
			std::cout << std::left
					  << std::setw(indxW) << i
					  << std::setw(nameW) << instanceLayers[i].layerName
					  << std::setw(specW) << instanceLayers[i].specVersion
					  << std::setw(implW) << instanceLayers[i].implementationVersion
					  << std::setw(descW) << instanceLayers[i].description
					  << '\n';
		}

		//Check if current layer is in list of desired layers
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredInstanceLayers->begin(), _desiredInstanceLayers->end(), [&](const char* l)
		{
			return std::strcmp(l, instanceLayers[i].layerName) == 0;
		})};
		if (it != _desiredInstanceLayers->end())
		{
			instanceLayerNamesToBeAdded.push_back(*it);
			_desiredInstanceLayers->erase(it);
		}
	}

	if (_debug)
	{
		for (const std::string& layerName : instanceLayerNamesToBeAdded)
		{
			std::cout << "Added " << layerName << " to instance creation\n";
		}
		for (const std::string& layerName : *_desiredInstanceLayers)
		{
			std::cout << "Failed to add " << layerName << " to instance creation - layer does not exist or is unavailable\n";
		}	
	}


	//Enumerate available instance extensions
	if (_debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for available instance-level extensions"; }
	std::uint32_t instanceExtensionCount{ 0 };
	result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (_debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (_debug) { std::cout << " (found: " << instanceExtensionCount << ")\n"; }
	}
	else
	{
		if (_debug) { std::cout << " (" << result << ")\n"; }
		Shutdown();
	}
	
	std::vector<VkExtensionProperties> instanceExtensions;
	instanceExtensions.resize(instanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());
	if (_debug)
	{
		std::cout << std::left
				  << std::setw(indxW) << "Index"
				  << std::setw(nameW) << "Name"
				  << std::setw(specW) << "Spec Ver";
		std::cout << '\n';
		std::cout << std::setfill('-') << std::setw(indxW + nameW + specW) << "";
		std::cout << std::setfill(' ');
		std::cout << '\n';
	}
	
	for (std::size_t i{ 0 }; i < instanceExtensionCount; ++i)
	{
		if (_debug)
		{
			std::cout << std::left
					  << std::setw(indxW) << i
					  << std::setw(nameW) << instanceExtensions[i].extensionName
					  << std::setw(specW) << instanceExtensions[i].specVersion
					  << '\n';
		}

		//Check if current extension is in list of desired extensions
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredInstanceExtensions->begin(), _desiredInstanceExtensions->end(), [&](const char* e)
		{
			return std::strcmp(e, instanceExtensions[i].extensionName) == 0;
		})};
		if (it != _desiredInstanceExtensions->end())
		{
			instanceExtensionNamesToBeAdded.push_back(*it);
			_desiredInstanceExtensions->erase(it);
		}
	}

	if (_debug)
	{
		for (const std::string& extensionName : instanceExtensionNamesToBeAdded)
		{
			std::cout << "Added " << extensionName << " to instance creation\n";
		}
		for (const std::string& extensionName : *_desiredInstanceExtensions)
		{
			std::cout << "Failed to add " << extensionName << " to instance creation - extension does not exist or is unavailable\n";
		}
	}


	//Initialise vulkan instance
	VkApplicationInfo vkAppInfo{};
	vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkAppInfo.pNext = nullptr;
	vkAppInfo.apiVersion = _apiVer;
	vkAppInfo.pApplicationName = _appName;
	vkAppInfo.applicationVersion = 1;
	vkAppInfo.pEngineName = "Vulkaneki";
	vkAppInfo.engineVersion = 1;

	VkInstanceCreateInfo vkInstInfo{};
	vkInstInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInstInfo.pNext = nullptr;
	vkInstInfo.pApplicationInfo = &vkAppInfo;
	vkInstInfo.enabledLayerCount = instanceLayerNamesToBeAdded.size();
	vkInstInfo.ppEnabledLayerNames = instanceLayerNamesToBeAdded.data();
	vkInstInfo.enabledExtensionCount = instanceExtensionNamesToBeAdded.size();
	vkInstInfo.ppEnabledExtensionNames = instanceExtensionNamesToBeAdded.data();

	if (_debug) { std::cout << std::left << std::setw(debugW) << "Creating vulkan instance"; }
	result = vkCreateInstance(&vkInstInfo, nullptr, &inst);
	if (_debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (_debug) { std::cout << " (" << result << ")\n"; }
	}


	//Enumerate physical devices
	if (_debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for physical vulkan-compatible devices"; }
	std::uint32_t physicalDeviceCount{ 0 };
	result = vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, nullptr);
	if (_debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (_debug) { std::cout << " (found: " << physicalDeviceCount << ")\n\n"; }
	}
	else
	{
		if (_debug) { std::cout << " (" << result << ")\n\n"; }
		Shutdown();
	}
	
	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;
	physicalDevices.resize(physicalDeviceCount);
	physicalDeviceProperties.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, physicalDevices.data());
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties[i]);
		if (_debug)
		{
			std::cout << "\n\nInformation for Device " << i << ":\n";
			std::cout << "Device Name: "; for (char* c{ physicalDeviceProperties[i].deviceName }; *c != '\0'; ++c) { std::cout << *c; } std::cout << "\n";
			std::cout << "API Version: " << physicalDeviceProperties[i].apiVersion << "\n";
			std::cout << "Driver Version: " << physicalDeviceProperties[i].driverVersion << "\n";
			std::cout << "Vendor ID: " << physicalDeviceProperties[i].vendorID << "\n";
			std::cout << "Device ID: " << physicalDeviceProperties[i].deviceID << "\n";
			std::cout << "Device Type: " << physicalDeviceProperties[i].deviceType << "\n";
		}
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProps);

		if (_debug)
		{
			std::cout << "\nMemory Information:\n";
			std::cout << "Num available heaps: " << memProps.memoryHeapCount << '\n';
			std::cout << "Num available memory types: " << memProps.memoryTypeCount << '\n';
			for (std::size_t j{ 0 }; j < memProps.memoryHeapCount; ++j)
			{
				std::cout << "Heap " << j << " (" << ((memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "VRAM" : "RAM") << ", ";
				if		(memProps.memoryHeaps[j].size / (1024 * 1024 * 1024) != 0)	{ std::cout << memProps.memoryHeaps[j].size / (1024 * 1024 * 1024) << "GiB)\n"; }
				else if (memProps.memoryHeaps[j].size / (1024 * 1024) != 0)			{ std::cout << memProps.memoryHeaps[j].size / (1024 * 1024) << "MiB)\n"; }
				else if (memProps.memoryHeaps[j].size / 1024 != 0)					{ std::cout << memProps.memoryHeaps[j].size / 1024 << "KiB)\n"; }
				else																{ std::cout << memProps.memoryHeaps[j].size << "B)\n"; }
			}
			for (std::size_t j{ 0 }; j < memProps.memoryTypeCount; ++j)
			{
				std::cout << "Memory type " << j << " (Heap " << memProps.memoryTypes[j].heapIndex << ", ";
				if (memProps.memoryTypes[j].propertyFlags == 0) { std::cout << "Base)\n"; continue; }
				std::string memTypePropFlagsString;
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)			{ memTypePropFlagsString += "DEVICE_LOCAL | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)			{ memTypePropFlagsString += "HOST_VISIBLE | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)			{ memTypePropFlagsString += "HOST_COHERENT | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)				{ memTypePropFlagsString += "HOST_CACHED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)		{ memTypePropFlagsString += "LAZILY_ALLOCATED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)				{ memTypePropFlagsString += "PROTECTED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)		{ memTypePropFlagsString += "DEVICE_COHERENT_AMD | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)		{ memTypePropFlagsString += "DEVICE_UNCACHED_AMD | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)			{ memTypePropFlagsString += "RDMA_CAPABLE_NV | "; }
				if (!memTypePropFlagsString.empty())
				{
					memTypePropFlagsString.resize(memTypePropFlagsString.length() - 3);
				}
				std::cout << memTypePropFlagsString << ")\n";
			}
		}


		//Enumerate queue families
		std::uint32_t queueFamilyCount;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilyProperties.data());

		if (_debug)
		{
			std::cout << "\nQueue Information:\n";
			std::cout << "Num queue families: " << queueFamilyCount << "\n";
			for (std::size_t j{ 0 }; j < queueFamilyCount; ++j)
			{
				std::cout << "Queue Family " << j << " (" << queueFamilyProperties[j].queueCount << " queues, ";
				if (queueFamilyProperties[j].queueFlags == 0) { std::cout << "Base)\n"; continue; }
				std::string queueFlagsString;
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)			{ queueFlagsString += "GRAPHICS | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)				{ queueFlagsString += "COMPUTE | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT)			{ queueFlagsString += "TRANSFER | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)		{ queueFlagsString += "SPARSE_BINDING | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_PROTECTED_BIT)			{ queueFlagsString += "PROTECTED | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)	{ queueFlagsString += "VIDEO_DECODE_KHR | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)	{ queueFlagsString += "VIDEO_ENCODE_KHR | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)		{ queueFlagsString += "OPTICAL_FLOW_NV | "; }
				if (!queueFlagsString.empty())
				{
					queueFlagsString.resize(queueFlagsString.length() - 3);
				}
				std::cout << queueFlagsString;
				std::cout << ", " << queueFamilyProperties[j].timestampValidBits << " valid timestamp bits)\n";
			}
		}


		//Enumerate available device layers
		if (_debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for available device-level layers"; }
		std::uint32_t deviceLayerCount{ 0 };
		result = vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, nullptr);
		if (_debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
		if (result == VK_SUCCESS)
		{
			if (_debug) { std::cout << " (found: " << deviceLayerCount << ")\n"; }
		}
		else
		{
			if (_debug) { std::cout << " (" << result << ")\n"; }
			Shutdown();
		}
		
		std::vector<VkLayerProperties> deviceLayers;
		deviceLayers.resize(deviceLayerCount);
		vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, deviceLayers.data());
		if (_debug)
		{
			std::cout << std::left
					  << std::setw(indxW) << "Index"
					  << std::setw(nameW) << "Name"
					  << std::setw(specW) << "Spec Ver"
					  << std::setw(implW) << "Impl Ver"
					  << std::setw(descW) << "Description";
			std::cout << '\n';
			std::cout << std::setfill('-') << std::setw(indxW + nameW + specW + implW + descW) << "";
			std::cout << std::setfill(' ');
			std::cout << '\n';
		}

		for (std::size_t j{ 0 }; j < deviceLayerCount; ++j)
		{
			if (_debug)
			{
				std::cout << std::left
						  << std::setw(indxW) << j
						  << std::setw(nameW) << deviceLayers[j].layerName
						  << std::setw(specW) << deviceLayers[j].specVersion
						  << std::setw(implW) << deviceLayers[j].implementationVersion
						  << std::setw(descW) << deviceLayers[j].description
						  << '\n';
			}

			//Check if current layer is in list of desired layers
			std::vector<const char*>::const_iterator it{ std::find_if(_desiredDeviceLayers->begin(), _desiredDeviceLayers->end(), [&](const char* l)
			{
				return std::strcmp(l, deviceLayers[j].layerName) == 0;
			})};
			
			if (it != _desiredDeviceLayers->end())
			{
				deviceLayerNamesToBeAdded.push_back(*it);
				_desiredDeviceLayers->erase(it);
			}
		}

		if (_debug)
		{
			for (const std::string& layerName : deviceLayerNamesToBeAdded)
			{
				std::cout << "Added " << layerName << " to device creation\n";
			}
			for (const std::string& layerName : *_desiredDeviceLayers)
			{
				std::cout << "Failed to add " << layerName << " to device creation - layer does not exist or is unavailable\n";
			}
		}


		//Enumerate available device extensions
		if (_debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for available device-level extensions"; }
		std::uint32_t deviceExtensionCount{ 0 };
		result = vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, nullptr);
		if (_debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
		if (result == VK_SUCCESS)
		{
			if (_debug) { std::cout << " (found: " << deviceExtensionCount << ")\n"; }
		}
		else
		{
			if (_debug) { std::cout << " (" << result << ")\n"; }
			Shutdown();
		}
		
		std::vector<VkExtensionProperties> deviceExtensions;
		deviceExtensions.resize(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, deviceExtensions.data());
		if (_debug)
		{
			std::cout << std::left
					  << std::setw(indxW) << "Index"
					  << std::setw(nameW) << "Name"
					  << std::setw(specW) << "Spec Ver";
			std::cout << '\n';
			std::cout << std::setfill('-') << std::setw(indxW + nameW + specW) << "";
			std::cout << std::setfill(' ');
			std::cout << '\n';
		}

		for (std::size_t j{ 0 }; j < deviceExtensionCount; ++j)
		{
			if (_debug)
			{
				std::cout << std::left
						  << std::setw(indxW) << j
						  << std::setw(nameW) << deviceExtensions[j].extensionName
						  << std::setw(specW) << deviceExtensions[j].specVersion
						  << '\n';
			}

			//Check if current extension is in list of desired extensions
			std::vector<const char*>::const_iterator it{ std::find_if(_desiredDeviceExtensions->begin(), _desiredDeviceExtensions->end(), [&](const char* e)
			{
				return std::strcmp(e, deviceExtensions[j].extensionName) == 0;
			})};
			if (it != _desiredDeviceExtensions->end())
			{
				deviceExtensionNamesToBeAdded.push_back(*it);
				_desiredDeviceExtensions->erase(it);
			}
		}

		if (_debug)
		{
			for (const std::string& extensionName : deviceExtensionNamesToBeAdded)
			{
				std::cout << "Added " << extensionName << " to device creation\n";
			}
			for (const std::string& extensionName : *_desiredDeviceExtensions)
			{
				std::cout << "Failed to add " << extensionName << " to device creation - extension does not exist or is unavailable\n";
			}
		}
	}


	//Create logical device (just based on physical device 0 for now - todo: change this)
	VkPhysicalDeviceFeatures supportedFeatures;
	VkPhysicalDeviceFeatures requiredFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevices[0], &supportedFeatures);
	requiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;
	requiredFeatures.tessellationShader = VK_TRUE;
	requiredFeatures.geometryShader = VK_TRUE;

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueFamilyIndex = 0;
	queueCreateInfo.queueCount = 1;
	constexpr float queuePriority{ 1.0f };
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = deviceLayerNamesToBeAdded.size();
	deviceCreateInfo.ppEnabledLayerNames = deviceLayerNamesToBeAdded.data();
	deviceCreateInfo.enabledExtensionCount = deviceExtensionNamesToBeAdded.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNamesToBeAdded.data();
	deviceCreateInfo.pEnabledFeatures = &requiredFeatures;

	if (_debug) { std::cout << std::left << std::setw(debugW) << "\n\n\nCreating vulkan logical device"; }
	result = vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device);
	if (_debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (_debug) { std::cout << " (" << result << ")\n"; }
		Shutdown();
	}
}



void VKApp::Shutdown()
{
	std::cout << "Shutting down\n";
	
	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);
		vkDestroyDevice(device, nullptr);
		std::cout << "Device Destroyed\n";
	}
	
	if (inst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(inst, nullptr);
		std::cout << "Instance Destroyed\n";
	}
	
	std::cout << "Shutdown complete\n";
}



}