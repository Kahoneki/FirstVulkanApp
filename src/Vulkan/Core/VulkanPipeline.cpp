#include "VulkanPipeline.h"
#include <stdexcept>

namespace Neki
{



VulkanPipeline::VulkanPipeline(const VKLogger &_logger, VKDebugAllocator &_deviceDebugAllocator, const VulkanDevice &_device, const std::vector<VkDescriptorSetLayout>* _descriptorSetLayouts, const std::vector<VkPushConstantRange>* _pushConstantRanges)
									: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), descriptorSetLayouts(_descriptorSetLayouts), pushConstantRanges(_pushConstantRanges)
{
	layout = VK_NULL_HANDLE;
	pipeline = VK_NULL_HANDLE;

	CreatePipelineLayout();
}



VulkanPipeline::~VulkanPipeline()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE,"Shutting down VulkanPipeline\n");
	
	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device.GetDevice(), pipeline, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		pipeline = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::PIPELINE,"\tPipeline Destroyed\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}

	if (!shaderModules.empty())
	{
		for (VkShaderModule& m : shaderModules)
		{
			if (m != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device.GetDevice(), m, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::PIPELINE,"\tDescriptor Set Layouts Destroyed\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	shaderModules.clear();

	if (layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device.GetDevice(), layout, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		layout = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::PIPELINE,"\tPipeline Layout Destroyed\n", VK_LOGGER_WIDTH::DEFAULT, false);

	}
}



VkPipeline VulkanPipeline::GetPipeline()
{
	return pipeline;
}



void VulkanPipeline::CreatePipelineLayout()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "Creating Pipeline Layout\n");
	
	//Define pipeline layout for descriptor sets
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.setLayoutCount = descriptorSetLayouts->size();
	layoutInfo.pSetLayouts = descriptorSetLayouts->data();
	layoutInfo.pushConstantRangeCount = pushConstantRanges ? pushConstantRanges->size() : 0;
	layoutInfo.pPushConstantRanges = pushConstantRanges ? pushConstantRanges->data() : nullptr;

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Creating pipeline layout", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreatePipelineLayout(device.GetDevice(), &layoutInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &layout) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



}