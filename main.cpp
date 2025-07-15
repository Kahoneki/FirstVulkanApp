#include <GLFW/glfw3.h>
#include "Vulkan/VKApp.h"
#include <iostream>

void VKTest()
{
	std::vector<const char*> desiredInstanceLayerNames{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_INSTANCE_LAYER" };
	std::vector<const char*> desiredInstanceExtensionNames{ "VK_KHR_surface", "NOT_A_REAL_INSTANCE_EXTENSION" };
	std::vector<const char*> desiredDeviceLayerNames{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_DEVICE_LAYER" };
	std::vector<const char*> desiredDeviceExtensionNames{ "VK_KHR_swapchain", "NOT_A_REAL_DEVICE_EXTENSION" };

	//Add necessary GLFW instance-extensions
	std::uint32_t glfwExtensionCount{ 0 };
	const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };
	desiredInstanceExtensionNames.insert(desiredInstanceExtensionNames.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
	
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

	app.Run();
}

int main()
{
	glfwInit();
	VKTest();
	glfwTerminate();
}