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
	window = nullptr;
	currentFrame = 0;
	inst = VK_NULL_HANDLE;
	surface = VK_NULL_HANDLE;
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
	renderPass = VK_NULL_HANDLE;
	pipeline = VK_NULL_HANDLE;
	swapchain = VK_NULL_HANDLE;
	Init(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions, _desiredDeviceLayers, _desiredDeviceExtensions);
}



VKApp::~VKApp()
{
	Shutdown();
}



void VKApp::Run()
{
	if (debug) { std::cout << "\n\n\nStarting Render\n"; }
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(device);
}



void VKApp::Init(const std::uint32_t _apiVer,
                 const char* _appName,
                 std::vector<const char*>* _desiredInstanceLayers,
                 std::vector<const char*>* _desiredInstanceExtensions,
                 std::vector<const char*>* _desiredDeviceLayers,
                 std::vector<const char*>* _desiredDeviceExtensions)
{

	CreateWindow();
	CreateInstance(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions);
	CreateSurface();
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
	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateRenderPass();
	CreatePipeline();
	CreateSwapchainFramebuffers();
	CreateSyncObjects();
}



void VKApp::CreateWindow()
{
	if (debug) { std::cout << "\n\n\nCreating Window\n"; }
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Don't create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //Don't let window be resized (for now)
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating window"; }
	window = glfwCreateWindow(800, 800, "Vulkaneki", nullptr, nullptr);
	if (debug) { std::cout << (window ? "success\n" : "failure\n"); }
	if (!window)
	{
		glfwTerminate();
		Shutdown(true);
		return;
	}
}



void VKApp::CreateSurface()
{
	if (debug) { std::cout << "\n\n\nCreating Surface\n"; }
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating surface"; }
	VkResult result{ glfwCreateWindowSurface(inst, window, nullptr, &surface) }; //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure\n"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
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
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	//Allocate the command buffer
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Allocating " + std::to_string(allocInfo.commandBufferCount) + " command buffer" + (allocInfo.commandBufferCount != 1 ? "s" : "") + " of level " + (allocInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? "PRIMARY" : "SECONDARY"); }
	VkResult result{ vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) };
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
	if (debug) { std::cout << "\n\n\nCreating Shader Modules\n"; }
	
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



void VKApp::CreateSwapchain()
{
	if (debug) { std::cout << "\n\n\nCreating Swapchain\n"; }


	//Get supported formats
	if (debug) { std::cout << std::left << std::setw(debugW) << "Scanning for supported swapchain formats"; }
	
	//Query surface capabilities
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[physicalDeviceIndex], surface, &capabilities);

	//Query supported formats
	std::uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[physicalDeviceIndex], surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats;
	if (formatCount != 0)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[physicalDeviceIndex], surface, &formatCount, formats.data());
	}
	if (debug)
	{
		std::cout << "Found: " << formatCount << '\n';
		for (const VkSurfaceFormatKHR& format : formats)
		{
			std::cout << "Format: " << format.format << " (Colour Space: " << format.colorSpace << ")\n";
		}
	}

	//Choose suitable format
	if (debug) { std::cout << "\nSelecting swapchain format (preferred: " << VK_FORMAT_B8G8R8A8_SRGB << " - VK_FORMAT_B8G8R8A8_SRGB)\n"; }
	VkSurfaceFormatKHR surfaceFormat{ formats[0] }; //Default to first one if ideal isn't found
	for (const VkSurfaceFormatKHR& availableFormat : formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormat = availableFormat;
			break;
		}
	}
	swapchainImageFormat = surfaceFormat.format;
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		swapchainExtent = capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		swapchainExtent.width = std::clamp(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		swapchainExtent.height = std::clamp(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}
	if (debug)
	{
		std::cout << "Selected swapchain format: " << swapchainImageFormat << '\n';
		std::cout << "Selected swapchain extent: " << swapchainExtent.width << 'x' << swapchainExtent.height << '\n';
	}


	//Get supported present modes
	if (debug) { std::cout << std::left << std::setw(debugW) << "\nScanning for supported swapchain present modes"; }

	//Query supported present modes
	std::uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[physicalDeviceIndex], surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes;
	if (presentModeCount != 0)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[physicalDeviceIndex], surface, &presentModeCount, presentModes.data());
	}
	if (debug)
	{
		std::cout << "Found: " << presentModeCount << '\n';
		for (const VkPresentModeKHR& presentMode : presentModes)
		{
			std::cout << "Present mode: " << presentMode << " (" << (presentMode == 0 ? "IMMEDIATE)\n" :
																	(presentMode == 1 ? "MAILBOX)\n" :
																	(presentMode == 2 ? "FIFO)\n" :
																	(presentMode == 3 ? "FIFO_RELAXED)\n" : ")\n"))));
		}
	}
	
	//Choose suitable present mode
	if (debug) { std::cout << "\nSelecting swapchain present mode (preferred: " << VK_PRESENT_MODE_MAILBOX_KHR << " - VK_PRESENT_MODE_MAILBOX_KHR)\n"; }
	VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR }; //Guaranteed to be available
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = availablePresentMode;
			break;
		}
	}
	if (debug)
	{
		std::cout << "Selected swapchain present mode: " << presentMode << '\n';
	}


	//Get image count
	std::uint32_t imageCount{ capabilities.minImageCount + 1 }; //Get 1 more than minimum required by driver for a bit of wiggle room
	if (capabilities.maxImageCount >0 && imageCount > capabilities.maxImageCount) { imageCount = capabilities.maxImageCount; }
	if (debug) { std::cout << "Requesting " << imageCount << " swapchain images\n"; }

	
	//Create swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapchainImageFormat;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating swapchain"; }
	VkResult result = vkCreateSwapchainKHR(device, &createInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &swapchain);
	if (debug) { std::cout << (result == VK_SUCCESS ? "success" : "failure"); }

	//Get number of images in swapchain
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	if (result == VK_SUCCESS)
	{
		if (debug) { std::cout << " (images: " << imageCount << ")\n"; }
	}
	else
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}


	//Get handles to swapchain's images
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
}



void VKApp::CreateSwapchainImageViews()
{
	if (debug) { std::cout << "\n\n\nCreating Swapchain Image Views\n"; }

	swapchainImageViews.resize(swapchainImages.size());
	for (std::size_t i{ 0 }; i < swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (debug) { std::cout << std::left << std::setw(debugW) << "Creating image view " + std::to_string(i); }
		VkResult result = vkCreateImageView(device, &createInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &swapchainImageViews[i]);
		if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
		if (result != VK_SUCCESS)
		{
			if (debug) { std::cout << " (" << result << ")\n"; }
			Shutdown(true);
			return;
		}
	}
}



void VKApp::CreateRenderPass()
{
	if (debug) { std::cout << "\n\n\nCreating Render Pass\n"; }

	//Define attachment description
	if (debug) { std::cout << "Defining attachment description (colour)\n"; }
	VkAttachmentDescription colourAttachment{};
	colourAttachment.flags = 0;
	colourAttachment.format = swapchainImageFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No MSAA
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //Colour attachment - no stencil
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //Colour attachment - no stencil
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //Layout to transition state after pass is done (for presentation)

	//Define subpass description
	if (debug) { std::cout << "Defining attachment reference (colour)\n"; }
	VkAttachmentReference colourAttachmentRef{};
	colourAttachmentRef.attachment = 0; //Index into pAttachments array of VkRenderPassCreateInfo
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Define subpass
	if (debug) { std::cout << "Defining subpass description\n"; }
	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;

	//Define render pass
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colourAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating render pass"; }
	VkResult result = vkCreateRenderPass(device, &renderPassInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &renderPass);
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
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;
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

	//Define rasteriser state
	if (debug) { std::cout << "Defining rasteriser state\n"; }
	VkPipelineRasterizationStateCreateInfo rasteriser{};
	rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasteriser.pNext = nullptr;
	rasteriser.flags = 0;
	rasteriser.depthClampEnable = VK_FALSE;
	rasteriser.rasterizerDiscardEnable = VK_FALSE;
	rasteriser.polygonMode = VK_POLYGON_MODE_FILL;
	rasteriser.lineWidth = 1.0f;
	rasteriser.cullMode = VK_CULL_MODE_NONE;
	rasteriser.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasteriser.depthBiasEnable = VK_FALSE;
	rasteriser.depthBiasConstantFactor = 0.0f;
	rasteriser.depthBiasClamp = 0.0f;
	rasteriser.depthBiasSlopeFactor = 0.0f;

	//Define multisampling state (even though we're not using multisampling)
	if (debug) { std::cout << "Defining multisampling state\n"; }
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	//Define colour blend attachment (even though we're not using blending)
	if (debug) { std::cout << "Defining colour blend attachment\n"; }
	VkPipelineColorBlendAttachmentState colourBlendAttachment{};
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourBlendAttachment.blendEnable = VK_FALSE;
	colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	//Define colour blend state (even though we're not using blending)
	if (debug) { std::cout << "Defining colour blend state\n"; }
	VkPipelineColorBlendStateCreateInfo colourBlending{};
	colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlending.pNext = nullptr;
	colourBlending.flags = 0;
	colourBlending.logicOpEnable = VK_FALSE;
	colourBlending.logicOp = VK_LOGIC_OP_COPY;
	colourBlending.attachmentCount = 1;
	colourBlending.pAttachments = &colourBlendAttachment;
	colourBlending.blendConstants[0] = 0.0f;
	colourBlending.blendConstants[1] = 0.0f;
	colourBlending.blendConstants[2] = 0.0f;
	colourBlending.blendConstants[3] = 0.0f;

	//Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = 0;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasteriser;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colourBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	if (debug) { std::cout << std::left << std::setw(debugW) << "Creating graphics pipeline"; }
	VkResult result{ vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &pipeline) };
	if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
	if (result != VK_SUCCESS)
	{
		if (debug) { std::cout << " (" << result << ")\n"; }
		Shutdown(true);
		return;
	}
	
}



void VKApp::CreateSwapchainFramebuffers()
{
	if (debug) { std::cout << "\n\n\nCreating Swapchain Framebuffers\n"; }

	swapchainFramebuffers.resize(swapchainImageViews.size());
	for (std::size_t i{ 0 }; i < swapchainFramebuffers.size(); ++i)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &(swapchainImageViews[i]);
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (debug) { std::cout << std::left << std::setw(debugW) << "Creating framebuffer " + std::to_string(i); }
		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &swapchainFramebuffers[i]);
		if (debug) { std::cout << (result == VK_SUCCESS ? "success\n" : "failure"); }
		if (result != VK_SUCCESS)
		{
			if (debug) { std::cout << " (" << result << ")\n"; }
			Shutdown(true);
			return;
		}
	}
}



void VKApp::CreateSyncObjects()
{
	if (debug) { std::cout << "\n\n\nCreating Sync Objects\n"; }

	//Allow for 2 frames to be in-flight at once
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapchainImages.size());

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (std::size_t i{ 0 }; i<MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			std::cout << "Failed to create sync objects. Shutting down.\n";
			Shutdown(true);
			return;
		}
	}
	if (debug) { std::cout << "Successfully created sync objects for " << MAX_FRAMES_IN_FLIGHT << " frames.\n"; }
}



void VKApp::DrawFrame()
{
	// if (debug) { std::cout << "Drawing frame " << currentFrame << '\n'; }

	
	//Wait for previous frame to finish
	vkWaitForFences(device, 1, &(inFlightFences[currentFrame]), VK_TRUE, UINT64_MAX);

	std::uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	
	//Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];
	
	vkResetFences(device, 1, &(inFlightFences[currentFrame]));

	
	//Start recording the command buffer
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		std::cout << "Failed to begin command buffer. Shutting down\n";
		Shutdown(true);
		return;
	}

	
	//Define the render pass begin info
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapchainExtent;


	//Define the clear colour
	VkClearValue clearColour{};
	clearColour.color = {0.0f, 1.0f, 0.0f, 0.0f};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColour;

	
	vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	
	//Define viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0,0};
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

	
	//Draw the damn triangle
	vkCmdDraw(commandBuffers[currentFrame], 3, 1, 0, 0);


	//Stop recording commands
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
	{
		std::cout << "Failed to end command buffer. Shutting down\n";
		Shutdown(true);
		return;
	}


	//Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &(imageAvailableSemaphores[currentFrame]);
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &(renderFinishedSemaphores[imageIndex]);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		std::cout << "Failed to submit command buffer. Shutting down\n";
		Shutdown(true);
		return;
	}


	//Present the result
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &(renderFinishedSemaphores[imageIndex]);
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(graphicsQueue, &presentInfo);
	
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}



void VKApp::Shutdown(bool _throwError)
{
	std::cout << "\n\n\nShutting down\n";

	if (!inFlightFences.empty())
	{
		for (VkFence& f : inFlightFences)
		{
			if (f != VK_NULL_HANDLE)
			{
				vkDestroyFence(device, f, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
			}
		}
		std::cout << "In-Flight Fences Destroyed\n";
	}
	inFlightFences.clear();
	imagesInFlight.clear();

	if (!renderFinishedSemaphores.empty())
	{
		for (VkSemaphore& s : renderFinishedSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device, s, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
			}
		}
		std::cout << "Render-Finished Semaphores Destroyed\n";
	}
	renderFinishedSemaphores.clear();

	if (!imageAvailableSemaphores.empty())
	{
		for (VkSemaphore& s : imageAvailableSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device, s, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
			}
		}
		std::cout << "Image-Available Semaphores Destroyed\n";
	}
	imageAvailableSemaphores.clear();
	
	if (!swapchainFramebuffers.empty())
	{
		for (VkFramebuffer f : swapchainFramebuffers)
		{
			vkDestroyFramebuffer(device, f, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		}
		std::cout << "Swapchain Framebuffers Destroyed\n";
	}
	swapchainFramebuffers.clear();
	
	if (!swapchainImageViews.empty())
	{
		for (VkImageView v : swapchainImageViews)
		{
			vkDestroyImageView(device, v, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		}
		std::cout << "Swapchain Image Views Destroyed\n";
	}
	swapchainImageViews.clear();

	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, swapchain, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		swapchain = VK_NULL_HANDLE;
		std::cout << "Swapchain Destroyed\n";
	}
	
	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, pipeline, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		pipeline = VK_NULL_HANDLE;
		std::cout << "Pipeline Destroyed\n";
	}

	if (renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device, renderPass, debug ? static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator) : nullptr);
		renderPass = VK_NULL_HANDLE;
		std::cout << "Render Pass Destroyed\n";
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

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(inst, surface, nullptr); //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
		surface = VK_NULL_HANDLE;
		std::cout << "Surface Destroyed\n";
	}
	
	if (inst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(inst, debug ? static_cast<const VkAllocationCallbacks*>(instDebugAllocator) : nullptr);
		inst = VK_NULL_HANDLE;
		std::cout << "Instance Destroyed\n";
	}
	
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
		std::cout << "Window Destroyed\n";
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