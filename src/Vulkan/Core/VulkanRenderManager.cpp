#include "VulkanRenderManager.h"
#include "../Debug/VKLogger.h"

#include <stdexcept>
#include <algorithm>


namespace Neki
{



VulkanRenderManager::VulkanRenderManager(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, VulkanCommandPool& _commandPool, ImageFactory& _imageFactory, VkExtent2D _windowSize, std::size_t _framesInFlight, VkRenderPassCreateInfo* _renderPassDesc)
										: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), commandPool(_commandPool), imageFactory(_imageFactory)
{
	windowSize = _windowSize;
	window = nullptr;
	surface = VK_NULL_HANDLE;
	swapchain = VK_NULL_HANDLE;
	swapchainImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
	depthTexture = VK_NULL_HANDLE;
	depthTextureView = VK_NULL_HANDLE;
	renderPass = VK_NULL_HANDLE;
	framesInFlight = _framesInFlight;
	imageIndex = 0;
	currentFrame = 0;

	if (windowSize.height == 0 || windowSize.width == 0)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Provided _windowSize (" + std::to_string(windowSize.width) + "x" + std::to_string(windowSize.height) + ") has a zero-length dimension.");
		throw std::runtime_error("");
	}

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Render Manager\n");

	CreateWindow();
	CreateSurface();
	AllocateCommandBuffers();
	CreateSwapchain();
	CreateSwapchainImageViews();
	InitialiseDepthTexture();

	if (_renderPassDesc == nullptr)
	{
		//Define colour attachment description
		DefaultRenderPassDescription renderPassDesc{};
		renderPassDesc.colourAttachment.flags = 0;
		renderPassDesc.colourAttachment.format = VkFormat::VK_FORMAT_UNDEFINED;
		renderPassDesc.colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No MSAA
		renderPassDesc.colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderPassDesc.colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderPassDesc.colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //Colour attachment - no stencil
		renderPassDesc.colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //Colour attachment - no stencil
		renderPassDesc.colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		renderPassDesc.colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //Layout to transition state after pass is done (for presentation)

		//Define depth attachment description
		renderPassDesc.depthAttachment.flags = 0;
		renderPassDesc.depthAttachment.format = VkFormat::VK_FORMAT_UNDEFINED;
		renderPassDesc.depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No MSAA
		renderPassDesc.depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderPassDesc.depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //Don't need depth after the frame
		renderPassDesc.depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		renderPassDesc.depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		renderPassDesc.depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		renderPassDesc.depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//Subpass attachment references
		renderPassDesc.colourAttachmentRef.attachment = 0; //Index into pAttachments array of VkRenderPassCreateInfo
		renderPassDesc.colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderPassDesc.depthAttachmentRef.attachment = 1;
		renderPassDesc.depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		//Define subpass
		renderPassDesc.subpass.flags = 0;
		renderPassDesc.subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		renderPassDesc.subpass.colorAttachmentCount = 1;
		renderPassDesc.subpass.pColorAttachments = &(renderPassDesc.colourAttachmentRef);
		renderPassDesc.subpass.pDepthStencilAttachment = &(renderPassDesc.depthAttachmentRef);

		//Define render pass
		renderPassDesc.attachmentDescriptions = { renderPassDesc.colourAttachment, renderPassDesc.depthAttachment };
		renderPassDesc.createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassDesc.createInfo.pNext = nullptr;
		renderPassDesc.createInfo.flags = 0;
		renderPassDesc.createInfo.dependencyCount = 0;
		renderPassDesc.createInfo.pDependencies = nullptr;
		renderPassDesc.createInfo.attachmentCount = static_cast<std::uint32_t>(renderPassDesc.attachmentDescriptions.size());
		renderPassDesc.createInfo.pAttachments = renderPassDesc.attachmentDescriptions.data();
		renderPassDesc.createInfo.subpassCount = 1;
		renderPassDesc.createInfo.pSubpasses = &renderPassDesc.subpass;
		CreateRenderPass(renderPassDesc.createInfo);
	}
	else
	{
		CreateRenderPass(*_renderPassDesc);
	}

	CreateSwapchainFramebuffers();
	CreateSyncObjects();
}



VulkanRenderManager::~VulkanRenderManager()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER,"Shutting down VulkanRenderManager\n");

	if (!inFlightFences.empty())
	{
		for (VkFence& f : inFlightFences)
		{
			if (f != VK_NULL_HANDLE)
			{
				vkDestroyFence(device.GetDevice(), f, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  In-Flight Fences Destroyed\n");
	}
	inFlightFences.clear();
	imagesInFlight.clear();

	if (!renderFinishedSemaphores.empty())
	{
		for (VkSemaphore& s : renderFinishedSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device.GetDevice(), s, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Render-Finished Semaphores Destroyed\n");
	}
	renderFinishedSemaphores.clear();

	if (!imageAvailableSemaphores.empty())
	{
		for (VkSemaphore& s : imageAvailableSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device.GetDevice(), s, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Image-Available Semaphores Destroyed\n");
	}
	imageAvailableSemaphores.clear();
	
	if (!swapchainFramebuffers.empty())
	{
		for (VkFramebuffer f : swapchainFramebuffers)
		{
			vkDestroyFramebuffer(device.GetDevice(), f, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Swapchain Framebuffers Destroyed\n");
	}
	swapchainFramebuffers.clear();
	
	if (!swapchainImageViews.empty())
	{
		for (VkImageView v : swapchainImageViews)
		{
			vkDestroyImageView(device.GetDevice(), v, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Swapchain Image Views Destroyed\n");
	}
	swapchainImageViews.clear();

	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device.GetDevice(), swapchain, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		swapchain = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Swapchain Destroyed\n");
	}

	if (renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device.GetDevice(), renderPass, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		renderPass = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Render Pass Destroyed\n");
	}

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(device.GetInstance(), surface, nullptr); //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
		surface = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Surface Destroyed\n");
	}

	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Window Destroyed\n");
	}
}



void VulkanRenderManager::StartFrame(std::uint32_t _clearValueCount, const VkClearValue* _clearValues)
{
	//Wait for previous frame of this frame index to finish before overwriting resources
	vkWaitForFences(device.GetDevice(), 1, &(inFlightFences[currentFrame]), VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(device.GetDevice(), swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device.GetDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	
	//Mark the image as now being in use by this frame - tying its synchronisation to this frame's fence
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	//Reset back to unsignalled
	vkResetFences(device.GetDevice(), 1, &(inFlightFences[currentFrame]));

	
	//Start recording the command buffer
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to begin command buffer\n");
		throw std::runtime_error("");
	}

	
	//Define the render pass begin info
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapchainExtent;
	renderPassInfo.clearValueCount = _clearValueCount;
	renderPassInfo.pClearValues = _clearValues;

	
	vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}



void VulkanRenderManager::SubmitAndPresent()
{
	//Stop recording commands
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to end command buffer\n");
		throw std::runtime_error("");
	}


	//Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	//Todo: get wait stages from render pass create info
	constexpr VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &(imageAvailableSemaphores[currentFrame]);
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &(renderFinishedSemaphores[imageIndex]);

	if (vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to submit command buffer\n");
		throw std::runtime_error("");
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

	vkQueuePresentKHR(device.GetGraphicsQueue(), &presentInfo);
	
	
	currentFrame = (currentFrame + 1) % framesInFlight;
}



VkCommandBuffer VulkanRenderManager::GetCurrentCommandBuffer()
{
	return commandBuffers[currentFrame];
}



VkExtent2D VulkanRenderManager::GetSwapchainExtent()
{
	return swapchainExtent;
}



VkRenderPass VulkanRenderManager::GetRenderPass()
{
	return renderPass;
}



bool VulkanRenderManager::WindowShouldClose() const
{
	return glfwWindowShouldClose(window);
}



void VulkanRenderManager::CreateWindow()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Window\n");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Don't create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //Don't let window be resized (for now)
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating window", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	window = glfwCreateWindow(windowSize.width, windowSize.height, "Vulkaneki", nullptr, nullptr);
	logger.Log(window ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, window ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (!window)
	{
		throw std::runtime_error("");
	}
}



void VulkanRenderManager::CreateSurface()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Surface\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating surface", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ glfwCreateWindowSurface(device.GetInstance(), window, nullptr, &surface) }; //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



void VulkanRenderManager::AllocateCommandBuffers()
{
	commandBuffers.resize(framesInFlight);
	commandBuffers = commandPool.AllocateCommandBuffers(framesInFlight);
}



void VulkanRenderManager::CreateSwapchain()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Swapchain\n");


	//Get supported formats
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Scanning for supported swapchain formats", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	
	//Query surface capabilities
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), surface, &capabilities);

	//Query supported formats
	std::uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats;
	if (formatCount != 0)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCount, formats.data());
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Found: " + std::to_string(formatCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (const VkSurfaceFormatKHR& format : formats)
	{
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "  Format: " + std::to_string(format.format) + " (Colour Space: " + std::to_string(format.colorSpace) + ")\n");
	}

	//Choose suitable format
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Selecting swapchain format (preferred: " + std::to_string(VK_FORMAT_B8G8R8A8_SRGB) + " - VK_FORMAT_B8G8R8A8_SRGB\n");
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
		swapchainExtent.width = std::clamp(static_cast<std::uint32_t>(windowSize.width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		swapchainExtent.height = std::clamp(static_cast<std::uint32_t>(windowSize.height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Selected swapchain format " + std::to_string(swapchainImageFormat) + "\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Selected swapchain extent " + std::to_string(swapchainExtent.width) + "x" + std::to_string(swapchainExtent.height) + "\n");


	//Get supported present modes
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Scanning for supported swapchain present modes", VK_LOGGER_WIDTH::SUCCESS_FAILURE);

	//Query supported present modes
	std::uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device.GetPhysicalDevice(), surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes;
	if (presentModeCount != 0)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device.GetPhysicalDevice(), surface, &presentModeCount, presentModes.data());
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Found: " + std::to_string(presentModeCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "  ", VK_LOGGER_WIDTH::DEFAULT, false);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Present mode: " + std::to_string(presentMode) + " (" + (presentMode == 0 ? "IMMEDIATE)\n" :
																																				(presentMode == 1 ? "MAILBOX)\n" :
																																				(presentMode == 2 ? "FIFO)\n" :
																																				(presentMode == 3 ? "FIFO_RELAXED\n" : ")\n")))));
	}
	
	//Choose suitable present mode
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Selecting swapchain present mode (preferred: " + std::to_string(VK_PRESENT_MODE_MAILBOX_KHR) + " - VK_PRESENT_MODE_MAILBOX_KHR)\n");
	VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR }; //Guaranteed to be available
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = availablePresentMode;
			break;
		}
	}
	logger.Log(presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::RENDER_MANAGER, "Selected swapchain present mode: " + std::to_string(presentMode) + "\n");


	//Get image count
	std::uint32_t imageCount{ capabilities.minImageCount + 1 }; //Get 1 more than minimum required by driver for a bit of wiggle room
	if (capabilities.maxImageCount >0 && imageCount > capabilities.maxImageCount) { imageCount = capabilities.maxImageCount; }
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Requesting " + std::to_string(imageCount) + " swapchain images\n");

	
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
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating swapchain", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result = vkCreateSwapchainKHR(device.GetDevice(), &createInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchain);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result == VK_SUCCESS)
	{
		//Get number of images in swapchain
		vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, nullptr);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, " (images: " + std::to_string(imageCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	else
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}


	//Get handles to swapchain's images
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, swapchainImages.data());
}



void VulkanRenderManager::CreateSwapchainImageViews()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Swapchain Image Views\n");

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

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating image view " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		VkResult result = vkCreateImageView(device.GetDevice(), &createInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchainImageViews[i]);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}
	}
}



void VulkanRenderManager::InitialiseDepthTexture()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Depth Texture\n");

	//Find a supported format
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "  Searching for available depth texture image format", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	depthTextureFormat = device.FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "success (format: " + std::to_string(depthTextureFormat) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "  Creating depth texture\n");
	depthTexture = imageFactory.AllocateImage(swapchainExtent, depthTextureFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "  Transitioning depth texture from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL\n");
	imageFactory.TransitionImage(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
								  VK_IMAGE_ASPECT_DEPTH_BIT,
								  0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
								  depthTexture);
	
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "  Creating depth texture view\n");
	depthTextureView = imageFactory.CreateImageView(depthTexture, depthTextureFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}



void VulkanRenderManager::CreateRenderPass(VkRenderPassCreateInfo& _renderPassCreateInfo)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Render Pass\n");
	
	//Loop through pAttachments and check format - if UNDEFINED, set to swapchain image format or depth format (depending on final layout)
	//Can't modify original array, so remake it
	std::vector<VkAttachmentDescription> attachments;
	if (_renderPassCreateInfo.pAttachments != nullptr)
	{
		attachments.assign(_renderPassCreateInfo.pAttachments, _renderPassCreateInfo.pAttachments + _renderPassCreateInfo.attachmentCount);
	}
	for (std::size_t i{ 0 }; i<attachments.size(); ++i)
	{
		if (_renderPassCreateInfo.pAttachments[i].format == VkFormat::VK_FORMAT_UNDEFINED)
		{
			if (_renderPassCreateInfo.pAttachments[i].finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			{
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Setting attachment " + std::to_string(i) + " to swapchain image format\n");
				attachments[i].format = swapchainImageFormat;
			}
			else if (_renderPassCreateInfo.pAttachments[i].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Setting attachment " + std::to_string(i) + " to depth format\n");
				attachments[i].format = depthTextureFormat;
			}
		}
	}
	_renderPassCreateInfo.pAttachments = attachments.data();

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating render pass ", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result = vkCreateRenderPass(device.GetDevice(), &_renderPassCreateInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &renderPass);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



void VulkanRenderManager::CreateSwapchainFramebuffers()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Swapchain Framebuffers\n");

	swapchainFramebuffers.resize(swapchainImageViews.size());
	for (std::size_t i{ 0 }; i < swapchainFramebuffers.size(); ++i)
	{
		VkImageView attachments[] { swapchainImageViews[i], depthTextureView };
		
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating framebuffer " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		VkResult result = vkCreateFramebuffer(device.GetDevice(), &framebufferInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchainFramebuffers[i]);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}
	}
}



void VulkanRenderManager::CreateSyncObjects()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Sync Objects\n");

	//Allow for MAX_FRAMES_IN_FLIGHT frames to be in-flight at once
	imageAvailableSemaphores.resize(framesInFlight);
	renderFinishedSemaphores.resize(framesInFlight);
	inFlightFences.resize(framesInFlight);
	imagesInFlight.resize(swapchainImages.size());

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (std::size_t i{ 0 }; i<framesInFlight; ++i)
	{
		if (vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device.GetDevice(), &fenceInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &inFlightFences[i]) != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to create sync objects\n");
			throw std::runtime_error("");
		}
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Successfully created sync objects for " + std::to_string(framesInFlight) + " frames\n");
}



}