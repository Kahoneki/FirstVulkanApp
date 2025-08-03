#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H
#include "BufferFactory.h"
#include "../../Utils/Loaders/ImageLoader.h"
#include "../Core/VulkanCommandPool.h"


//Responsible for the initialisation, ownership, and clean shutdown of VkImages and accompanying VkDeviceMemorys, VkImageViews, and VkSamplers
namespace Neki
{



class ImageFactory
{
public:
	explicit ImageFactory(const VKLogger& _logger,
						  VKDebugAllocator& _deviceDebugAllocator,
						  const VulkanDevice& _device,
						  VulkanCommandPool& _commandPool,
						  BufferFactory& _bufferFactory);

	~ImageFactory();


	//----IMAGES----//
	
	//Allocate a single image on a device local heap (passed through a intermediate staging buffer)
	//Optionally, pass an ImageData pointer to get metadata about the image
	[[nodiscard]] VkImage AllocateImage(const char* _filepath, const VkImageUsageFlags _flags, ImageMetadata* _out_metadata=nullptr);

	//Allocate a vector of _count images on a device local heap (passed through intermediate staging buffers)
	//Optionally, pass a list of _count ImageDatas to get metadata about the images
	[[nodiscard]] std::vector<VkImage> AllocateImages(std::uint32_t _count, const char** _filepaths, const VkImageUsageFlags* _flags, ImageMetadata* _out_metadata=nullptr);
	
	//Free a specific image
	void FreeImage(VkImage& _image);

	//Free a list of _count images
	void FreeImages(std::uint32_t _count, VkImage* _images);

	//--------//


	
	//----IMAGE VIEWS----//
	
	//Create a single image view for _image of format _format
	[[nodiscard]] VkImageView CreateImageView(const VkImage& _image, const VkFormat& _format);

	//Create a vector of _count image views for _images of formats _formats
	[[nodiscard]] std::vector<VkImageView> CreateImageViews(std::uint32_t _count, const VkImage* _images, const VkFormat* _formats);
	
	//Free a specific image view
	void FreeImageView(VkImageView& _imageView);

	//Free a list of _count image views
	void FreeImageViews(std::uint32_t _count, VkImageView* _imageViews);

	//--------//

	

	//----SAMPLERS----//
	
	//Create a single sampler
	[[nodiscard]] VkSampler CreateSampler(const VkSamplerCreateInfo& _createInfo);

	//Create a vector of _count samplers
	[[nodiscard]] std::vector<VkSampler> CreateSamplers(std::uint32_t _count, const VkSamplerCreateInfo* _createInfos);
	
	//Free a specific sampler
	void FreeSampler(VkSampler& _sampler);

	//Free a list of _count samplers
	void FreeSampler(std::uint32_t _count, VkSampler* _samplers);

	//--------//


private:
	[[nodiscard]] VkImage AllocateImageImpl(const char* _filepath, const VkImageUsageFlags _flags, ImageMetadata* _out_metadata);
	void FreeImageImpl(VkImage& _image);

	[[nodiscard]] VkImageView CreateImageViewImpl(const VkImage& _image, const VkFormat& _format);
	void FreeImageViewImpl(VkImageView& _imageView);

	[[nodiscard]] VkSampler CreateSamplerImpl(const VkSamplerCreateInfo& _createInfo);
	void FreeSamplerImpl(VkSampler& _sampler);

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	BufferFactory& bufferFactory;
	VulkanCommandPool& commandPool;

	std::unordered_map<VkImage, VkDeviceMemory> imageMemoryMap;
	std::unordered_map<VkImageView, VkImage> imageViewImageMap;
	std::vector<VkSampler> samplers;
};



}



#endif