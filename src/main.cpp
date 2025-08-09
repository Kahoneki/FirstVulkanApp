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

	VkDescriptorPoolSize descriptorPoolSizes[]{ {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1} };


	//Attachments
	VkAttachmentDescription attachments[]
	{
		Neki::VulkanRenderManager::GetDefaultOutputDepthAttachmentDescription(),
		Neki::VulkanRenderManager::GetDefaultOutputColourAttachmentDescription(),
		Neki::VulkanRenderManager::GetDefaultOutputColourAttachmentDescription(),
	};
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkFormat attachmentFormats[]{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED };
	Neki::FORMAT_TYPE attachmentTypes[]{ Neki::FORMAT_TYPE::DEPTH_NO_SAMPLING, Neki::FORMAT_TYPE::COLOUR_INPUT_ATTACHMENT, Neki::FORMAT_TYPE::SWAPCHAIN };

	//Subpass 0
	VkAttachmentReference depthAttachmentRef{ 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference colourAttachmentRef{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass{ Neki::VulkanRenderManager::GetDefaultSubpassDescription(&colourAttachmentRef, &depthAttachmentRef) };

	//Subpass 1
	VkAttachmentReference postprocessInputRef{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkAttachmentReference postprocessOutputRef{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription postprocessSubpass{};
	postprocessSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	postprocessSubpass.colorAttachmentCount = 1;
	postprocessSubpass.pColorAttachments = &postprocessOutputRef;
	postprocessSubpass.inputAttachmentCount = 1;
	postprocessSubpass.pInputAttachments = &postprocessInputRef;

	VkSubpassDescription subpasses[]{ subpass, postprocessSubpass };

	//this is so cool
	VkSubpassDependency dependency{};
	dependency.srcSubpass = 0;
	dependency.dstSubpass = 1;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	Neki::VKRenderPassCleanDesc renderPassDesc{};
	renderPassDesc.attachmentCount = 3;
	renderPassDesc.attachments = attachments;
	renderPassDesc.attachmentFormats = attachmentFormats;
	renderPassDesc.attachmentTypes = attachmentTypes;
	renderPassDesc.subpassCount = 2;
	renderPassDesc.subpasses = subpasses;
	renderPassDesc.dependencyCount = 1;
	renderPassDesc.dependencies = &dependency;

	Neki::GraphicsPipelineShaderFilepaths subpassPipelines[]{ {"shader.vert", "shader.frag"}, { "pp.vert", "pp.frag" } };

	//Define the clear colour
	VkClearValue clearValues[3];
	clearValues[0].depthStencil = { 1.0f, 0 };
	clearValues[1].color = {0.9f, 0.5f, 0.5f, 0.0f};
	clearValues[2].color = {0.0f, 0.0f, 0.0f, 0.0f};
	
	Neki::VKAppCreationDescription creationDescription{};
	creationDescription.windowSize = {1280, 720};
	creationDescription.renderPassDesc = renderPassDesc;
	creationDescription.subpassPipelines = subpassPipelines;
	creationDescription.clearValueCount = 3;
	creationDescription.clearValues = clearValues;
	creationDescription.descriptorPoolSizeCount = 3;
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