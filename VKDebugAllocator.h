#ifndef VKDEBUGALLOCATOR_H
#define VKDEBUGALLOCATOR_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>

namespace Neki
{



class VKDebugAllocator
{
public:
	VKDebugAllocator(bool _verbose=false);
	~VKDebugAllocator();

	explicit inline operator const VkAllocationCallbacks*() const
	{
		return &callbacks;
	}

private:
	VkAllocationCallbacks callbacks;

	static void* VKAPI_CALL Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	static void* VKAPI_CALL Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	static void VKAPI_CALL Free(void* _pUserData, void* _pMemory);
	//todo: maybe add internal allocation notification callbacks ?

	void* Allocation(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	void* Reallocation(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	void Free(void* _pMemory);

	//Stores the sizes of all allocations (required for reallocation)
	std::unordered_map<void*, std::size_t> allocationSizes;
	//Mutex for accessing allocationSizes in a thread safe manner
	std::mutex allocationSizesMtx;
	
	bool verbose;
};



}

#endif //VKDEBUGALLOCATOR_H
