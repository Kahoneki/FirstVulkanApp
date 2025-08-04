#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
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
	  vulkanDescriptorPool(std::make_unique<VulkanDescriptorPool>(logger, deviceDebugAllocator, *vulkanDevice, _descriptorPoolSizeCount, _descriptorPoolSizes)),
	  bufferFactory(std::make_unique<BufferFactory>(logger, deviceDebugAllocator, *vulkanDevice, *vulkanCommandPool)),
	  imageFactory(std::make_unique<ImageFactory>(logger, deviceDebugAllocator, *vulkanDevice, *vulkanCommandPool, *bufferFactory)),
	  vulkanRenderManager(std::make_unique<VulkanRenderManager>(logger, deviceDebugAllocator, *vulkanDevice, *vulkanCommandPool, *imageFactory, _windowSize, 3, _renderPassDesc))
{
	vertexBuffer = VK_NULL_HANDLE;
	indexBuffer = VK_NULL_HANDLE;
	ubo = VK_NULL_HANDLE;
	image = VK_NULL_HANDLE;
	imageView = VK_NULL_HANDLE;
	descriptorSetLayout = VK_NULL_HANDLE;
	descriptorSet = VK_NULL_HANDLE;

	InitialiseVertexBuffer();
	InitialiseIndexBuffer();
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
	//Define quad vertex buffer data
	const Vertex vertices[]{
		//Back face (-Z)
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},

		//Front face (+Z)
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},

		//Left face (-X)
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

		//Right face (+X)
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

		//Bottom face (-Y)
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

		//Top face (+Y)
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}}
	};
	constexpr VkDeviceSize bufferSize{ std::size(vertices) * sizeof(Vertex) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Vertex Buffer\n");
	vertexBuffer = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  Vertex buffer memory filled with quad vertex data\n");
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(vertexBuffer));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  Vertex buffer memory unmapped\n");
	
	//Transfer to device-local buffer
	vertexBuffer = bufferFactory->TransferToDeviceLocalBuffer(vertexBuffer, true);
}



void VKApp::InitialiseIndexBuffer()
{
	//Define quad index buffer data (clockwise)
	const std::uint16_t indices[] = {
		//Back face
		0, 1, 2, 0, 3, 1,
		//Front face
		4, 5, 6, 4, 6, 7,
		//Left face
		8, 9, 10, 8, 10, 11,
		//Right face
		12, 13, 14, 12, 15, 13,
		//Bottom face
		16, 17, 18, 16, 18, 19,
		//Top face
		20, 21, 22, 20, 23, 21
	};
	constexpr VkDeviceSize bufferSize{ std::size(indices) * sizeof(indices[0]) };
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Creating Index Buffer\n");
	indexBuffer = bufferFactory->AllocateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::APPLICATION, "Populating Index Buffer\n");

	//Map buffer memory
	void* indexBufferMap;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Mapping index buffer memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vulkanDevice->GetDevice(), indexBuffer, &memRequirements);
	VkResult result{ vkMapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(indexBuffer), 0, memRequirements.size, 0, &indexBufferMap) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::APPLICATION," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
	
	//Write to buffer
	memcpy(indexBufferMap, indices, static_cast<std::size_t>(bufferSize));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::APPLICATION, "  Index buffer memory filled with quad index data\n");

	//Unmap memory
	vkUnmapMemory(vulkanDevice->GetDevice(), bufferFactory->GetMemory(indexBuffer));
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::APPLICATION, "  Index buffer memory unmapped\n");

	//Transfer to device-local buffer
	indexBuffer = bufferFactory->TransferToDeviceLocalBuffer(indexBuffer, true);
}



void VKApp::InitialiseUBO()
{
	//Define cube data
	cubeData.model = glm::mat4(1.0f);
	cubeData.model = glm::rotate(cubeData.model, glm::radians(45.0f), glm::vec3(0, 1, 0));
	cubeData.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const float aspectRatio{ static_cast<float>(vulkanRenderManager->GetSwapchainExtent().width) / static_cast<float>(vulkanRenderManager->GetSwapchainExtent().height) };
	cubeData.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	constexpr VkDeviceSize bufferSize{ sizeof(UBOData) };
	
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
	memcpy(uboMap, &cubeData, static_cast<std::size_t>(bufferSize));
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
	imageView = imageFactory->CreateImageView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
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
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
	vertInputBindingDesc.stride = sizeof(Vertex);
	vertInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	piplDesc.vertexBindingDescriptionCount = 1;
	piplDesc.pVertexBindingDescriptions = &vertInputBindingDesc;

	VkVertexInputAttributeDescription posAttribDesc{};
	posAttribDesc.binding = 0;
	posAttribDesc.location = 0;
	posAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
	posAttribDesc.offset = offsetof(Vertex, pos);

	VkVertexInputAttributeDescription uvAttribDesc{};
	uvAttribDesc.binding = 0;
	uvAttribDesc.location = 1;
	uvAttribDesc.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribDesc.offset = offsetof(Vertex, texCoord);

	VkVertexInputAttributeDescription attribDescs[]{ posAttribDesc, uvAttribDesc };
	
	piplDesc.vertexAttributeDescriptionCount = 2;
	piplDesc.pVertexAttributeDescriptions = attribDescs;
	
	vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(logger, deviceDebugAllocator, *vulkanDevice, &piplDesc, "shader.vert", "shader.frag", nullptr, nullptr, 1, &descriptorSetLayout);
}



void VKApp::UpdateUBO()
{
	cubeData.model = glm::rotate(cubeData.model, glm::radians(0.01f), glm::vec3(1,0,0));
	memcpy(uboMap, &cubeData, sizeof(UBOData));
}



void VKApp::DrawFrame()
{

	//Define the clear colour
	VkClearValue clearValues[2];
	clearValues[0].color = {0.0f, 1.0f, 0.0f, 0.0f};
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	vulkanRenderManager->StartFrame(2, clearValues);

	UpdateUBO();
	
	vkCmdBindPipeline(vulkanRenderManager->GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline->GetPipeline());
	constexpr VkDeviceSize zeroOffset{ 0 };
	vkCmdBindVertexBuffers(vulkanRenderManager->GetCurrentCommandBuffer(), 0, 1, &vertexBuffer, &zeroOffset);
	vkCmdBindIndexBuffer(vulkanRenderManager->GetCurrentCommandBuffer(), indexBuffer, zeroOffset, VK_INDEX_TYPE_UINT16);
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
	vkCmdDrawIndexed(vulkanRenderManager->GetCurrentCommandBuffer(), 36, 1, 0, 0, 0);

	vulkanRenderManager->SubmitAndPresent();
}


}