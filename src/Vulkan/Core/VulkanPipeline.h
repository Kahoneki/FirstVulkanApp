#ifndef VULKANPIPELINE_H
#define VULKANPIPELINE_H

#include "VulkanDevice.h"

//Responsible for the initialisation, ownership, and clean shutdown of a VkPipelineLayout, a VkPipeline, and VkShaderModules
//Pure virtual class - must be created as inherited class
//Inheriting classes only have to override CreateShaderModules()
namespace Neki
{

	//VkPipelineCreateInfo has a bunch of boilerplate which is unnecessary for 99% of applications, this makes that process a lot easier
	//Polymorphic base class - inherited by VKGraphicsPipelineCleanDesc and VKComputePipelineCleanDesc - these are to be passed to the constructors of VKGraphicsPipeline or VKComputePipeline respectively.
	struct VKPipelineCleanDesc { virtual ~VKPipelineCleanDesc() = default; };



	class VulkanPipeline
	{
	public:
		VulkanPipeline(const VKLogger& _logger,
					   VKDebugAllocator& _deviceDebugAllocator,
					   const VulkanDevice& _device,
					   const std::vector<VkDescriptorSetLayout>* _descriptorSetLayouts=nullptr,
					   const std::vector<VkPushConstantRange>* _pushConstantRanges=nullptr);

		virtual ~VulkanPipeline();

		[[nodiscard]] VkPipeline GetPipeline();
		[[nodiscard]] VkPipelineLayout GetPipelineLayout();
		
	protected:
		//Called in VulkanPipeline's constructor
		void CreatePipelineLayout();

		//Called in derived class' constructors
		virtual void CreateShaderModules(const std::vector<const char*>& _filepaths) = 0;
		virtual void CreatePipeline(const VKPipelineCleanDesc* _desc) = 0;

		//Dependency injections from VKApp
		const VKLogger& logger;
		const VKDebugAllocator& deviceDebugAllocator;
		const VulkanDevice& device;
		const std::vector<VkDescriptorSetLayout>* descriptorSetLayouts;
		const std::vector<VkPushConstantRange>* pushConstantRanges;
		
		VkPipelineLayout layout;
		VkPipeline pipeline;
		std::vector<VkShaderModule> shaderModules;
	};
}



#endif