#include <vulkan/vulkan.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstring>
#include <string>
#include <fstream>
#include <cmath>

#include "VKApp.h"

namespace Neki
{

VKApp::VKApp(VkExtent2D _windowSize,
			 VkRenderPassCreateInfo _renderPassDesc,
			 VkDescriptorPoolSize _descriptorPoolSize,
			 const std::uint32_t _apiVer,
			 const char* _appName,
			 const VKLoggerConfig& _loggerConfig,
			 const VK_ALLOCATOR_TYPE _allocatorType,
			 std::vector<const char*>* _desiredInstanceLayers,
			 std::vector<const char*>* _desiredInstanceExtensions,
			 std::vector<const char*>* _desiredDeviceLayers,
			 std::vector<const char*>* _desiredDeviceExtensions)
	: logger(_loggerConfig), instDebugAllocator(_allocatorType), deviceDebugAllocator(_allocatorType),
	  vulkanDevice(std::make_unique<VulkanDevice>(logger, instDebugAllocator, deviceDebugAllocator, _apiVer, _appName, _desiredInstanceLayers, _desiredInstanceExtensions, _desiredDeviceLayers, _desiredDeviceExtensions)),
	  vulkanCommandPool(std::make_unique<VulkanCommandPool>(logger, deviceDebugAllocator, *vulkanDevice, VK_COMMAND_POOL_TYPE::GRAPHICS)),
	  vulkanRenderManager(std::make_unique<VulkanRenderManager>(logger, deviceDebugAllocator, *vulkanDevice, *vulkanCommandPool, _windowSize, 3, _renderPassDesc)),
	  vulkanDescriptorPool(std::make_unique<VulkanDescriptorPool>(logger, deviceDebugAllocator, *vulkanDevice, _descriptorPoolSize)),
	  bufferFactory(std::make_unique<BufferFactory>(logger, deviceDebugAllocator, *vulkanDevice))
{
	vertexBuffer = VK_NULL_HANDLE;
	ubo = VK_NULL_HANDLE;

	InitialiseVertexBuffer();
	InitialiseUBO();
	CreateDescriptorSet();
	BindDescriptorSet();
	CreatePipeline();
}



VKApp::~VKApp()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION,"Shutting down VKApp\n");

	//Unmap buffers
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(ubo));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "\tUBO memory unmapped\n", VK_LOGGER_WIDTH::DEFAULT, false);
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(vertexBuffer));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "\tVertex buffer memory unmapped\n", VK_LOGGER_WIDTH::DEFAULT, false);
}



void VKApp::Run()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Starting Render\n");
	
	while (!vulkanRenderManager->WindowShouldClose())
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(vulkanDevice->GetDevice());
}



void VKApp::InitialiseVertexBuffer()
{
	//Define vertex buffer data
	constexpr float vertices[]
	{
		-0.5f, 0.5f,
		0.0f, -0.5f,
		0.5f, 0.5f
	};
	const VkDeviceSize bufferSize{ 6 * sizeof(float) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Vertex Buffer\n");
	vertexBuffer = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Populating Vertex Buffer\n");

	//Map buffer memory
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Mapping vertex buffer memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vulkanDevice->GetDevice(), vertexBuffer, &memRequirements);
	VkResult result{ vkMapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(vertexBuffer), 0, memRequirements.size, 0, &vertexBufferMap) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
	
	//Write to buffer
	memcpy(vertexBufferMap, vertices, static_cast<std::size_t>(bufferSize));
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Vertex buffer memory filled with triangle data\n");
}



void VKApp::InitialiseUBO()
{
	//Define colour data
	constexpr float colour[] { 1.0f, 1.0f, 1.0f, 1.0f };
	const VkDeviceSize bufferSize{ 4 * sizeof(float) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating UBO\n");
	ubo = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Populating UBO\n");

	//Map buffer memory
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Mapping UBO memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vulkanDevice->GetDevice(), ubo, &memRequirements);
	VkResult result{ vkMapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(ubo), 0, memRequirements.size, 0, &uboMap) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Write to buffer
	memcpy(uboMap, colour, static_cast<std::size_t>(bufferSize));
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "UBO memory filled with colour\n");
}



void VKApp::CreateDescriptorSet()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Descriptor Set\n");

	descriptorSetLayouts.resize(1);

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Defining UBO binding at binding point 0 accessible to the vertex stage\n");
	
	//Define descriptor binding 0 as a uniform buffer accessible from the vertex shader
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	
	//Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Creating descriptor set layout", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vulkanDevice->GetDevice(), ubo, &memRequirements);
	VkResult result{ vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &(descriptorSetLayouts[0]) ) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	descriptorSets.resize(1);
	descriptorSets = vulkanDescriptorPool->AllocateDescriptorSets(1, descriptorSetLayouts.data());
}



void VKApp::BindDescriptorSet()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Binding Descriptor Set\n");

	//Bind descriptor set to make descriptor at binding 0 point to VKApp::buffer
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = ubo;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptorWrite;
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.pNext = nullptr;
	descriptorWrite.dstSet = descriptorSets[0];
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.pBufferInfo = &bufferInfo;
	descriptorWrite.pTexelBufferView = nullptr;
	descriptorWrite.pImageInfo = nullptr;

	vkUpdateDescriptorSets(vulkanDevice->GetDevice(), 1, &descriptorWrite, 0, nullptr);

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "Descriptor set binding 0 updated to point to buffer\n");
}



void VKApp::CreatePipeline()
{
	VKGraphicsPipelineCleanDesc piplDesc{};
	piplDesc.renderPass = vulkanRenderManager->GetRenderPass();

	VkVertexInputBindingDescription vertInputBindingDesc{};
	vertInputBindingDesc.binding = 0;
	vertInputBindingDesc.stride = 2 * sizeof(float);
	vertInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	piplDesc.vertexBindingDescriptionCount = 1;
	piplDesc.pVertexBindingDescriptions = &vertInputBindingDesc;

	VkVertexInputAttributeDescription vertInputAttribDesc{};
	vertInputAttribDesc.location = 0;
	vertInputAttribDesc.binding = 0;
	vertInputAttribDesc.format = VK_FORMAT_R32G32_SFLOAT;
	vertInputAttribDesc.offset = 0;
	piplDesc.vertexAttributeDescriptionCount = 1;
	piplDesc.pVertexAttributeDescriptions = &vertInputAttribDesc;
	
	vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(logger, deviceDebugAllocator, *vulkanDevice, &piplDesc, "shader.vert", "shader.frag", nullptr, nullptr, &descriptorSetLayouts);
}



void VKApp::UpdateVertexBuffer()
{
	//Rotate the vertices
	float* vertexPositions{ static_cast<float*>(vertexBufferMap) };
	constexpr float speed{ 0.0005f }; //yes this is janky
	for (std::size_t i{ 0 }; i<6; i+=2)
	{
		float* x{ &(vertexPositions[i]) };
		float* y{ &(vertexPositions[i+1]) };
		float newX{ *x * std::cos(speed) - *y * std::sin(speed) };
		float newY{ *x * std::sin(speed) + *y * std::cos(speed) };
		*x = newX;
		*y = newY;
	}
}



void VKApp::UpdateUBO()
{
	//Make the colour pulse in a sine wave
	const double time{ glfwGetTime() };
	float* colour{ static_cast<float*>(uboMap) };
	constexpr float speed{ 1.0f }; //yes this is janky
	colour[0] = (std::sin(time * speed) + 1.0f) / 2.0f;
	colour[1] = (std::cos(time * speed) + 1.0f) / 2.0f;
	colour[2] = 0.5f;
}



void VKApp::DrawFrame()
{

	//Define the clear colour
	VkClearValue clearValue{};
	clearValue.color = {0.0f, 1.0f, 0.0f, 0.0f};
	
	vulkanRenderManager->StartFrame(clearValue);

	UpdateVertexBuffer();
	UpdateUBO();
	
	vkCmdBindPipeline(vulkanRenderManager->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline->GetPipeline());
	constexpr VkDeviceSize vertexBufferOffset{ 0 };
	vkCmdBindVertexBuffers(vulkanRenderManager->GetCurrentCommandBuffer(), 0, 1, &vertexBuffer, &vertexBufferOffset);
	vkCmdBindDescriptorSets(vulkanRenderManager->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline->GetPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
	
	//Define viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(vulkanRenderManager->GetSwapchainExtent().width);
	viewport.height = static_cast<float>(vulkanRenderManager->GetSwapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vulkanRenderManager->GetCurrentCommandBuffer(), 0, 1, &viewport);

	//Define scissor
	VkRect2D scissor{};
	scissor.offset = {0,0};
	scissor.extent = vulkanRenderManager->GetSwapchainExtent();
	vkCmdSetScissor(vulkanRenderManager->GetCurrentCommandBuffer(), 0, 1, &scissor);
	
	//Draw the damn triangle
	vkCmdDraw(vulkanRenderManager->GetCurrentCommandBuffer(), 3, 1, 0, 0);

	vulkanRenderManager->SubmitAndPresent();
}


}