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

struct VKRenderPassCleanDesc final
{
	std::uint32_t attachmentCount;
	VkAttachmentDescription* attachments;

	std::uint32_t subpassCount;
	VkSubpassDescription* subpasses;
};

class VulkanRenderManager
{
public:
	explicit VulkanRenderManager(const VKLogger& _logger,
							 VKDebugAllocator& _deviceDebugAllocator,
							 const VulkanDevice& _device,
							 VulkanCommandPool& _commandPool,
							 ImageFactory& _imageFactory,
							 VkExtent2D _windowSize,
							 std::size_t _framesInFlight,
							 VKRenderPassCleanDesc _renderPassDesc);

	~VulkanRenderManager();

	//Descriptions for a single-subpass render pass
	[[nodiscard]] static VkAttachmentDescription GetDefaultColourAttachmentDescription();
	[[nodiscard]] static VkAttachmentDescription GetDefaultDepthAttachmentDescription();
	[[nodiscard]] static VkSubpassDescription GetDefaultSubpassDescription(std::uint32_t _colourAttachmentCount, VkAttachmentReference* _colourAttachments, VkAttachmentReference* _depthStencilAttachment);
	
	void StartFrame(std::uint32_t _clearValueCount, const VkClearValue* _clearValues);
	void SubmitAndPresent();

	[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer();
	[[nodiscard]] VkExtent2D GetSwapchainExtent();
	[[nodiscard]] VkRenderPass GetRenderPass();
	[[nodiscard]] GLFWwindow* GetWindow();
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