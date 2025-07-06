#include "Vulkan/VKApp.h"
#include <iostream>

void VKTest()
{
	std::vector<const char*> desiredInstanceLayerNames{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_INSTANCE_LAYER" };
	std::vector<const char*> desiredInstanceExtensionNames{ "NOT_A_REAL_INSTANCE_EXTENSION" };
	std::vector<const char*> desiredDeviceLayerNames{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_DEVICE_LAYER" };
	std::vector<const char*> desiredDeviceExtensionNames{ "NOT_A_REAL_DEVICE_EXTENSION" };

	Neki::VKApp app
	(
		true,
		VK_MAKE_API_VERSION(0, 1, 4, 0),
		"Neki App",
		&desiredInstanceLayerNames,
		&desiredInstanceExtensionNames,
		&desiredDeviceLayerNames,
		&desiredDeviceExtensionNames
	);
}

int main()
{
	VKTest();

	//Pause
	int x;
	std::cin >> x;
}