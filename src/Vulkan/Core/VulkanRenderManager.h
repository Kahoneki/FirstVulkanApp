#ifndef VULKANRENDERMANAGER_H
#define VULKANRENDERMANAGER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanCommandPool.h"
#include "VulkanDevice.h"

//Responsible for the initialisation, ownership, and clean shutdown of a GLFWwindow, VkSurfaceKHR, VkSwapchainKHR, VkRenderPass, and all accompanying sync objects
namespace Neki
{

	//Only used for returning a memory-safe render pass create info in GetDefaultRenderPassCreateInfo
	struct DefaultRenderPassDescription
	{
		friend class VulkanRenderManager;
	public:
		VkRenderPassCreateInfo createInfo{};
	private:
		VkAttachmentDescription attachment{};
		VkSubpassDescription subpass{};
		VkAttachmentReference attachmentRef{};
	};

	class VulkanRenderManager
	{
	public:
		//_renderPassDesc should be fully filled out. To use the swapchain image format for attachments, set format to VkFormat::VK_FORMAT_UNDEFINED and it will be replaced
		explicit VulkanRenderManager(const VKLogger& _logger,
								 VKDebugAllocator& _deviceDebugAllocator,
								 const VulkanDevice& _device,
								 VulkanCommandPool& _commandPool,
								 VkExtent2D _windowSize,
								 std::size_t _framesInFlight,
								 VkRenderPassCreateInfo& _renderPassDesc);

		~VulkanRenderManager();
		
		void StartFrame(const VkClearValue& _clearValue);
		void SubmitAndPresent();

		[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer();
		[[nodiscard]] VkExtent2D GetSwapchainExtent();
		[[nodiscard]] VkRenderPass GetRenderPass();
		[[nodiscard]] bool WindowShouldClose() const;

		//Use Neki::VulkanRenderManager::GetDefaultRenderPassCreateInfo.createInfo to get a default create info that can be passed to VulkanRenderManager's constructor
		static DefaultRenderPassDescription GetDefaultRenderPassCreateInfo();

		
	private:
		void CreateWindow();
		void CreateSurface();
		void AllocateCommandBuffers();
		void CreateSwapchain();
		void CreateSwapchainImageViews();
		void CreateRenderPass(VkRenderPassCreateInfo& _renderPassCreateInfo);
		void CreateSwapchainFramebuffers();
		void CreateSyncObjects();

		//Dependency injections from VKApp
		const VKLogger& logger;
		VKDebugAllocator& deviceDebugAllocator;
		const VulkanDevice& device;
		VulkanCommandPool& commandPool;

		VkExtent2D windowSize;
		GLFWwindow* window;
		VkSurfaceKHR surface;
		
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