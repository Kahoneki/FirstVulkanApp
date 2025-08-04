#include <GLFW/glfw3.h>
#include "Vulkan/VKApp.h"
#include <iostream>

#include "Managers/Application.h"

void AppTest()
{
	const char* desiredInstanceLayerNames[]{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_INSTANCE_LAYER" };
	const char* desiredInstanceExtensionNames[]{ "VK_KHR_surface", "NOT_A_REAL_INSTANCE_EXTENSION" };
	const char* desiredDeviceLayerNames[]{ "VK_LAYER_KHRONOS_validation", "NOT_A_REAL_DEVICE_LAYER" };
	const char* desiredDeviceExtensionNames[]{ "VK_KHR_swapchain", "NOT_A_REAL_DEVICE_EXTENSION" };

	Neki::VKLoggerConfig loggerConfig{ true };

	VkDescriptorPoolSize descriptorPoolSizes[]{ {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
	
	Neki::VKAppCreationDescription creationDescription{};
	creationDescription.windowSize = {1280, 720};
	creationDescription.renderPassDesc = nullptr;
	creationDescription.descriptorPoolSizeCount = 2;
	creationDescription.descriptorPoolSizes = descriptorPoolSizes;
	creationDescription.apiVer = VK_MAKE_API_VERSION(0, 1, 4, 0);
	creationDescription.appName = "Neki App";
	creationDescription.loggerConfig = &loggerConfig;
	creationDescription.allocatorType = Neki::VK_ALLOCATOR_TYPE::DEBUG;
	creationDescription.desiredInstanceLayerCount = std::size(desiredInstanceLayerNames);
	creationDescription.desiredInstanceLayers = desiredInstanceLayerNames;
	creationDescription.desiredInstanceExtensionCount = std::size(desiredInstanceExtensionNames);
	creationDescription.desiredInstanceExtensions = desiredInstanceExtensionNames;
	creationDescription.desiredDeviceLayerCount = std::size(desiredDeviceLayerNames);
	creationDescription.desiredDeviceLayers = desiredDeviceLayerNames;
	creationDescription.desiredDeviceExtensionCount = std::size(desiredDeviceExtensionNames);
	creationDescription.desiredDeviceExtensions = desiredDeviceExtensionNames;

	Neki::Application app(creationDescription);
	
	app.Start();
}

int main()
{
	glfwInit();
	AppTest();
	glfwTerminate();
}