#ifndef VKAPP_H
#define VKAPP_H

#include <vulkan/vulkan.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

	void Run();
	
private:
	void Init(const std::uint32_t _apiVer,
			  const char* _appName,
			  std::vector<const char*>* _desiredInstanceLayers,
			  std::vector<const char*>* _desiredInstanceExtensions,
			  std::vector<const char*>* _desiredDeviceLayers,
			  std::vector<const char*>* _desiredDeviceExtensions);

	//Init subfunctions
	void CreateWindow();
	
	void CreateInstance(const std::uint32_t _apiVer,
						const char* _appName,
						std::vector<const char*>* _desiredInstanceLayers,
						std::vector<const char*>* _desiredInstanceExtensions);

	void CreateSurface();
	
	void SelectPhysicalDevice();

	void CreateLogicalDevice(std::vector<const char*>* _desiredDeviceLayers,
							 std::vector<const char*>* _desiredDeviceExtensions);

	void CreateCommandPool();
	void AllocateCommandBuffers();

	void CreateBuffer();
	void PopulateBuffer();

	void CreateDescriptorSetLayout();
	void CreatePipelineLayout();
	void CreateDescriptorPool();
	void AllocateDescriptorSets();
	void UpdateDescriptorSets();
	void CreateShaderModules();
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateRenderPass();
	void CreatePipeline();
	void CreateSwapchainFramebuffers();
	void CreateSyncObjects();

	void DrawFrame();

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
	VkSurfaceKHR surface;
	std::vector<VkPhysicalDevice> physicalDevices;
	VkDevice device;
	VkQueue graphicsQueue;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkBuffer buffer;
	VkDeviceMemory bufferMemory;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;
	VkRenderPass renderPass;
	VkPipeline pipeline;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	//Index into `physicalDevices` that will be used to create the logical device - prefer discrete GPU -> iGPU -> CPU
	std::size_t physicalDeviceIndex;
	//The type of the selected physical device
	VkPhysicalDeviceType physicalDeviceType;

	//Queue family index that has graphics support
	std::size_t graphicsQueueFamilyIndex;

	//Debug allocators (used if debug=true)
	VKDebugAllocator instDebugAllocator;
	VKDebugAllocator deviceDebugAllocator;

	//The current frame being rendered
	std::size_t currentFrame;
	static constexpr std::size_t MAX_FRAMES_IN_FLIGHT{ 3 };
	

	//Window
	GLFWwindow* window;
};

}


#endif //VKAPP_H
