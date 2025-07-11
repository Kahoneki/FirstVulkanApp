#include <vulkan/vulkan.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstring>
#include <string>
#include <fstream>
#include "VKApp.h"

namespace Neki
{
	//Forward declarations
	std::string GetFormattedSizeString(std::size_t numBytes);


VKApp::VKApp(const bool _debug,
			 const std::uint32_t _apiVer,
			 const char* _appName,
			 std::vector<const char*>* _desiredInstanceLayers,
			 std::vector<const char*>* _desiredInstanceExtensions,
			 std::vector<const char*>* _desiredDeviceLayers,
			 std::vector<const char*>* _desiredDeviceExtensions)
	: instDebugAllocator(false), deviceDebugAllocator(false)
{
	debug = _debug;
	inst = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	graphicsQueue = VK_NULL_HANDLE;
	commandPool = VK_NULL_HANDLE;
	buffer = VK_NULL_HANDLE;
	bufferMemory = VK_NULL_HANDLE;
	descriptorSetLayout = VK_NULL_HANDLE;
	pipelineLayout = VK_NULL_HANDLE;
	descriptorPool = VK_NULL_HANDLE;
	descriptorSet = VK_NULL_HANDLE;
	vertexShaderModule = VK_NULL_HANDLE;
	fragmentShaderModule = VK_NULL_HANDLE;
	pipeline = VK_NULL_HANDLE;
	Init(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions, _desiredDeviceLayers, _desiredDeviceExtensions);
}



VKApp::~VKApp()
{
	Shutdown();
}



void VKApp::Init(const std::uint32_t _apiVer,
				 const char* _appName,
				 std::vector<const char*>* _desiredInstanceLayers,
				 std::vector<const char*>* _desiredInstanceExtensions,
				 std::vector<const char*>* _desiredDeviceLayers,
				 std::vector<const char*>* _desiredDeviceExtensions)
{

	CreateInstance(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions);
	SelectPhysicalDevice();
	CreateLogicalDevice(_desiredDeviceLayers, _desiredDeviceExtensions);
	CreateCommandPool();
	AllocateCommandBuffers();
	CreateBuffer();
	PopulateBuffer();
	CreateDescriptorSetLayout();
	CreatePipelineLayout();
	CreateDescriptorPool();
	AllocateDescriptorSets();
	UpdateDescriptorSets();
	CreateShaderModules();
	CreatePipeline();
}



void VKApp::CreateInstance(const std::uint32_t _apiVer,
						   const char* _appName,
						   std::vector<const char*>* _desiredInstanceLayers,
						   std::vector<const char*>* _desiredInstanceExtensions)
{
	if (debug) { std::cout << "\n\n\nCreating Instance\n"; }

	//If a name is in a `desired...` vector and is available, it will be added to its corresponding `...ToBeAdded` vector
	std::vector<const char*> instanceLayerNamesToBeAdded;
	std::vector<const char*> instanceExtensionNamesToBeAdded;

	//Enumerate available instance layers
	if (debug) { std::cout << std::left << std::setw(debugW) << "Scanning for available instance-level layers"; }
	std::uint32_t instanceLayerCount{ 0 };
	VkResult result{ vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (debug) { std::cout << " (found: " << instanceLayerCount << ")\n"; }
	}
	else
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
	std::vector<VkLayerProperties> instanceLayers;
	instanceLayers.resize(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

	if (debug)
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
		if (debug)
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
		if (_desiredInstanceLayers == nullptr) { continue; }
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredInstanceLayers->begin(), _desiredInstanceLayers->end(), [&](const char* l)
		{
			return std::strcmp(l, instanceLayers[i].layerName) == 0;
		}) };
		if (it != _desiredInstanceLayers->end())
		{
			instanceLayerNamesToBeAdded.push_back(*it);
			_desiredInstanceLayers->erase(it);
		}
	}

	if (debug)
	{
		for (const std::string& layerName : instanceLayerNamesToBeAdded)
		{
			std::cout << "Added " << layerName << " to instance creation\n";
		}
		if (_desiredInstanceLayers != nullptr)
		{
			for (const std::string& layerName : *_desiredInstanceLayers)
			{
				std::cout << "Failed to add " << layerName << " to instance creation - layer does not exist or is unavailable\n";
			}
		}
	}


	//Enumerate available instance extensions
	if (debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for available instance-level extensions"; }
	std::uint32_t instanceExtensionCount{ 0 };
	result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (debug) { std::cout << " (found: " << instanceExtensionCount << ")\n"; }
	}
	else
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	std::vector<VkExtensionProperties> instanceExtensions;
	instanceExtensions.resize(instanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());
	if (debug)
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
		if (debug)
		{
			std::cout << std::left
				<< std::setw(indxW) << i
				<< std::setw(nameW) << instanceExtensions[i].extensionName
				<< std::setw(specW) << instanceExtensions[i].specVersion
				<< '\n';
		}

		//Check if current extension is in list of desired extensions
		if (_desiredInstanceExtensions == nullptr) { continue; }
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredInstanceExtensions->begin(), _desiredInstanceExtensions->end(), [&](const char* e)
		{
			return std::strcmp(e, instanceExtensions[i].extensionName) == 0;
		}) };
		if (it != _desiredInstanceExtensions->end())
		{
			instanceExtensionNamesToBeAdded.push_back(*it);
			_desiredInstanceExtensions->erase(it);
		}
	}

	if (debug)
	{
		for (const std::string& extensionName : instanceExtensionNamesToBeAdded)
		{
			std::cout << "Added " << extensionName << " to instance creation\n";
		}
		if (_desiredInstanceExtensions != nullptr)
		{
			for (const std::string& extensionName : *_desiredInstanceExtensions)
			{
				std::cout << "Failed to add " << extensionName << " to instance creation - extension does not exist or is unavailable\n";
			}
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

	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating vulkan instance"; }
	result = vkCreateInstance(&vkInstInfo, debug ? static_cast<const VkAllocationCallbacks*>(instDebugAllocator) : nullptr, &inst);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
	}
}



void VKApp::SelectPhysicalDevice()
{

	if (debug) { std::cout << "\n\n\nSelecting Physical Device\n"; }

	//Enumerate physical devices
	if (debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for physical vulkan-compatible devices"; }
	std::uint32_t physicalDeviceCount{ 0 };
	VkResult result{ vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, nullptr) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }
	if (result == VK_SUCCESS)
	{
		if (debug) { std::cout << " (found: " << physicalDeviceCount << ")\n"; }
	}
	else
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;
	physicalDevices.resize(physicalDeviceCount);
	physicalDeviceProperties.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, physicalDevices.data());
	physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM; //Stores the best device type that has currently been found
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties[i]);
		if (debug)
		{
			std::cout << "\n\nInformation for Device " << i << ":\n";
			std::cout << "Device Name: "; for (char* c{ physicalDeviceProperties[i].deviceName }; *c != '\0'; ++c) { std::cout << *c; } std::cout << "\n";
			std::cout << "API Version: " << physicalDeviceProperties[i].apiVersion << "\n";
			std::cout << "Driver Version: " << physicalDeviceProperties[i].driverVersion << "\n";
			std::cout << "Vendor ID: " << physicalDeviceProperties[i].vendorID << "\n";
			std::cout << "Device ID: " << physicalDeviceProperties[i].deviceID << "\n";
			std::cout << "Device Type: " << physicalDeviceProperties[i].deviceType << "\n";
		}
		if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			physicalDeviceIndex = i;
		}
		else if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			if (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
				physicalDeviceIndex = i;
			}
		}
		else if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
		{
			if ((physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
			{
				physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
				physicalDeviceIndex = i;
			}
		}

		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProps);
		if (debug)
		{
			std::cout << "\nMemory Information:\n";
			std::cout << "Num available heaps: " << memProps.memoryHeapCount << '\n';
			std::cout << "Num available memory types: " << memProps.memoryTypeCount << '\n';
			for (std::size_t j{ 0 }; j < memProps.memoryHeapCount; ++j)
			{
				std::cout << "Heap " << j << " (" << ((memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "VRAM" : "RAM") << ", ";
				std::cout << GetFormattedSizeString(memProps.memoryHeaps[j].size) << "\n";
			}
			for (std::size_t j{ 0 }; j < memProps.memoryTypeCount; ++j)
			{
				std::cout << "Memory type " << j << " (Heap " << memProps.memoryTypes[j].heapIndex << ", ";
				if (memProps.memoryTypes[j].propertyFlags == 0) { std::cout << "Base)\n"; continue; }
				std::string memTypePropFlagsString;
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) { memTypePropFlagsString += "DEVICE_LOCAL | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { memTypePropFlagsString += "HOST_VISIBLE | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) { memTypePropFlagsString += "HOST_COHERENT | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) { memTypePropFlagsString += "HOST_CACHED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) { memTypePropFlagsString += "LAZILY_ALLOCATED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) { memTypePropFlagsString += "PROTECTED | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) { memTypePropFlagsString += "DEVICE_COHERENT_AMD | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) { memTypePropFlagsString += "DEVICE_UNCACHED_AMD | "; }
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) { memTypePropFlagsString += "RDMA_CAPABLE_NV | "; }
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
		if (debug)
		{
			std::cout << "\nQueue Information:\n";
			std::cout << "Num queue families: " << queueFamilyCount << "\n";
			for (std::size_t j{ 0 }; j < queueFamilyCount; ++j)
			{
				std::cout << "Queue Family " << j << " (" << queueFamilyProperties[j].queueCount << " queues, ";
				if (queueFamilyProperties[j].queueFlags == 0) { std::cout << "Base)\n"; continue; }
				std::string queueFlagsString;
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queueFlagsString += "GRAPHICS | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) { queueFlagsString += "COMPUTE | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) { queueFlagsString += "TRANSFER | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) { queueFlagsString += "SPARSE_BINDING | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_PROTECTED_BIT) { queueFlagsString += "PROTECTED | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) { queueFlagsString += "VIDEO_DECODE_KHR | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) { queueFlagsString += "VIDEO_ENCODE_KHR | "; }
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) { queueFlagsString += "OPTICAL_FLOW_NV | "; }
				if (!queueFlagsString.empty())
				{
					queueFlagsString.resize(queueFlagsString.length() - 3);
				}
				std::cout << queueFlagsString;
				std::cout << ", " << queueFamilyProperties[j].timestampValidBits << " valid timestamp bits)\n";
			}
		}


		//Enumerate available device layers
		if (debug)
		{
			std::cout << std::left << std::setw(debugW) << "\nScanning for available device-level layers";
			std::uint32_t deviceLayerCount{ 0 };
			result = vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, nullptr);
			std::cout << (result == VK_SUCCESS ? "success" : "failure");
			if (result == VK_SUCCESS)
			{
				std::cout << " (found: " << deviceLayerCount << ")\n";
			}
			else
			{
				std::cout << " (" << result << ")\n";
				Shutdown(true);
				return;
			}

			std::vector<VkLayerProperties> deviceLayers;
			deviceLayers.resize(deviceLayerCount);
			vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, deviceLayers.data());
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
			for (std::size_t j{ 0 }; j < deviceLayerCount; ++j)
			{
				std::cout << std::left
					<< std::setw(indxW) << j
					<< std::setw(nameW) << deviceLayers[j].layerName
					<< std::setw(specW) << deviceLayers[j].specVersion
					<< std::setw(implW) << deviceLayers[j].implementationVersion
					<< std::setw(descW) << deviceLayers[j].description
					<< '\n';
			}
		}


		//Enumerate available device extensions
		if (debug)
		{
			std::cout << std::left << std::setw(debugW) << "\nScanning for available device-level extensions";
			std::uint32_t deviceExtensionCount{ 0 };
			result = vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, nullptr);
			std::cout << (result == VK_SUCCESS ? "success" : "failure");
			if (result == VK_SUCCESS)
			{
				std::cout << " (found: " << deviceExtensionCount << ")\n";
			}
			else
			{
				std::cout << " (" << result << ")\n";
				Shutdown(true);
				return;
			}

			std::vector<VkExtensionProperties> deviceExtensions;
			deviceExtensions.resize(deviceExtensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, deviceExtensions.data());
			std::cout << std::left
				<< std::setw(indxW) << "Index"
				<< std::setw(nameW) << "Name"
				<< std::setw(specW) << "Spec Ver";
			std::cout << '\n';
			std::cout << std::setfill('-') << std::setw(indxW + nameW + specW) << "";
			std::cout << std::setfill(' ');
			std::cout << '\n';
			for (std::size_t j{ 0 }; j < deviceExtensionCount; ++j)
			{
				std::cout << std::left
					<< std::setw(indxW) << j
					<< std::setw(nameW) << deviceExtensions[j].extensionName
					<< std::setw(specW) << deviceExtensions[j].specVersion
					<< '\n';
			}
		}
	}
}



void VKApp::CreateLogicalDevice(std::vector<const char*>* _desiredDeviceLayers,
								std::vector<const char*>* _desiredDeviceExtensions)
{
	//If a name is in a `desired...` vector and is available, it will be added to its corresponding `...ToBeAdded` vector
	std::vector<const char*> deviceLayerNamesToBeAdded;
	std::vector<const char*> deviceExtensionNamesToBeAdded;

	if (debug) { std::cout << "\n\n\nCreating Logical Device\n"; }

	if ((physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_CPU))
	{
		std::cerr << "ERR::VKAPP::CREATE_LOGICAL_DEVICE::NO_AVAILABLE_DEVICE_FITTING_TYPE_REQUIREMENTS::AVAILABLE_TYPES=DISCRETE_GPU;INTEGRATED_GPU;CPU::SHUTTING_DOWN" << std::endl;
		Shutdown(true);
		return;
	}

	//Get graphics queue family index for chosen device
	std::uint32_t queueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[physicalDeviceIndex], &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[physicalDeviceIndex], &queueFamilyCount, queueFamilies.data());
	bool foundGraphicsQueue{ false };
	for (size_t i = 0; i < queueFamilies.size(); ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsQueueFamilyIndex = i;
			foundGraphicsQueue = true;
			break;
		}
	}
	if (!foundGraphicsQueue)
	{
		std::cerr << "ERR::VKAPP::CREATE_LOGICAL_DEVICE::CHOSEN_DEVICE_DOES_NOT_PROVIDE_A_QUEUE_FAMILY_WITH_GRAPHICS_QUEUE_BIT_ENABLED::SHUTTING_DOWN" << std::endl;
		Shutdown(true);
		return;
	}

	//Get desired layers for chosen device
	std::uint32_t deviceLayerCount{ 0 };
	VkResult result{ vkEnumerateDeviceLayerProperties(physicalDevices[physicalDeviceIndex], &deviceLayerCount, nullptr) };
	std::vector<VkLayerProperties> deviceLayers;
	deviceLayers.resize(deviceLayerCount);
	vkEnumerateDeviceLayerProperties(physicalDevices[physicalDeviceIndex], &deviceLayerCount, deviceLayers.data());
	for (std::size_t j{ 0 }; j < deviceLayerCount; ++j)
	{
		//Check if current layer is in list of desired layers
		if (_desiredDeviceLayers == nullptr) { break; }
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredDeviceLayers->begin(), _desiredDeviceLayers->end(), [&](const char* l)
		{
			return std::strcmp(l, deviceLayers[j].layerName) == 0;
		}) };

		if (it != _desiredDeviceLayers->end())
		{
			deviceLayerNamesToBeAdded.push_back(*it);
			_desiredDeviceLayers->erase(it);
		}
	}
	if (debug)
	{
		for (const std::string& layerName : deviceLayerNamesToBeAdded)
		{
			std::cout << "Added " << layerName << " to device creation\n";
		}
		if (_desiredDeviceLayers != nullptr)
		{
			for (const std::string& layerName : *_desiredDeviceLayers)
			{
				std::cout << "Failed to add " << layerName << " to device creation - layer does not exist or is unavailable\n";
			}
		}
	}

	//Get desired extensions for chosen device
	std::uint32_t deviceExtensionCount{ 0 };
	result = vkEnumerateDeviceExtensionProperties(physicalDevices[physicalDeviceIndex], nullptr, &deviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> deviceExtensions;
	deviceExtensions.resize(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevices[physicalDeviceIndex], nullptr, &deviceExtensionCount, deviceExtensions.data());
	for (std::size_t j{ 0 }; j < deviceExtensionCount; ++j)
	{
		//Check if current extension is in list of desired extensions
		if (_desiredDeviceExtensions == nullptr) { continue; }
		std::vector<const char*>::const_iterator it{ std::find_if(_desiredDeviceExtensions->begin(), _desiredDeviceExtensions->end(), [&](const char* e)
		{
			return std::strcmp(e, deviceExtensions[j].extensionName) == 0;
		}) };
		if (it != _desiredDeviceExtensions->end())
		{
			deviceExtensionNamesToBeAdded.push_back(*it);
			_desiredDeviceExtensions->erase(it);
		}
	}
	if (debug)
	{
		for (const std::string& extensionName : deviceExtensionNamesToBeAdded)
		{
			std::cout << "Added " << extensionName << " to device creation\n";
		}
		if (_desiredDeviceExtensions != nullptr)
		{
			for (const std::string& extensionName : *_desiredDeviceExtensions)
			{
				std::cout << "Failed to add " << extensionName << " to device creation - extension does not exist or is unavailable\n";
			}
		}
	}


	VkPhysicalDeviceFeatures supportedFeatures;
	VkPhysicalDeviceFeatures requiredFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevices[physicalDeviceIndex], &supportedFeatures);
	requiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;
	requiredFeatures.tessellationShader = VK_TRUE;
	requiredFeatures.geometryShader = VK_TRUE;

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
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

	if (debug)
	{
		std::string deviceTypeName{ physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "DISCRETE_GPU" : (physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "INTEGRATED_GPU" : "CPU") };
		std::cout << std::left << std::setw(debugW) << "Creating vulkan logical device for device " + std::to_string(physicalDeviceIndex) + " (" + deviceTypeName + ")";
	}
	result = vkCreateDevice(physicalDevices[physicalDeviceIndex], &deviceCreateInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &device);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	//Get the queue handle
	if (debug) { std::cout << "Getting queue handle\n"; }
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}



void VKApp::CreateCommandPool()
{
	if (debug) { std::cout << "\n\n\nCreating Command Pool\n"; }

	//Crete the pool
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating command pool for queue family " + std::to_string(graphicsQueueFamilyIndex) + " (queue index " + std::to_string(0) + ")"; }
	VkResult result{ vkCreateCommandPool(device, &poolInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &commandPool) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::AllocateCommandBuffers()
{
	if (debug) { std::cout << "\n\n\nAllocating Command Buffers\n"; }

	//Allocate the command buffer
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Allocating " + std::to_string(allocInfo.commandBufferCount) + " command buffer" + (allocInfo.commandBufferCount != 1 ? "s" : "") + " of level " + (allocInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? "PRIMARY" : "SECONDARY"); }
	VkResult result{ vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::CreateBuffer()
{
	if (debug) { std::cout << "\n\n\nCreating Buffer\n"; }

	//Create buffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = 1024 * 1024; //1 MiB
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (debug)
	{
		std::cout << std::left << std::setw(debugW) << "Creating buffer (size: " + GetFormattedSizeString(bufferInfo.size) + ")";
	}
	VkResult result{ vkCreateBuffer(device, &bufferInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &buffer) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	//Check memory requirements of buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	if (debug)
	{
		std::cout << "Buffer memory requirements:\n";
		std::cout << "- " << GetFormattedSizeString(memRequirements.size) << " bytes minimum allocation size\n";
		std::cout << "- " << memRequirements.alignment << "-byte allocation alignment\n";
		std::string allowedMemTypeIndicesStr{};
		bool isFirst{ true };
		for (std::uint32_t i{ 0 }; i < 32; ++i)
		{
			if (memRequirements.memoryTypeBits & (1 << i))
			{
				if (!isFirst) { allowedMemTypeIndicesStr += ", "; }
				allowedMemTypeIndicesStr += std::to_string(i);
				isFirst = false;
			}
		}
		std::cout << "- Allocated to memory-type-index in {" << allowedMemTypeIndicesStr << "}\n";
	}

	//Find a memory type that fits the requirements
	std::uint32_t memTypeIndex{ UINT32_MAX };
	VkMemoryPropertyFlags properties{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	VkPhysicalDeviceMemoryProperties physicalDeviceMemProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevices[physicalDeviceIndex], &physicalDeviceMemProps);
	for (std::uint32_t i{ 0 }; i < physicalDeviceMemProps.memoryTypeCount; ++i)
	{
		//Check if this memory type is allowed for our buffer
		if (!(memRequirements.memoryTypeBits & (1 << i))) { continue; }
		if ((physicalDeviceMemProps.memoryTypes[i].propertyFlags & properties) != properties) { continue; }
		
		//This memory type is allowed
		memTypeIndex = i;
		if (debug) { std::cout << "Found suitable memory type at index " << i << "\n"; }
		break;
	}
	if (memTypeIndex == UINT32_MAX)
	{
		std::cerr << "ERR::VKAPP::CREATEBUFFER::NO_AVAILABLE_MEMORY_TYPE_FITTING_REQUIREMENTS::SHUTTING_DOWN" << std::endl;
		Shutdown(true);
		return;
	}

	//Allocate an adequate region of memory for the buffer
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memTypeIndex;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Allocating buffer memory"; }
	result = vkAllocateMemory(device, &allocInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &bufferMemory);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	//Bind the allocated memory region to the buffer
	if (debug) { std::cout << std::left << std::setw(debugW) << "Binding buffer memory"; }
	result = vkBindBufferMemory(device, buffer, bufferMemory, 0);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << "(" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::PopulateBuffer()
{
	if (debug) { std::cout << "\n\n\nPopulating Buffer\n"; }

	//Map buffer memory
	void* data;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Mapping buffer memory"; }
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	VkResult result{ vkMapMemory(device, bufferMemory, 0, memRequirements.size, 0, &data) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	//Write to buffer (just use a test pattern for now)
	memset(data, 0xCD, memRequirements.size);
	if (debug) { std::cout << "Buffer memory filled with 0xCD pattern\n"; }

	//Unmap the memory
	vkUnmapMemory(device, bufferMemory);
	if (debug) { std::cout << "Buffer memory unmapped\n"; }
}



void VKApp::CreateDescriptorSetLayout()
{
	if (debug) { std::cout << "\n\n\nCreating Descriptor Set Layout\n"; }

	if (debug) { std::cout << "Defining UBO binding at binding point 0 accessible to the vertex stage\n"; }
	
	//Define descriptor binding 0 as a uniform buffer accessible from the vertex shader
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	
	//Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating descriptor set layout"; }
	VkResult result{ vkCreateDescriptorSetLayout(device, &layoutInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &descriptorSetLayout ) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::CreatePipelineLayout()
{
	if (debug) { std::cout << "\n\n\nCreating Pipeline Layout\n"; }

	//Define pipeline layout for our descriptor set
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pPushConstantRanges = nullptr;

	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating pipeline layout"; }
	VkResult result{ vkCreatePipelineLayout(device, &layoutInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &pipelineLayout) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::CreateDescriptorPool()
{
	if (debug) { std::cout << "\n\n\nCreating Descriptor Pool\n"; }

	//Define the pool sizes
	VkDescriptorPoolSize poolSize;
	poolSize.descriptorCount = 1;
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	//Crete the pool
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating descriptor pool for 1 UBO descriptor"; }
	VkResult result{ vkCreateDescriptorPool(device, &poolInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &descriptorPool) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::AllocateDescriptorSets()
{
	if (debug) { std::cout << "\n\n\nAllocating Descriptor Sets\n"; }

	//Allocate the descriptor set
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Allocating " + std::to_string(allocInfo.descriptorSetCount) + " descriptor set" + (allocInfo.descriptorSetCount != 1 ? "s" : ""); }
	VkResult result{ vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::UpdateDescriptorSets()
{
	if (debug) { std::cout << "\n\n\nUpdating Descriptor Sets\n"; }

	//Update descriptor set to make descriptor at binding 0 point to VKApp::buffer
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptorWrite;
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.pNext = nullptr;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite.pBufferInfo = &bufferInfo;
	descriptorWrite.pTexelBufferView = nullptr;
	descriptorWrite.pImageInfo = nullptr;

	vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

	if (debug) { std::cout << "Descriptor set binding 0 updated to point to buffer\n"; }
}



void VKApp::CreateShaderModules()
{
	if (debug) { std::cout << "\n\n\Creating Shader Modules\n"; }
	
	//Read vertex shader
	if (debug) { std::cout << "Reading vertex shader\n"; }
	std::ifstream vertFile("shader.vert.spv", std::ios::ate | std::ios::binary);
	if (!vertFile.is_open())
	{
		std::cerr << "ERR::VKAPP::CREATE_SHADER_MODULES::FAILED_TO_OPEN_VERTEX_SHADER" << std::endl;
		throw std::runtime_error("");
	}
	std::size_t vertShaderFileSize{ static_cast<std::size_t>(vertFile.tellg()) };
	std::vector<char> vertShaderBuf(vertShaderFileSize);
	vertFile.seekg(0);
	vertFile.read(vertShaderBuf.data(), vertShaderFileSize);
	vertFile.close();
	
	//Read fragment shader
	if (debug) { std::cout << "Reading fragment shader\n"; }
	std::ifstream fragFile("shader.frag.spv", std::ios::ate | std::ios::binary);
	if (!fragFile.is_open())
	{
		std::cerr << "ERR::VKAPP::CREATE_SHADER_MODULES::FAILED_TO_OPEN_FRAGMENT_SHADER" << std::endl;
		throw std::runtime_error("");
	}
	std::size_t fragShaderFileSize{ static_cast<std::size_t>(fragFile.tellg()) };
	std::vector<char> fragShaderBuf(fragShaderFileSize);
	fragFile.seekg(0);
	fragFile.read(fragShaderBuf.data(), fragShaderFileSize);
	fragFile.close();


	//Create vertexShaderModule
	VkShaderModuleCreateInfo vertShaderModuleInfo{};
	vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderModuleInfo.pNext = nullptr;
	vertShaderModuleInfo.flags = 0;
	vertShaderModuleInfo.codeSize = vertShaderBuf.size();
	vertShaderModuleInfo.pCode = reinterpret_cast<const std::uint32_t*>(vertShaderBuf.data());
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating vertex shader module"; }
	VkResult result{ vkCreateShaderModule(device, &vertShaderModuleInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &vertexShaderModule) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}

	//Create fragmentShaderModule
	VkShaderModuleCreateInfo fragShaderModuleInfo{};
	fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderModuleInfo.pNext = nullptr;
	fragShaderModuleInfo.flags = 0;
	fragShaderModuleInfo.codeSize = fragShaderBuf.size();
	fragShaderModuleInfo.pCode = reinterpret_cast<const std::uint32_t*>(fragShaderBuf.data());
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating fragment shader module"; }
	result = vkCreateShaderModule(device, &fragShaderModuleInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &fragmentShaderModule);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
}



void VKApp::CreatePipeline()
{
	if (debug) { std::cout << "\n\n\nCreating Graphics Pipeline\n"; }

	//Define vertex shader stage
	if (debug) { std::cout << "Defining vertex shader stage\n"; }
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	//Define fragment shader stage
	if (debug) { std::cout << "Defining fragment shader stage\n"; }
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderStageInfo, fragShaderStageInfo };

	//Define vertex input (minimal since we're not using vertex buffers)
	if (debug) { std::cout << "Defining vertex input\n"; }
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.pVertexAttributeDescriptions = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	//Define input assembly
	if (debug) { std::cout << "Defining input assembly\n"; }
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//Define viewport and scissor as dynamic states
	if (debug) { std::cout << "Defining viewport and scissor dynamic states\n"; }
	std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//Define viewport state (even though we're using dynamic states for both viewport and scissor)
	if (debug) { std::cout << "Defining viewport state\n"; }
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.pScissors = nullptr;


}



void VKApp::Shutdown(bool _throwError)
{
	std::cout << "\n\n\nShutting down\n";

	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, pipeline, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		pipeline = VK_NULL_HANDLE;
		std::cout << "Pipeline Destroyed\n";
	}
	
	if (fragmentShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(device, fragmentShaderModule, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		fragmentShaderModule = VK_NULL_HANDLE;
		std::cout << "Fragment Shader Module Destroyed\n";
	}

	if (vertexShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(device, vertexShaderModule, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		vertexShaderModule = VK_NULL_HANDLE;
		std::cout << "Vertex Shader Module Destroyed\n";
	}

	if (descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device, descriptorPool, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		descriptorPool = VK_NULL_HANDLE;
		std::cout << "Descriptor Pool (and all contained descriptor sets) Destroyed\n";
	}

	if (pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, pipelineLayout, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		pipelineLayout = VK_NULL_HANDLE;
		std::cout << "Pipeline Layout Destroyed\n";
	}

	if (descriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		descriptorSetLayout = VK_NULL_HANDLE;
		std::cout << "Descriptor Set Layout Destroyed\n";
	}

	if (bufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, bufferMemory, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		bufferMemory = VK_NULL_HANDLE;
		std::cout << "Buffer Memory Freed\n";
	}

	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, buffer, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		buffer = VK_NULL_HANDLE;
		std::cout << "Buffer Destroyed\n";
	}

	if (commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, commandPool, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		commandPool = VK_NULL_HANDLE;
		std::cout << "Command Pool (and all contained command buffers) Destroyed\n";
	}

	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);
		vkDestroyDevice(device, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		device = VK_NULL_HANDLE;
		std::cout << "Device Destroyed\n";
	}
	
	if (inst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(inst, debug ? static_cast<const VkAllocationCallbacks*>(instDebugAllocator) : nullptr);
		inst = VK_NULL_HANDLE;
		std::cout << "Instance Destroyed\n";
	}
	
	std::cout << "Shutdown complete\n";

	//True when Shutdown is called as part of error handling, rather than cleanly through the destructor
	//Need to throw an exception here, otherwise an invalid VKApp object still exists
	if (_throwError)
	{
		std::cerr << "ERR::VKAPP::INITIALISATION_FAILED" << std::endl;
		throw std::runtime_error("");
	}
}



std::string GetFormattedSizeString(std::size_t numBytes)
{
	if (numBytes / (1024 * 1024 * 1024) != 0) { return std::to_string(numBytes / (1024 * 1024 * 1024)) + "GiB"; }
	else if (numBytes / (1024 * 1024) != 0) { return std::to_string(numBytes / (1024 * 1024)) + "MiB"; }
	else if (numBytes / 1024 != 0) { return std::to_string(numBytes / 1024) + "KiB"; }
	else { return std::to_string(numBytes) + "B"; }
}



}