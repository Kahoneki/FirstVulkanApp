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
			 std::uint32_t _descriptorPoolSizeCount,
			 VkDescriptorPoolSize* _descriptorPoolSizes,
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
	  vulkanDescriptorPool(std::make_unique<VulkanDescriptorPool>(logger, deviceDebugAllocator, *vulkanDevice, _descriptorPoolSizeCount, _descriptorPoolSizes)),
	  bufferFactory(std::make_unique<BufferFactory>(logger, deviceDebugAllocator, *vulkanDevice)),
	  imageFactory(std::make_unique<ImageFactory>(logger, deviceDebugAllocator, *vulkanDevice, *vulkanCommandPool, *bufferFactory))
{
	vertexBuffer = VK_NULL_HANDLE;
	ubo = VK_NULL_HANDLE;
	image = VK_NULL_HANDLE;
	imageView = VK_NULL_HANDLE;
	descriptorSetLayout = VK_NULL_HANDLE;
	descriptorSet = VK_NULL_HANDLE;

	InitialiseVertexBuffer();
	InitialiseUBO();
	InitialiseSampler();
	InitialiseImage();
	
	CreateDescriptorSet();
	BindDescriptorSet();
	CreatePipeline();
}



VKApp::~VKApp()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION,"Shutting down VKApp\n");

	//Unmap buffers
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(ubo));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  UBO memory unmapped\n");
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(vertexBuffer));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  Vertex buffer memory unmapped\n");
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
		//Position		//UVs
		-0.5f,  0.5f,	0.0f, 0.0f,		//Top left
		 0.0f, -0.5f,	0.5f, 1.0f,		//Bottom centre
		 0.5f,  0.5f,	1.0f, 0.0f		//Top right
	};
	constexpr VkDeviceSize bufferSize{ 12 * sizeof(float) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Vertex Buffer\n");
	vertexBuffer = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Populating Vertex Buffer\n");

	//Map buffer memory
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Mapping vertex buffer memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
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
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  Vertex buffer memory filled with triangle data\n");
}



void VKApp::InitialiseUBO()
{
	//Define colour data
	constexpr float colour[] { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr VkDeviceSize bufferSize{ 4 * sizeof(float) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating UBO\n");
	ubo = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Populating UBO\n");

	//Map buffer memory
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Mapping UBO memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
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
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  UBO memory filled with colour\n");
}



void VKApp::InitialiseSampler()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Sampler\n");

	VkSamplerCreateInfo samplerInfo;
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = nullptr;
	samplerInfo.flags = 0;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	sampler = imageFactory->CreateSampler(samplerInfo);
}



void VKApp::InitialiseImage()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Image\n");

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Creating image\n");
	image = imageFactory->AllocateImage("Resource Files/garfield.png", VK_IMAGE_USAGE_SAMPLED_BIT);

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Creating image view\n");
	imageView = imageFactory->CreateImageView(image, VK_FORMAT_R8G8B8A8_SRGB);
}



void VKApp::CreateDescriptorSet()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Descriptor Set\n");

	//Define descriptor binding 0 as a uniform buffer accessible from the fragment shader
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Defining UBO binding at binding point 0 accessible to the fragment stage\n");
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	//Define descriptor binding 1 as an image sampler accessible from the fragment shader
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Defining sampler binding at binding point 1 accessible to the fragment stage\n");
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	
	//Create the descriptor set layout
	VkDescriptorSetLayoutBinding bindings[]{ uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Creating descriptor set layout", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &descriptorSetLayout ) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	descriptorSet = vulkanDescriptorPool->AllocateDescriptorSet(descriptorSetLayout);
}



void VKApp::BindDescriptorSet()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Binding Descriptor Set\n");

	//Bind descriptor set to make descriptor at binding 0 point to VKApp::buffer
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Creating descriptor set write: bind binding point 0 to UBO\n");
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = ubo;
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet descriptorWriteUBO;
	descriptorWriteUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteUBO.pNext = nullptr;
	descriptorWriteUBO.dstSet = descriptorSet;
	descriptorWriteUBO.dstBinding = 0;
	descriptorWriteUBO.dstArrayElement = 0;
	descriptorWriteUBO.descriptorCount = 1;
	descriptorWriteUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWriteUBO.pBufferInfo = &bufferInfo;
	descriptorWriteUBO.pTexelBufferView = nullptr;
	descriptorWriteUBO.pImageInfo = nullptr;

	//Bind descriptor set to make descriptor at binding 1 point to image view and sampler
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Creating descriptor set write: bind binding point 1 to image view and sampler\n");
	VkDescriptorImageInfo imageSamplerInfo{};
	imageSamplerInfo.imageView = imageView;
	imageSamplerInfo.sampler = sampler;
	imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet descriptorWriteImageSampler;
	descriptorWriteImageSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteImageSampler.pNext = nullptr;
	descriptorWriteImageSampler.dstSet = descriptorSet;
	descriptorWriteImageSampler.dstBinding = 1;
	descriptorWriteImageSampler.dstArrayElement = 0;
	descriptorWriteImageSampler.descriptorCount = 1;
	descriptorWriteImageSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWriteImageSampler.pBufferInfo = nullptr;
	descriptorWriteImageSampler.pTexelBufferView = nullptr;
	descriptorWriteImageSampler.pImageInfo = &imageSamplerInfo;

	VkWriteDescriptorSet descriptorSetWrites[]{ descriptorWriteUBO, descriptorWriteImageSampler };

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Updating descriptor set\n");
	vkUpdateDescriptorSets(vulkanDevice->GetDevice(), 2, descriptorSetWrites, 0, nullptr);
}



void VKApp::CreatePipeline()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Pipeline\n");
	
	VKGraphicsPipelineCleanDesc piplDesc{};
	piplDesc.renderPass = vulkanRenderManager->GetRenderPass();

	VkVertexInputBindingDescription vertInputBindingDesc{};
	vertInputBindingDesc.binding = 0;
	vertInputBindingDesc.stride = 4 * sizeof(float);
	vertInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	piplDesc.vertexBindingDescriptionCount = 1;
	piplDesc.pVertexBindingDescriptions = &vertInputBindingDesc;

	VkVertexInputAttributeDescription posAttribDesc{};
	posAttribDesc.binding = 0;
	posAttribDesc.location = 0;
	posAttribDesc.format = VK_FORMAT_R32G32_SFLOAT;
	posAttribDesc.offset = 0;

	VkVertexInputAttributeDescription uvAttribDesc{};
	uvAttribDesc.binding = 0;
	uvAttribDesc.location = 1;
	uvAttribDesc.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribDesc.offset = 2 * sizeof(float);

	VkVertexInputAttributeDescription attribDescs[]{ posAttribDesc, uvAttribDesc };
	
	piplDesc.vertexAttributeDescriptionCount = 2;
	piplDesc.pVertexAttributeDescriptions = attribDescs;
	
	vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(logger, deviceDebugAllocator, *vulkanDevice, &piplDesc, "shader.vert", "shader.frag", nullptr, nullptr, 1, &descriptorSetLayout);
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
	vkCmdBindDescriptorSets(vulkanRenderManager->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
	
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