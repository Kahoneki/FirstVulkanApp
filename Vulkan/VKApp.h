#ifndef VKAPP_H
#define VKAPP_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include "VKDebugAllocator.h"

namespace Neki
{

class VKApp final
{
public:
	explicit VKApp(const bool _debug=false,
				   const std::uint32_t _apiVer=VK_MAKE_API_VERSION(0,1,0,0),
				   const char* _appName="Vulkan App",
				   std::vector<const char*>* _desiredInstanceLayers=nullptr,
				   std::vector<const char*>* _desiredInstanceExtensions=nullptr,
				   std::vector<const char*>* _desiredDeviceLayers=nullptr,
				   std::vector<const char*>* _desiredDeviceExtensions=nullptr);

	~VKApp();
	
private:
	void Init(const std::uint32_t _apiVer,
			  const char* _appName,
			  std::vector<const char*>* _desiredInstanceLayers,
			  std::vector<const char*>* _desiredInstanceExtensions,
			  std::vector<const char*>* _desiredDeviceLayers,
			  std::vector<const char*>* _desiredDeviceExtensions);

	//Init subfunctions
	void CreateInstance(const std::uint32_t _apiVer,
						const char* _appName,
						std::vector<const char*>* _desiredInstanceLayers,
						std::vector<const char*>* _desiredInstanceExtensions);

	void SelectPhysicalDevice();

	void CreateLogicalDevice(std::vector<const char*>* _desiredDeviceLayers,
							 std::vector<const char*>* _desiredDeviceExtensions);

	void Shutdown(bool _throwError=false);

	//For debug print formatting
	constexpr static std::int32_t debugW{ 65 };
	constexpr static std::int32_t indxW{ 10 };
	constexpr static std::int32_t nameW{ 50 };
	constexpr static std::int32_t specW{ 15 };
	constexpr static std::int32_t implW{ 15 };
	constexpr static std::int32_t descW{ 40 };

	bool debug;

	//Vulkan resources
	VkInstance inst;
	std::vector<VkPhysicalDevice> physicalDevices;
	VkDevice device;

	//Index into `physicalDevices` that will be used to create the logical device - prefer discrete GPU -> iGPU -> CPU
	std::size_t physicalDeviceIndex;
	//The type of the selected physical device
	VkPhysicalDeviceType physicalDeviceType;

	//Queue family index that has graphics support
	std::size_t graphicsQueueFamilyIndex;

	//Debug allocators (used if debug=true)
	VKDebugAllocator instDebugAllocator;
	VKDebugAllocator deviceDebugAllocator;
};

}


#endif //VKAPP_H
