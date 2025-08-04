#ifndef VULKANRENDERMANAGER_H
#define VULKANRENDERMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "../Memory/ImageFactory.h"

//Responsible for the initialisation, ownership, and clean shutdown of a GLFWwindow, VkSurfaceKHR, VkSwapchainKHR, VkImage (depthTexture), VkImageView (depthTextureView), VkRenderPass, and all accompanying sync objects
namespace Neki
{

//For internal use only
struct DefaultRenderPassDescription
{
	friend class VulkanRenderManager;

public:
	VkRenderPassCreateInfo createInfo{};

	
private:
	VkAttachmentDescription colourAttachment{};
	VkAttachmentReference colourAttachmentRef{};

	VkAttachmentDescription depthAttachment{};
	VkAttachmentReference depthAttachmentRef{};
	
	VkSubpassDescription subpass{};
	std::vector<VkAttachmentDescription> attachmentDescriptions; //Will be populated by colourAttachment and depthAttachment
};

class VulkanRenderManager
{
public:
	//_renderPassDesc should, if provided, be fully filled out. To use the swapchain image or depth formats for attachments, set format to VkFormat::VK_FORMAT_UNDEFINED and it will be replaced
	//Alternatively, leave _renderPassDesc as nullptr to use a default render pass
	explicit VulkanRenderManager(const VKLogger& _logger,
							 VKDebugAllocator& _deviceDebugAllocator,
							 const VulkanDevice& _device,
							 VulkanCommandPool& _commandPool,
							 ImageFactory& _imageFactory,
							 VkExtent2D _windowSize,
							 std::size_t _framesInFlight,
							 VkRenderPassCreateInfo* _renderPassDesc=nullptr);

	~VulkanRenderManager();
	
	void StartFrame(std::uint32_t _clearValueCount, const VkClearValue* _clearValues);
	void SubmitAndPresent();

	[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer();
	[[nodiscard]] VkExtent2D GetSwapchainExtent();
	[[nodiscard]] VkRenderPass GetRenderPass();
	[[nodiscard]] bool WindowShouldClose() const;

	
private:
	void CreateWindow();
	void CreateSurface();
	void AllocateCommandBuffers();
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void InitialiseDepthTexture();
	void CreateRenderPass(VkRenderPassCreateInfo& _renderPassCreateInfo);
	void CreateSwapchainFramebuffers();
	void CreateSyncObjects();

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	VulkanCommandPool& commandPool;
	ImageFactory& imageFactory;

	VkExtent2D windowSize;
	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkImage depthTexture;
	VkFormat depthTextureFormat;
	VkImageView depthTextureView;
	
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	std::uint32_t imageIndex;
	std::size_t currentFrame;
	std::size_t framesInFlight;
	
	std::vector<VkCommandBuffer> commandBuffers;
};
}

#endif