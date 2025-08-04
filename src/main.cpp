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
	
	VkAttachmentDescription attachments[]
	{
		Neki::VulkanRenderManager::GetDefaultColourAttachmentDescription(),
		Neki::VulkanRenderManager::GetDefaultDepthAttachmentDescription()
	};
	VkAttachmentReference colourAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthAttachmentRef{ 1, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass{ Neki::VulkanRenderManager::GetDefaultSubpassDescription(1, &colourAttachmentRef, &depthAttachmentRef) };
	Neki::VKRenderPassCleanDesc renderPassDesc{};
	renderPassDesc.attachmentCount = 2;
	renderPassDesc.attachments = attachments;
	renderPassDesc.subpassCount = 1;
	renderPassDesc.subpasses = &subpass;
	
	Neki::VKAppCreationDescription creationDescription{};
	creationDescription.windowSize = {1280, 720};
	creationDescription.renderPassDesc = renderPassDesc;
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