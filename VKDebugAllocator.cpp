#include "VKDebugAllocator.h"
#include <cstdlib>
#include <iostream>
#include <cstring>

namespace Neki
{


VKDebugAllocator::VKDebugAllocator(bool _verbose)
{
	callbacks.pUserData = static_cast<void*>(this);
	callbacks.pfnAllocation = &Allocation;
	callbacks.pfnReallocation = &Reallocation;
	callbacks.pfnFree = &Free;

	verbose = _verbose;

	if (verbose) { std::cout << "VKDebugAllocator - Initialised.\n"; }
}


VKDebugAllocator::~VKDebugAllocator()
{
	//Check for memory leaks
	if (!allocationSizes.empty())
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::~VKDEBUGALLOCATOR::MEMORY_LEAK::ALLOCATOR_DESTRUCTED_WITH_" << std::to_string(allocationSizes.size()) << "_ALLOCATIONS_UNFREED" << std::endl;
	}

	if (verbose) { std::cout << "VKDebugAllocator - Destroyed.\n"; }
}




void* VKAPI_CALL VKDebugAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->Allocation(_size, _alignment, _allocationScope);
}

void* VKAPI_CALL VKDebugAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->Reallocation(_pOriginal, _size, _alignment, _allocationScope);
}

void VKAPI_CALL VKDebugAllocator::Free(void* _pUserData, void* _pMemory)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->Free(_pMemory);
}



void* VKDebugAllocator::Allocation(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	if (verbose) { std::cout << "VKDebugAllocator - Allocation called for " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }
	void* ptr{ nullptr };
	if (posix_memalign(&ptr, _alignment, _size) != 0)
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::ALLOCATION::ALLOCATION_FAILED::RETURNING_NULLPTR" << std::endl;
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(allocationSizesMtx);
	allocationSizes[ptr] = _size;
	
	return ptr;
}



void* VKDebugAllocator::Reallocation(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	if (verbose) { std::cout << "VKDebugAllocator - Reallocation called for address " << _pOriginal << " to the new pool of " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }

	if (_pOriginal == nullptr)
	{
		if (verbose) { std::cout << "VKDebugAllocator - Reallocation - _pOriginal == nullptr, falling back to VKDebugAllocator::Allocation\n"; }
		return Allocation(_size, _alignment, _allocationScope);
	}

	if (_size == 0)
	{
		if (verbose) { std::cout << "VKDebugAllocator - Reallocation - _size == 0, falling back to VKDebugAllocator::Free and returning nullptr\n"; }
		Free(_pOriginal);
		return nullptr;
	}
	
	//Get size of old allocation
	std::size_t oldSize;
	{
		std::lock_guard<std::mutex> lock(allocationSizesMtx);
		std::unordered_map<void*, std::size_t>::iterator it{ allocationSizes.find(_pOriginal) };
		if (it == allocationSizes.end())
		{
			std::cerr << "ERR::VKDEBUGALLOCATOR::REALLOCATION::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VKDEBUGALLOCATOR::RETURNING_NULLPTR" << std::endl;
			return nullptr;
		}
		oldSize = it->second;
	}

	//Allocate a new block
	void* newPtr{ Allocation(_size, _alignment, _allocationScope) };
	if (newPtr == nullptr)
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::REALLOCATION::ALLOCATION_RETURNED_NULLPTR" << std::endl;
	}

	//Copy data and free old block
	std::size_t bytesToCopy{ (oldSize < _size) ? oldSize : _size };
	memcpy(newPtr, _pOriginal, bytesToCopy);
	Free(_pOriginal);

	return newPtr;
}


void VKDebugAllocator::Free(void* _pMemory)
{
	if (verbose) { std::cout << "VKDebugAllocator - Free called for address " << _pMemory << "\n"; }

	if (_pMemory == nullptr)
	{
		if (verbose) { std::cout << "VKDebugAllocator - Free - _pMemory == nullptr, returning.\n"; }
		return;
	}
	
	//Remove from allocationSizes map
	std::lock_guard<std::mutex> lock(allocationSizesMtx);
	std::unordered_map<void*, std::size_t>::iterator it{ allocationSizes.find(_pMemory) };
	if (it == allocationSizes.end())
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::FREE::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VKDEBUGALLOCATOR::RETURNING" << std::endl;
		return;
	}
	allocationSizes.erase(it);

	free(_pMemory);
}



}