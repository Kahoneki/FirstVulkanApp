#ifndef BUFFERFACTORY_H
#define BUFFERFACTORY_H

#include "../Core/VulkanDevice.h"

//Responsible for the initialisation, ownership, and clean shutdown of VkBuffers and accompanying VkDeviceMemorys
namespace Neki
{



class BufferFactory
{
public:
	explicit BufferFactory(const VKLogger& _logger,
						   VKDebugAllocator& _deviceDebugAllocator,
						   const VulkanDevice& _device);

	~BufferFactory();

	//Allocate a single buffer
	[[nodiscard]] VkBuffer AllocateBuffer(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode=VK_SHARING_MODE_EXCLUSIVE, const VkMemoryPropertyFlags _requiredMemFlags=0);

	//Allocate multiple buffers from this pool
	[[nodiscard]] std::vector<VkBuffer> AllocateBuffers(std::uint32_t _count, const VkDeviceSize* _sizes, const VkBufferUsageFlags* _usages, const VkSharingMode* _sharingModes=nullptr, const VkMemoryPropertyFlags* _requiredMemFlags=nullptr);

	//Free a specific buffer
	void FreeBuffer(VkBuffer& _buffer);

	//Free a vector of buffers
	void FreeBuffers(std::vector<VkBuffer>& _buffers);


	[[nodiscard]] VkDeviceMemory GetMemory(VkBuffer _buffer);

	
private:
	[[nodiscard]] VkBuffer AllocateBufferImpl(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode, const VkMemoryPropertyFlags _requiredMemFlags);
	void FreeBufferImpl(VkBuffer& _buffer);
	
	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;

	std::unordered_map<VkBuffer, VkDeviceMemory> bufferMemoryMap;
};



}

#endif