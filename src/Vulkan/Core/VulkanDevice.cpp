#include "VulkanDevice.h"
#include "../Debug/VKLogger.h"

#include <format>
#include <cstring>

#include "../../Utils/Strings/format.h"



namespace Neki
{

std::string PhysicalDeviceTypeToString(const VkPhysicalDeviceType& _type)
{
	switch (_type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "Other";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
		default:                                     return "Unknown";
	}
}



VulkanDevice::VulkanDevice(const VKLogger& _logger,
                           VKDebugAllocator& _instDebugAllocator,
                           VKDebugAllocator& _deviceDebugAllocator,
                           const std::uint32_t _apiVer,
                           const char* _appName,
                           std::vector<const char*>* _desiredInstanceLayers,
                           std::vector<const char*>* _desiredInstanceExtensions,
                           std::vector<const char*>* _desiredDeviceLayers,
                           std::vector<const char*>* _desiredDeviceExtensions)
					: logger(_logger), instDebugAllocator(_instDebugAllocator), deviceDebugAllocator(_deviceDebugAllocator)
{
	inst = VK_NULL_HANDLE;
	physicalDevice = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	graphicsQueue = VK_NULL_HANDLE;
	CreateInstance(_apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions);
	SelectPhysicalDevice();
	CreateLogicalDevice(_desiredDeviceLayers, _desiredDeviceExtensions);
}



void VulkanDevice::CreateInstance(const std::uint32_t _apiVer, const char* _appName, std::vector<const char*>* _desiredInstanceLayers, std::vector<const char*>* _desiredInstanceExtensions)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "Creating Instance\n");

	//If a name is in a `desired...` vector and is available, it will be added to its corresponding `...ToBeAdded` vector
	std::vector<const char*> instanceLayerNamesToBeAdded;
	std::vector<const char*> instanceExtensionNamesToBeAdded;

	//Enumerate available instance layers
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Scanning for available instance-level layers", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	std::uint32_t instanceLayerCount{ 0 };
	VkResult result{ vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result == VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE," (found: " + std::to_string(instanceLayerCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	else
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
	std::vector<VkLayerProperties> instanceLayers;
	instanceLayers.resize(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

	static constexpr std::uint32_t indxW{ 10 };
	static constexpr std::uint32_t nameW{ 50 };
	static constexpr std::uint32_t specW{ 15 };
	static constexpr std::uint32_t implW{ 15 };
	static constexpr std::uint32_t descW{ 40 };
	
	std::string layerTableStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}\n",
									  "Index", indxW, "Name", nameW, "Spec Ver", specW, "Impl Ver", implW, "Description", descW) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, layerTableStr, VK_LOGGER_WIDTH::DEFAULT, false);

	//Create hyphen underline
	std::string layerUnderlineStr{ std::format("{0:-<{1}}\n", "", indxW + nameW + specW + implW + descW) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, layerUnderlineStr, VK_LOGGER_WIDTH::DEFAULT, false);

	for (std::size_t i{ 0 }; i < instanceLayerCount; ++i)
	{
		std::string entryStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}\n",
										  i, indxW,
										  instanceLayers[i].layerName, nameW,
										  instanceLayers[i].specVersion, specW,
										  instanceLayers[i].implementationVersion, implW,
										  instanceLayers[i].description, descW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, entryStr, VK_LOGGER_WIDTH::DEFAULT, false);

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
	
	for (const std::string& layerName : instanceLayerNamesToBeAdded)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE, "Added " + layerName + " to instance creation\n");
	}
	if (_desiredInstanceLayers != nullptr)
	{
		for (const std::string& layerName : *_desiredInstanceLayers)
		{
			logger.Log(VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::DEVICE, "Failed to add " + layerName + " to instance creation - layer does not exist or is unavailable\n");
		}
	}


	//Enumerate available instance extensions
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Scanning for available instance-level extensions", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	std::uint32_t instanceExtensionCount{ 0 };
	result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result == VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE," (found: " + std::to_string(instanceExtensionCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	else
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
		return;
	}

	std::vector<VkExtensionProperties> instanceExtensions;
	instanceExtensions.resize(instanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());

	std::string extensionTableStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}\n",
								  "Index", indxW, "Name", nameW, "Spec Ver", specW)};
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, extensionTableStr, VK_LOGGER_WIDTH::DEFAULT, false);

	//Create hyphen underline
	std::string extensionUnderlineStr{ std::format("{0:-<{1}}\n", "", indxW + nameW + specW) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, extensionUnderlineStr, VK_LOGGER_WIDTH::DEFAULT, false);

	for (std::size_t i{ 0 }; i < instanceExtensionCount; ++i)
	{

		std::string entryStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}\n",
								  i, indxW,
								  instanceExtensions[i].extensionName, nameW,
								  instanceExtensions[i].specVersion, specW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, entryStr, VK_LOGGER_WIDTH::DEFAULT, false);

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

	for (const std::string& extensionName : instanceExtensionNamesToBeAdded)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE, "Added " + extensionName + " to instance creation\n");
	}
	if (_desiredInstanceExtensions != nullptr)
	{
		for (const std::string& extensionName : *_desiredInstanceExtensions)
		{
			logger.Log(VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::DEVICE, "Failed to add " + extensionName + " to instance creation - layer does not exist or is unavailable\n");
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

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Creating vulkan instance", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	result = vkCreateInstance(&vkInstInfo, static_cast<const VkAllocationCallbacks*>(instDebugAllocator), &inst);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
		return;
	}
}



void VulkanDevice::SelectPhysicalDevice()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "Selecting Physical Device\n");

	//Enumerate physical devices
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Scanning for physical vulkan-compatible devices", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	std::uint32_t physicalDeviceCount{ 0 };
	VkResult result{ vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, nullptr) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result == VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE," (found: " + std::to_string(physicalDeviceCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	else
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;
	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices.resize(physicalDeviceCount);
	physicalDeviceProperties.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(inst, &physicalDeviceCount, physicalDevices.data());
	physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM; //Stores the best device type that has currently been found

	static constexpr std::uint32_t indxW{ 15 };
	static constexpr std::uint32_t nameW{ 50 };
	static constexpr std::uint32_t apiW{ 15 };
	static constexpr std::uint32_t driverW{ 15 };
	static constexpr std::uint32_t vendW{ 15 };
	static constexpr std::uint32_t idW{ 15 };
	static constexpr std::uint32_t typeW{ 15 };

	std::string deviceInfoTableStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}{10:<{11}}{12:<{13}}\n",
	                                            "Device Index", indxW, "Name", nameW, "API Version", apiW, "Driver Version", driverW, "Vendor ID", vendW, "Device ID", idW, "Type", typeW) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, deviceInfoTableStr, VK_LOGGER_WIDTH::DEFAULT, false);

	std::string deviceUnderlineStr{ std::format("{0:-<{1}}\n", "", indxW + nameW + apiW + driverW + vendW + idW + typeW) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, deviceUnderlineStr, VK_LOGGER_WIDTH::DEFAULT, false);
	
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties[i]);

		std::string entryStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}{10:<{11}}{12:<{13}}\n",
								  i, indxW,
								  std::string(physicalDeviceProperties[i].deviceName), nameW,
								  physicalDeviceProperties[i].apiVersion, apiW,
								  physicalDeviceProperties[i].driverVersion, driverW,
								  physicalDeviceProperties[i].vendorID, vendW,
								  physicalDeviceProperties[i].deviceID, idW,
								  PhysicalDeviceTypeToString(physicalDeviceProperties[i].deviceType), typeW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, entryStr, VK_LOGGER_WIDTH::DEFAULT, false);
		
		if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			physicalDevice = physicalDevices[i];
			physicalDeviceIndex = i;
		}
		else if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			if (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
				physicalDevice = physicalDevices[i];
				physicalDeviceIndex = i;
			}
		}
		else if (physicalDeviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
		{
			if ((physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
			{
				physicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
				physicalDevice = physicalDevices[i];
				physicalDeviceIndex = i;
			}
		}
	}

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Memory Information:\n");
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProps);

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\tDevice: " + std::to_string(i) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\tAvailable heaps: " + std::to_string(memProps.memoryHeapCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
		for (std::size_t j{ 0 }; j < memProps.memoryHeapCount; ++j)
		{
			std::string heapDescStr{ "\t\tHeap " + std::to_string(j) + " (" + ((memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "VRAM" : "RAM") + ", " + GetFormattedSizeString(memProps.memoryHeaps[j].size) + ")\n" };
			logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, heapDescStr, VK_LOGGER_WIDTH::DEFAULT, false);
		}
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\tAvailable memory types: " + std::to_string(memProps.memoryTypeCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
		for (std::size_t j{ 0 }; j < memProps.memoryTypeCount; ++j)
		{
			std::string memTypePropFlagsString{ "\t\tMemory type " + std::to_string(j) + " (Heap " + std::to_string(memProps.memoryTypes[j].heapIndex) + ", " };
			if (memProps.memoryTypes[j].propertyFlags == 0)
			{
				memTypePropFlagsString += "Base)\n";
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, memTypePropFlagsString, VK_LOGGER_WIDTH::DEFAULT, false);
				continue;
			}
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) { memTypePropFlagsString += "DEVICE_LOCAL | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { memTypePropFlagsString += "HOST_VISIBLE | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) { memTypePropFlagsString += "HOST_COHERENT | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) { memTypePropFlagsString += "HOST_CACHED | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) { memTypePropFlagsString += "LAZILY_ALLOCATED | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) { memTypePropFlagsString += "PROTECTED | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) { memTypePropFlagsString += "DEVICE_COHERENT_AMD | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) { memTypePropFlagsString += "DEVICE_UNCACHED_AMD | "; }
			if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) { memTypePropFlagsString += "RDMA_CAPABLE_NV | "; }
			memTypePropFlagsString.resize(memTypePropFlagsString.length() - 3);
			memTypePropFlagsString += ")\n";
			logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, memTypePropFlagsString, VK_LOGGER_WIDTH::DEFAULT, false);
		}

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Queue Information:\n");
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{
		//Enumerate queue families
		std::uint32_t queueFamilyCount;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilyProperties.data());
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\tDevice: " + std::to_string(i) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\tQueue Families: " + std::to_string(queueFamilyCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);

		for (std::size_t j{ 0 }; j < queueFamilyCount; ++j)
		{
			std::string queueFamilyFlagsString{ "\t\tQueue family " + std::to_string(j) + " ( " + std::to_string(queueFamilyProperties[j].queueCount) + "queues, " };
			if (queueFamilyProperties[j].queueFlags == 0)
			{
				queueFamilyFlagsString += "Base)\n";
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, queueFamilyFlagsString, VK_LOGGER_WIDTH::DEFAULT, false);
				continue;
			}
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queueFamilyFlagsString += "GRAPHICS | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) { queueFamilyFlagsString += "COMPUTE | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) { queueFamilyFlagsString += "TRANSFER | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) { queueFamilyFlagsString += "SPARSE_BINDING | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_PROTECTED_BIT) { queueFamilyFlagsString += "PROTECTED | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) { queueFamilyFlagsString += "VIDEO_DECODE_KHR | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) { queueFamilyFlagsString += "VIDEO_ENCODE_KHR | "; }
			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) { queueFamilyFlagsString += "OPTICAL_FLOW_NV | "; }
			if (!queueFamilyFlagsString.empty())
			{
				queueFamilyFlagsString.resize(queueFamilyFlagsString.length() - 3);
			}
			queueFamilyFlagsString += ", " + std::to_string(queueFamilyProperties[j].timestampValidBits) + " valid timestamp bits)\n";
		}

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	
	for (std::size_t i{ 0 }; i < physicalDeviceCount; ++i)
	{

		//Enumerate available device layers
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Scanning for available device-level layers for device " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		std::uint32_t deviceLayerCount{ 0 };
		result = vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, nullptr);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result == VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE," (found: " + std::to_string(deviceLayerCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		}
		else
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}

		std::vector<VkLayerProperties> deviceLayers;
		deviceLayers.resize(deviceLayerCount);
		vkEnumerateDeviceLayerProperties(physicalDevices[i], &deviceLayerCount, deviceLayers.data());
		static constexpr std::uint32_t indxW{ 10 };
		static constexpr std::uint32_t nameW{ 50 };
		static constexpr std::uint32_t specW{ 15 };
		static constexpr std::uint32_t implW{ 15 };
		static constexpr std::uint32_t descW{ 40 };
	
		std::string layerTableStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}\n",
										  "Index", indxW, "Name", nameW, "Spec Ver", specW, "Impl Ver", implW, "Description", descW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, layerTableStr, VK_LOGGER_WIDTH::DEFAULT, false);

		//Create hyphen underline
		std::string layerUnderlineStr{ std::format("{0:-<{1}}\n", "", indxW + nameW + specW + implW + descW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, layerUnderlineStr, VK_LOGGER_WIDTH::DEFAULT, false);
		
		for (std::size_t j{ 0 }; j < deviceLayerCount; ++j)
		{
			std::string entryStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}{6:<{7}}{8:<{9}}\n",
											  j, indxW,
											  deviceLayers[j].layerName, nameW,
											  deviceLayers[j].specVersion, specW,
											  deviceLayers[j].implementationVersion, implW,
											  deviceLayers[j].description, descW) };
			logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, entryStr, VK_LOGGER_WIDTH::DEFAULT, false);
		}

		
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\n", VK_LOGGER_WIDTH::DEFAULT, false);


		//Enumerate available device extensions
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Scanning for available device-level extensions for device " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		std::uint32_t deviceExtensionCount{ 0 };
		result = vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, nullptr);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result == VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE," (found: " + std::to_string(deviceExtensionCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		}
		else
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}

		std::vector<VkExtensionProperties> deviceExtensions;
		deviceExtensions.resize(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevices[i], nullptr, &deviceExtensionCount, deviceExtensions.data());

		std::string extensionTableStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}\n",
									  "Index", indxW, "Name", nameW, "Spec Ver", specW)};
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, extensionTableStr, VK_LOGGER_WIDTH::DEFAULT, false);

		//Create hyphen underline
		std::string extensionUnderlineStr{ std::format("{0:-<{1}}\n", "", indxW + nameW + specW) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, extensionUnderlineStr, VK_LOGGER_WIDTH::DEFAULT, false);
		
		for (std::size_t j{ 0 }; j < deviceExtensionCount; ++j)
		{
			std::string entryStr{ std::format("{0:<{1}}{2:<{3}}{4:<{5}}\n",
									  j, indxW,
									  deviceExtensions[j].extensionName, nameW,
									  deviceExtensions[j].specVersion, specW) };
			logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, entryStr, VK_LOGGER_WIDTH::DEFAULT, false);
		}

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
}



void VulkanDevice::CreateLogicalDevice(std::vector<const char *> *_desiredDeviceLayers, std::vector<const char *> *_desiredDeviceExtensions)
{
	//If a name is in a `desired...` vector and is available, it will be added to its corresponding `...ToBeAdded` vector
	std::vector<const char*> deviceLayerNamesToBeAdded;
	std::vector<const char*> deviceExtensionNamesToBeAdded;

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE, "Creating Logical Device\n");

	if ((physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) && (physicalDeviceType != VK_PHYSICAL_DEVICE_TYPE_CPU))
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, "No available physical device fitting type requirements! Available types: Discrete GPU, Integrated GPU, CPU.\n");
		throw std::runtime_error("");
	}

	//Get graphics queue family index for chosen device
	std::uint32_t queueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
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
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, "Chosen device does not provide a queue family with graphics queue bit enabled!");
		throw std::runtime_error("");
	}

	//Get desired layers for chosen device
	std::uint32_t deviceLayerCount{ 0 };
	VkResult result{ vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, nullptr) };
	std::vector<VkLayerProperties> deviceLayers;
	deviceLayers.resize(deviceLayerCount);
	vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, deviceLayers.data());
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
	for (const std::string& layerName : deviceLayerNamesToBeAdded)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE, "Added " + layerName + " to device creation\n");
	}
	if (_desiredDeviceLayers != nullptr)
	{
		for (const std::string& layerName : *_desiredDeviceLayers)
		{
			logger.Log(VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::DEVICE, "Failed to add " + layerName + " to device creation - layer does not exist or is unavailable\n");
		}
	}

	//Get desired extensions for chosen device
	std::uint32_t deviceExtensionCount{ 0 };
	result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> deviceExtensions;
	deviceExtensions.resize(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());
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
	for (const std::string& extensionName : deviceExtensionNamesToBeAdded)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE, "Added " + extensionName + " to device creation\n");
	}
	if (_desiredDeviceExtensions != nullptr)
	{
		for (const std::string& extensionName : *_desiredDeviceExtensions)
		{
			logger.Log(VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::DEVICE, "Failed to add " + extensionName + " to device creation - extension does not exist or is unavailable\n");

		}
	}


	VkPhysicalDeviceFeatures supportedFeatures;
	VkPhysicalDeviceFeatures requiredFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
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

	std::string deviceTypeName{ physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "DISCRETE_GPU" : (physicalDeviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "INTEGRATED_GPU" : "CPU") };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Creating vulkan logical device for device " + std::to_string(physicalDeviceIndex) + " (" + deviceTypeName + ")", VK_LOGGER_WIDTH::SUCCESS_FAILURE);

	result = vkCreateDevice(physicalDevice, &deviceCreateInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &device);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
		return;
	}

	//Get the queue handle
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DEVICE, "Getting queue handle\n");
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}



VulkanDevice::~VulkanDevice()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DEVICE,"Shutting down VulkanDevice\n");
	
	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);
		vkDestroyDevice(device, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		device = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE,"\tDevice Destroyed\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}

	if (inst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(inst, static_cast<const VkAllocationCallbacks*>(instDebugAllocator));
		inst = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DEVICE,"\tInstance Destroyed\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
}



const VkInstance& VulkanDevice::GetInstance() const { return inst; }
const VkPhysicalDevice& VulkanDevice::GetPhysicalDevice() const { return physicalDevice; }
const VkDevice& VulkanDevice::GetDevice() const { return device; }
const VkQueue& VulkanDevice::GetGraphicsQueue() const { return graphicsQueue; }
const std::size_t& VulkanDevice::GetGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }

}