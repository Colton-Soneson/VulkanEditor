#include "DemoApplication.h"
#include "DebugTools.h"

//stb for image/ texture (reinclude this when doing shit)
//		STB_IMAGE_IMPLEMENTATION MUST BE DEFINED IN HERE, its preprocessor directive states it cant
//		be defined within a .h file
#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
#include <stb/stb_image.h>

//not sure if tinyobjloader is the same way that stb is but do this all the same
#define TINYOBJLOADER_IMPLEMENTATION
//#include <tiny_obj_loader.h>
#include <tol/tiny_obj_loader.h>

VkCommandBuffer DemoApplication::beginSingleTimeCommands()
{
	//temp command buffer (can be optimized at some point with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
	// for this temp crap)
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mpDevSel->selectedDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	//tells the driver we are using CB once and wait with returning 
																	//	until we are done copying

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}


bool DemoApplication::checkValidationLayerSupport()
{
	//find total number of layers and pass it into layerCount
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	//go through em
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//check all layers in validationLayers exist in availableLayers
	for (const char* layerName : validationLayers)
	{
		bool layerFound = false; //basic flag

		for (const auto& layerProperties : availableLayers)
		{

			//name comparison check
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		//we couldnt find a validationLayer inside the available layers we have
		if (layerFound == false)
		{
			return false;
		}
	}

	//all VLs are in ALs
	return true;
}


VkExtent2D DemoApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	//query current size of framebuffer (to make sure swap chain images are correct size)
	if (capabilities.currentExtent.width != UINT64_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(mpWindow, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		
		//get the extents of the surface bounds and sets them in actualExtent 
		//max and min clamp width and height between the supported implementation
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}


VkPresentModeKHR DemoApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	//Four modes:
	//		VK_PRESENT_MODE_IMMEDIATE_KHR
	//		VK_PRESENT_MODE_FIFO_KHR
	//		VK_PRESENT_MODE_FIFO_RELAXED_KHR
	//		VK_PRESENT_MODE_MAILBOX_KHR

	for (const auto& availablePresentMode : availablePresentModes)
	{
		//This allows for triple buffering, so its preffered
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	//this is the only garaunteed one
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkSurfaceFormatKHR DemoApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//color space, using VK_FORMAT_B8G8R8A8_UNORM as a color base instead of SRGB because its easy
	//	that means 8 bits per each B R G (different order RGB) and A channels for a 32 bit color pixel
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	//the [0] is for if we want to have a rating system like for GPU selection
	return availableFormats[0];
}


void DemoApplication::cleanup()
{
	cleanupSwapChain();

	for (auto& object : objects)
	{
		//sampler destruction
		vkDestroySampler(mpDevSel->selectedDevice(), object.msTextureSampler, nullptr);

		//destroy texture image views out
		vkDestroyImageView(mpDevSel->selectedDevice(), object.msTextureImageView, nullptr);

		//this is for textures
		vkDestroyImage(mpDevSel->selectedDevice(), object.msTextureImage, nullptr);
		vkFreeMemory(mpDevSel->selectedDevice(), object.msTextureImageMemory, nullptr);
	}

		//descriptor destruction
		vkDestroyDescriptorSetLayout(mpDevSel->selectedDevice(), mDescriptorSetLayout, nullptr);

		//index buffer deletion (works same as vertex stuff really)s
		vkDestroyBuffer(mpDevSel->selectedDevice(), models.msIndexBuffer, nullptr);
		vkFreeMemory(mpDevSel->selectedDevice(), models.msIndexBufferMemory, nullptr);

		//we dont need the vertex buffer in memory anymore
		vkDestroyBuffer(mpDevSel->selectedDevice(), models.msVertexBuffer, nullptr);

		//free out VBM
		vkFreeMemory(mpDevSel->selectedDevice(), models.msVertexBufferMemory, nullptr);
	//semaphore destruction (multiple per pipeline)
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(mpDevSel->selectedDevice(), mRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(mpDevSel->selectedDevice(), mImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(mpDevSel->selectedDevice(), mInFlightFences[i], nullptr);
	}

	
	//semaphore destruction (if only one per pipeline)
	//vkDestroySemaphore(mpDevSel->selectedDevice(), mRenderFinishedSemaphore, nullptr);
	//vkDestroySemaphore(mpDevSel->selectedDevice(), mImageAvailableSemaphore, nullptr);

	//cammand pool final destruction
	vkDestroyCommandPool(mpDevSel->selectedDevice(), mCommandPool, nullptr);

	vkDestroyDevice(mpDevSel->selectedDevice(), nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}

	//destroy the surface abstraction
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

	//clean up the instance right before program exit
	vkDestroyInstance(mInstance, nullptr);

	//remove the window
	glfwDestroyWindow(mpWindow);

	//cleanup call for glfw
	glfwTerminate();

	/*
	//	THIS WAS FOR WITHOUT SWAPCHAIN RECREATION

	//framebuffer list destruction
	for (auto framebuffer : mSwapChainFrameBuffers)
	{
		vkDestroyFramebuffer(mpDevSel->selectedDevice(), framebuffer, nullptr);
	}

	//destroy the graphics pipeline
	vkDestroyPipeline(mpDevSel->selectedDevice(), mGraphicsPipeline, nullptr);
	
	//pipeline destruction
	vkDestroyPipelineLayout(mpDevSel->selectedDevice(), mPipelineLayout, nullptr);

	//renderpass destruction
	vkDestroyRenderPass(mpDevSel->selectedDevice(), mRenderPass, nullptr);

	//we need to destroy images via loop through
	for (auto imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mpDevSel->selectedDevice(), imageView, nullptr);
	}

	//destroy swapchain
	vkDestroySwapchainKHR(mpDevSel->selectedDevice(), mSwapChain, nullptr);

	//device destruction
	vkDestroyDevice(mpDevSel->selectedDevice(), nullptr);

	//debug util destruction
	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}

	//destroy the surface abstraction
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

	//clean up the instance right before program exit
	vkDestroyInstance(mInstance, nullptr);

	//remove the window
	glfwDestroyWindow(mpWindow);

	//cleanup call for glfw
	glfwTerminate();
	*/
}


void DemoApplication::cleanupSwapChain()
{
	//color stuff with multisampling
	vkDestroyImageView(mpDevSel->selectedDevice(), mColorImageView, nullptr);
	vkDestroyImage(mpDevSel->selectedDevice(), mColorImage, nullptr);
	vkFreeMemory(mpDevSel->selectedDevice(), mColorImageMemory, nullptr);

	//depth buffer and image destruction
	vkDestroyImageView(mpDevSel->selectedDevice(), mDepthImageView, nullptr);
	vkDestroyImage(mpDevSel->selectedDevice(), mDepthImage, nullptr);
	vkFreeMemory(mpDevSel->selectedDevice(), mDepthImageMemory, nullptr);

	for (size_t i = 0; i < mSwapChainFrameBuffers.size(); ++i)
	{
		vkDestroyFramebuffer(mpDevSel->selectedDevice(), mSwapChainFrameBuffers[i], nullptr);
	}

	//destroy or free all things that depend on swap Chain
	vkFreeCommandBuffers(mpDevSel->selectedDevice(), mCommandPool, static_cast<uint32_t>(mCommandBuffers.size()), mCommandBuffers.data());
	vkDestroyPipeline(mpDevSel->selectedDevice(), mGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mpDevSel->selectedDevice(), mPipelineLayout, nullptr);
	vkDestroyRenderPass(mpDevSel->selectedDevice(), mRenderPass, nullptr);

	//go through image views and delete them
	for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
	{
		vkDestroyImageView(mpDevSel->selectedDevice(), mSwapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(mpDevSel->selectedDevice(), mSwapChain, nullptr);
	
	for (auto& object : objects)
	{
		for (size_t i = 0; i < mSwapChainImages.size(); ++i)
		{
			vkDestroyBuffer(mpDevSel->selectedDevice(), object.msUniformBuffers[i], nullptr);
			vkFreeMemory(mpDevSel->selectedDevice(), object.msUniformBuffersMemory[i], nullptr);
		}
	}

	vkDestroyDescriptorPool(mpDevSel->selectedDevice(), mDescriptorPool, nullptr);
}


void DemoApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	//this function can just be called inside buffer creations

	//since this is a generalized helper function, based upon our parameters we set up this info
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//create buffer based on the info
	if (vkCreateBuffer(mpDevSel->selectedDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer");
	}

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(mpDevSel->selectedDevice(), buffer, &memReqs);	//get staging requirements (memory shit)

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties);	

	if (vkAllocateMemory(mpDevSel->selectedDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(mpDevSel->selectedDevice(), buffer, bufferMemory, 0);	//now we do the binding after memory allocation
}


void DemoApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};	//not possible to use VK_WHOLE_SIZE
	//copyRegion.srcOffset = 0;	
	//copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion); //the transfer

	endSingleTimeCommands(commandBuffer);
}


void DemoApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	//specify which part of the buffer is going to be copied
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;		//byte offset at which pixel value starts
	region.bufferRowLength = 0;		//how laid in memory (padding and whatnot)
	region.bufferImageHeight = 0;	//how laid in memory

	// subresource, offset, and extent below signify where we want to copy pixels

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };	//last one is depth

	//fourth param here indicates which layout the image is currently using 
	//	we just assume here that we already transitioned to the layout optimal for copying pixels
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}


void DemoApplication::createColorResources()
{
	VkFormat colorFormat = mSwapChainImageFormat;

	createImage(mSwapChainExtent.width, mSwapChainExtent.height, 1, mMSAASamples, colorFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mColorImage, mColorImageMemory);

	mColorImageView = createImageView(mColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}


void DemoApplication::createCommandBuffers()
{
	//has to be the same number as num of swap chain images
	mCommandBuffers.resize(mSwapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

	if (vkAllocateCommandBuffers(mpDevSel->selectedDevice(), &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers");
	}

	//begin recording commands buffers

		for (size_t i = 0; i < mCommandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;	//only relavant for secondary CBs

			if (vkBeginCommandBuffer(mCommandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffer");
			}

			//start the render pass
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = mRenderPass;
			renderPassInfo.framebuffer = mSwapChainFrameBuffers[i];

			//define render area (everything not in this range will be undefined)
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = mSwapChainExtent;

			//the VK_ATTACHMENT_LOAD_OP_CLEAR requires use of clearVals
			std::array<VkClearValue, 2> clearValues = {};
			clearValues[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };	//1.0 is far view plane 0.0 is at near view plane

			//these two define use for VK_ATTACHMENT_LOAD_OP_CLEAR (load op for color attachment)
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			//take into account renderpass and stuff it in a CB (choice of secondary or primary)
			vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			//bind to graphics pipeline
			vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

			//custom input from vertex buffers into vertex count
			VkBuffer vertexBuffers[] = { models.msVertexBuffer };
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(mCommandBuffers[i], models.msIndexBuffer, 0, VK_INDEX_TYPE_UINT32);	//FOR SMALL APPS UINT16 is used and defined by mIndices

			for (auto object : objects)
			{
				//bind right descriptor for each swap chain image
				vkCmdBindDescriptorSets(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &(object.msDescriptorSets[i]), 0, nullptr);
				//specify bind descriptor to gpu or cpu, layout descriptors are based on, 
				//index of the first descriptor set, number of sets to bind, array of sets to bind, (last two) specify array of offsets


				//SUPER TRIANGLE SPECIFIC (fixed in vertex buffer)
				//Parameters:
				//	commandBuffer passed into, vertexCount, instanceCount, firstVertex, firstInstance
				vkCmdDrawIndexed(mCommandBuffers[i], static_cast<uint32_t>(models.msIndices.size()), 1, 0, 0, 0);
			}
			//end the render pass first
			vkCmdEndRenderPass(mCommandBuffers[i]);

			if (vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to record command buffer");
			}
		}

	
}


void DemoApplication::createCommandPool()
{
	//we need a queuefam
	//QueueFamilyIndices queueFamilyIndices = mQuery.findQueueFamilies(mpDevSel->selectedPhysicalDevice(), mSurface);
	QueueFamilyIndices queueFamilyIndices = mpDevSel->fQueFam();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;	//allows CBs rerecorded individually
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;

	if (vkCreateCommandPool(mpDevSel->selectedDevice(), &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}

}


void DemoApplication::createDepthResource()
{
	VkFormat depthFormat = findDepthFormat();

	//use standard helper functions for create image and image view but use the new formatting for
	//	 depth into account
	createImage(mSwapChainExtent.width, mSwapChainExtent.height, 1, mMSAASamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);

	mDepthImageView = createImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	//explicit transition of depth image (but this is just handled in render pass 
	//	dont have to do this process)
	//transitionImageLayout(mDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void DemoApplication::createDepthSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	//linear works for shadowmaps
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	//texture space coords
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	//anistrophic filtering
	samplerInfo.maxAnisotropy = 1.0f;		

	//which color is returned when sampling beyond the image WITH CLAMP TO BORDER ADDRESSING MODE
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;		//true means you use coords of 0 to textureWidth/Height
														//false means its 0 to 1

	samplerInfo.compareEnable = VK_FALSE;			//true means texels will first be compared to value 
													//	then the result will be used in filter operation
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.mipLodBias = 0.0f;

	if (vkCreateSampler(mpDevSel->selectedDevice(), &samplerInfo, nullptr, &mDepthImageSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shadowmap sampler");
	}
	
}

void DemoApplication::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(mSwapChainImages.size() * objects.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(mSwapChainImages.size() * objects.size());

	//allocate one descriptor every frame
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(mSwapChainImages.size() * objects.size());
	
	if (vkCreateDescriptorPool(mpDevSel->selectedDevice(), &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool");
	}
}


void DemoApplication::createDescriptorSetLayout()
{
	//provide details about every descriptor binding used in shaders

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;	
	uboLayoutBinding.descriptorCount = 1;	//you can have an array		!!!!!CAN BE USED TO SPECIFY TRANSFORMATIONS FOR EACH OF THE BBONES IN A SKELETON FOR SKELETAL ANIMATION
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;	//for image sampling realted to descriptors
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	//can combine with VkShaderStageFlagBits

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	
	if (vkCreateDescriptorSetLayout(mpDevSel->selectedDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create desccriptor set layout");
	}

}


void DemoApplication::createDescriptorSets()
{
	for (auto& object : objects)
	{
		std::vector<VkDescriptorSetLayout> layouts(mSwapChainImages.size(), mDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		object.msDescriptorSets.resize(mSwapChainImages.size());
		
		if (vkAllocateDescriptorSets(mpDevSel->selectedDevice(), &allocInfo, object.msDescriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets");
		}

		for (size_t i = 0; i < mSwapChainImages.size(); i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = object.msUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = object.msTextureImageView;
			imageInfo.sampler = object.msTextureSampler;

	/*		VkDescriptorImageInfo depthImageInfo = {};
			depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			depthImageInfo.imageView = mDepthImageView;
			depthImageInfo.sampler = mDepthImageSampler;*/

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = object.msDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;			//UB binding was index 0
			descriptorWrites[0].dstArrayElement = 0;	//first index of the array we want to update, we arent using an array though
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;	//how many array elements you want to update
			descriptorWrites[0].pImageInfo = nullptr;		//refer to image data
			descriptorWrites[0].pTexelBufferView = nullptr;	//refer to buffer views
			descriptorWrites[0].pBufferInfo = &bufferInfo;	//for referal to buffer data

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = object.msDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;			//UB binding was index 0
			descriptorWrites[1].dstArrayElement = 0;	//first index of the array we want to update, we arent using an array though
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;	//how many array elements you want to update
			descriptorWrites[1].pImageInfo = &imageInfo;		//refer to image data

			
			//descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			//descriptorWrites[2].dstSet = object.msDescriptorSets[i];
			//descriptorWrites[2].dstBinding = 2;			//UB binding was index 0
			//descriptorWrites[2].dstArrayElement = 0;	//first index of the array we want to update, we arent using an array though
			//descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			//descriptorWrites[2].descriptorCount = 1;	//how many array elements you want to update
			//descriptorWrites[2].pImageInfo = &depthImageInfo;		//refer to image data

			vkUpdateDescriptorSets(mpDevSel->selectedDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}


void DemoApplication::createFramebuffers()
{
	//we need the number of framebuffers to equal the number of swapchain image views they
	//	will be referencing
	mSwapChainFrameBuffers.resize(mSwapChainImageViews.size());

	//iterate and create from copy
	for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
	{
					//depth image can be the same for all IVs because only a single subpass runs at a time
		std::array<VkImageView, 3> attachments = { mColorImageView, mDepthImageView, mSwapChainImageViews[i] };

		//that sort of "copy" over
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;

		//create the framebuffer and store it at position in vector
		if (vkCreateFramebuffer(mpDevSel->selectedDevice(), &framebufferInfo, nullptr, &mSwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer");
		}
	}

}


void DemoApplication::createGraphicsPipeline()
{
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	auto vertShaderCode = readFile(objects[0].msVertShaderPath);
	auto fragShaderCode = readFile(objects[0].msFragShaderPath);

	//create the modules (same way for both vetex and frag)
	vertShaderModule = createShaderModule(vertShaderCode);
	fragShaderModule = createShaderModule(fragShaderCode);
	

	//vertex shader graphics pipeline object fill
	//	sType here describes which pipeline stage we are on
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	//allows for combination of multiple fragment shaders into one shader module and 
	// use different entry points for their behaviours, but instead just use the "main"
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	//frag shader
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	//array to contain them
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	//formatting the vertex data
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	//fill this shit later
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	//How we draw vertices from data
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;   //true will break up lines and triangles in the _STRIP topolgy modes using index of 0xFFFF or 0xFFFFFFF

	//viewport (region of framebuffer output renders to)
	VkViewport viewport = {};

	//top left corner
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	//bottom right corner (use what the swapchain defines these as)
	viewport.width = (float)mSwapChainExtent.width;
	viewport.height = (float)mSwapChainExtent.height;

	//depth shit (has to be within 0 an 1, but min may be higher then max
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//ignore the scissor filter by just drawing the full framebuffer
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = mSwapChainExtent;

	//combining the viewport and scissor
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;	//possiblility depending on card to have more than one viewport
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//depth testing, face culling, wireframe rendering
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;				//true, frags past near or far planes are clamped instead of discarding. Requires GPU feature
	rasterizer.rasterizerDiscardEnable = VK_FALSE;		//true, geometry never passes through rasterizer phase (nothing gets to framebuffer)
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;		//(FILL, LINE, or POINT) for entire poly filled, draw wireframe, or draw just points
	rasterizer.lineWidth = 1.0f;						//thickness defined by number of fragments
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;				//MVP stuff
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;		//MVP stuff (since we did Y-flip, then drawy in counter-clockwise order)
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;			//opt
	rasterizer.depthBiasClamp = 0.0f;					//opt
	rasterizer.depthBiasSlopeFactor = 0.0f;				//opt


	//Multisampling (anti-aliasing is sharper edges and without running frag shader a lot)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;	//enable sample shading in pipeline
	multisampling.rasterizationSamples = mMSAASamples;
	multisampling.minSampleShading = 0.5f;			//min fraction for sample shading (closer to one is smoother)
	multisampling.pSampleMask = nullptr;			//opt
	multisampling.alphaToCoverageEnable = VK_FALSE; //opt
	multisampling.alphaToOneEnable = VK_FALSE;		//opt

	//use the depth testing attachment
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;		//should depth of new frags be compared to depth buffer
	depthStencil.depthWriteEnable = VK_TRUE;	//new depth of frags should be allowed to go through (good for transparency)
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;	//comparison performed to keep or discard frags (LESS is lowerDepth = closer)
	depthStencil.depthBoundsTestEnable = VK_FALSE;		//this is for restricting fragments within specific depth range (backface culling???)
	depthStencil.minDepthBounds = 0.0f;			
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;	//allows for operations for the buffer
	depthStencil.front = {};
	depthStencil.back = {}; 

	//color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		//opt
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	//opt
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				//opt
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		//opt
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	//opt
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				//opt


	//color blending for all of the framebuffers
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//uniform values for pipeline
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;	

	//push constants can pass dynamic values to shaders
	pipelineLayoutInfo.pushConstantRangeCount = 0;			//opt	CHECK THIS 
	pipelineLayoutInfo.pPushConstantRanges = nullptr;		//opt	CHECK THIS

	if (vkCreatePipelineLayout(mpDevSel->selectedDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}

	//now we have all the components to define a graphics pipeline we can finally start creating it
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;			//vertShaderStageInfo and fragShaderStageInfo
	pipelineInfo.pStages = shaderStages;

	//go further into connecting shaderstages with pipeline
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	//fixed function stage input to pipeline
	pipelineInfo.layout = mPipelineLayout;

	//renderPass and subpasses into pipeline
		//this may be useful for Engines, its a reference to render pass and index of subpass, not
		//	used for this tutorial though
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;

	//pipeline derrivatives: able to derive new pipeline from existing one
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;		//specified handle
	//pipelineInfo.basePipelineIndex = -1;					//reference to another pipeline


	//final graphics pipeline
	if (vkCreateGraphicsPipelines(mpDevSel->selectedDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline");
	}

	//MAKE SURE THIS IS LAST
	
	
	vkDestroyShaderModule(mpDevSel->selectedDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(mpDevSel->selectedDevice(), vertShaderModule, nullptr);
	
}


void DemoApplication::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
											VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;		//what kind of coordinate system the texels (texture pixel) in the image
												//	are going to be addressed
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;					//texels in extent need a depth of atleast 1 not zero
	imageInfo.mipLevels = mipLevels;					//tex not an array? use 1
	imageInfo.arrayLayers = 1;					//tex not an array? use 1
	imageInfo.format = format;					//specified for copy operation of texels as pixels
	imageInfo.tiling = tiling;					//LINEAR or OPTIMAL
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;					//we use it as a destination to transfer (DST) from copy 
	imageInfo.samples = numSamples;				//default is set to 1_bit if not adjusted, its multisampling
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	//just by queue family
	//imageInfo.flags = 0;						//optional: sparse images, where only certain regions are actually backed in memory

	if (vkCreateImage(mpDevSel->selectedDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(mpDevSel->selectedDevice(), image, &memReqs);

	//works the same way as allocating memory for a buffer
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties);
	
	if (vkAllocateMemory(mpDevSel->selectedDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory");
	}

	vkBindImageMemory(mpDevSel->selectedDevice(), image, imageMemory, 0);
}


VkImageView DemoApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	//similar to create image views
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(mpDevSel->selectedDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view");
	}

	return imageView;
}


void DemoApplication::createImageViews()
{
	//be able to fit all the image views to be creating (same size as available images)
	mSwapChainImageViews.resize(mSwapChainImages.size());

	for (size_t i = 0; i < mSwapChainImages.size(); ++i)
	{
		mSwapChainImageViews[i] = createImageView(mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		/*
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = mSwapChainImages[i];

		//specification on how data should be interpritted
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;	//you can treat an image as 1D, 2D, 3D, or cube maps
		imageViewCreateInfo.format = mSwapChainImageFormat;

		//default mapping setting for these (you can do 0 to 1)
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//subresource range describes image purpose and what should be accessed
		//		stereographic 3D requires multiple layers
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;


		//actually create the image view and store it in the vector
		if (vkCreateImageView(mpDevSel->selectedDevice(), &imageViewCreateInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views");
		}
		*/
	}
}


void DemoApplication::createIndexBuffer()
{

	VkDeviceSize bufferSize = sizeof(models.msIndices[0]) * models.msIndices.size();

	//actual staging buffer that only host visible buffer as temp buffer (then we later use device local as actual)
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);


	void* data;

	//mapping the buffer memory into CPU accessible memory
	vkMapMemory(mpDevSel->selectedDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);	//doesnt immdeiately copy into buffer mem
	memcpy(data, models.msIndices.data(), (size_t)bufferSize);	//We can do this from the above HOST_COHERENT_BIT
	vkUnmapMemory(mpDevSel->selectedDevice(), stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, models.msIndexBuffer, models.msIndexBufferMemory);
	/*
		SRC_BIT: Buffer can be used as source in a mem transfer op
		DST_BIT: Buffer can be used as destination in mem transfer op
	*/

	copyBuffer(stagingBuffer, models.msIndexBuffer, bufferSize);	//move vertex data to device local buffer
	vkDestroyBuffer(mpDevSel->selectedDevice(), stagingBuffer, nullptr);		//free out our staging buffer
	vkFreeMemory(mpDevSel->selectedDevice(), stagingBufferMemory, nullptr);
}


void DemoApplication::createInstance() {

	//do validation layers stuff
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation Layers requested, but not available");
	}

	//set the app info (pretty self explanitory)
	mAppInfo = {};
	mAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;	//this is what pNext can point to for extensions (default val here leaves as nullptr)
	mAppInfo.pApplicationName = "Triangle";
	mAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	mAppInfo.pEngineName = "No Engine";
	mAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	mAppInfo.apiVersion = VK_API_VERSION_1_0;		//maybe need to set this to _1_1

	//not optional driver extensions
	mCreateInfo = {};
	mCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	mCreateInfo.pApplicationInfo = &mAppInfo;

	//include the validation layer names (if enabled)
	/*
	//this got replaced
	if (enableValidationLayers)
	{
		mCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		mCreateInfo.ppEnabledLayerNames = validationLayers.data();	//sets names
	}
	else
	{
		mCreateInfo.enabledLayerCount = 0;	//no layer names
	}
	*/

	//debug messaging
	auto extensionsDebug = getRequiredExtensions();
	mCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsDebug.size());	//set extensions count in info
	mCreateInfo.ppEnabledExtensionNames = extensionsDebug.data();	//put actual data into create info

	//final process
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	
	//if debugging is on
	if (enableValidationLayers)
	{
		//enables global validator layers
		mCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		mCreateInfo.ppEnabledLayerNames = validationLayers.data();

		//fill debug
		populateDebugMessengerCreateInfo(debugCreateInfo);
		mCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		//otherwise we have no layers because nother is there
		mCreateInfo.enabledLayerCount = 0;
		mCreateInfo.pNext = nullptr;
	}

	//finalized
	//VkResult result = vkCreateInstance(&mCreateInfo, nullptr, &mInstance);	//everything can pass to VkResult, it can return the VK_SUCCESS code or errors

	//alternative not storing result
	if (vkCreateInstance(&mCreateInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed To create instance");
	}
	else
	{
		std::cout << "instance created";
	}

	//go through extensions props manually inside (OVERWRITTEN)
	/*
	//go through extension array
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//allocate array to hold extension details
	std::vector<VkExtensionProperties> extensionsProps(extensionCount);
	mpExtensionsProps = new std::vector<VkExtensionProperties>(extensionCount); //this is me trying to make it a member 

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsProps.data());

	//go through the VkExtensionProperties (just for proof)
	std::cout << "\n\navailable extensions: \n";
	for (const auto& extension : extensionsProps)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	//you can also go through glfwGetRequiredInstanceExtensions
	*/
}



void DemoApplication::createRenderPass()
{
	//single color buffer attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = mSwapChainImageFormat;		//same format as from swapchain
	colorAttachment.samples = mMSAASamples;	//without multisampling its 1 sample
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		//means we dont care about previous layout
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//we want it to be ready for presentation after rendering

	//for depth testing the image
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();		//same as the depth image
	depthAttachment.samples = mMSAASamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//it doesnt matter what previous data was
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//multisampled  images cannot be presented directlly
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = mSwapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	//subpass p1: referrencing attachments in framebuffer and optimizing them
	
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;		//which attachment to reference in atachment descriptiion array
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//which layout we would like to have during subpass
																				//this means we get color performance
	
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//points to color buffer which serves as resolve target
	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//subpass p2
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//GPU subpasses instead of CPU
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;	//single depth (+stencil) attachment (we just depth test on one buffer not multiple)
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
															//pInputAttachments, pResolveAttachments, pDepthStencilAttachment, and pPreserveAttachments all also possible

		//subpass dependencies for subpass use
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	//implicit subpass defined before or render render pass decided by index below
	dependency.dstSubpass = 0;						//refers to our subpass, first and only one in this case

	//ops to wait on stages in which they occur
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//ops should involve reading and writing of color attachment in this stage
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	//renderpass
	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();	
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;


	if (vkCreateRenderPass(mpDevSel->selectedDevice(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass");
	}
}


void DemoApplication::createSyncObjects()
{
	//multiple semaphores, to use the graphics pipeline for more than a frame at a time
	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	//for cpu gpu
	mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	//to prevent "in_flight" image rendering
	mImagesInFlight.resize(mSwapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	//we create the signal state
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(mpDevSel->selectedDevice(), &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mpDevSel->selectedDevice(), &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(mpDevSel->selectedDevice(), &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores/ fences for a frame");
		}
	}

	/*
	//for a single one of each (not optimal)
	if (vkCreateSemaphore(mpDevSel->selectedDevice(), &semaphoreInfo, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(mpDevSel->selectedDevice(), &semaphoreInfo, nullptr, &mRenderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores");
	}
	*/
}


VkShaderModule DemoApplication::createShaderModule(const std::vector<char>& code)
{
	//based upon the SPIR-V code imported from file
	//	we just want to delete it in here because its just easier, so no member data
	VkShaderModuleCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.codeSize = code.size();
	shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	//we are gonna return from this func so dont make member function
	VkShaderModule shaderModule;

	if (vkCreateShaderModule(mpDevSel->selectedDevice(), &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	return shaderModule;
}


void DemoApplication::createSurface()
{
	if (glfwCreateWindowSurface(mInstance, mpWindow, nullptr, &mSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface");
	}
}


void DemoApplication::createSwapChain()
{
	//the properties are finally used
	//SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mpDevSel->selectedPhysicalDevice(), mSurface);
	SwapChainSupportDetails swapChainSupport = mpDevSel->swapSupDet();
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//there is a minimum image count we need to fullfill in a swapchain
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;	//we do +1 so we can have atleast one more than the minimum 

	//
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//structure filling like usual
	mSwapChainCreateInfo = {};
	mSwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	mSwapChainCreateInfo.surface = mSurface;

	//this has to come after surface def (more details in the defining functions)
	mSwapChainCreateInfo.minImageCount = imageCount;
	mSwapChainCreateInfo.imageFormat = surfaceFormat.format;
	mSwapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	mSwapChainCreateInfo.imageExtent = extent;
	mSwapChainCreateInfo.imageArrayLayers = 1;								//ALWAYS ONE unless developing a stereoscopic 3D application
	mSwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//specifies swapchain operation type to transfer the rendered image

	//get all the indices for graphics fam and presentation queue
	//QueueFamilyIndices indices = mQuery.findQueueFamilies(mpDevSel->selectedPhysicalDevice(), mSurface);
	QueueFamilyIndices indices = mpDevSel->fQueFam();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//draw fon images in the swap chain from graphics queue then submit to presentation queue
	if (indices.graphicsFamily != indices.presentFamily)
	{
		mSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	//no explicit ownership transfer
		mSwapChainCreateInfo.queueFamilyIndexCount = 2;
		mSwapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		mSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	//image owned by one queue fam and needs explicit transfer to another fam
		mSwapChainCreateInfo.queueFamilyIndexCount = 0;
		mSwapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	//defining you want the transform to be, this is just the standard call
	mSwapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	//alpha channel used to blend with other windows in window system, this call ignores that option
	mSwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;


	mSwapChainCreateInfo.presentMode = presentMode;
	mSwapChainCreateInfo.clipped = VK_TRUE;				//performance option, enable unless pixel reading and prediction required

	//given time the swapchain may become unoptimized during runtime
	mSwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	//the actual creation
	if (vkCreateSwapchainKHR(mpDevSel->selectedDevice(), &mSwapChainCreateInfo, nullptr, &mSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swapchain");
	}

	//fill the vector of images from the swapchain
	vkGetSwapchainImagesKHR(mpDevSel->selectedDevice(), mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mpDevSel->selectedDevice(), mSwapChain, &imageCount, mSwapChainImages.data());

	//later change these
	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;

}


void DemoApplication::createTextureImage()
{
	for (auto& object : objects)
	{
		int textureWidth, textureHeight, textureChannels;

		//take file path and number of channels to load as args
		stbi_uc* pixels = stbi_load(object.msTexturePath.c_str(),
			&textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		//we use 4 because there are 4 bytes per pixel
		VkDeviceSize imageSize = textureWidth * textureHeight * 4;

		//mip level division (explained in notes) but obvious
		object.msMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

		if (!pixels) { throw std::runtime_error("failed to load texture image"); };

		//create buffer in host visible memory to use vkMapMem to copy on the pixels
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		//copy those pixel values to the buffer
		void* data;
		vkMapMemory(mpDevSel->selectedDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(mpDevSel->selectedDevice(), stagingBufferMemory);

		//free up the pixel array
		stbi_image_free(pixels);

		//mipmapping VkCmdBlit transfer op needs SRC and DST here
		createImage(textureWidth, textureHeight, object.msMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			object.msTextureImage, object.msTextureImageMemory);

		//we need to transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		//		then execute the buffer to image copy operation
		transitionImageLayout(object.msTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, object.msMipLevels);
		copyBufferToImage(stagingBuffer, object.msTextureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

		//VK_IMAGE_LAYOUT_UNDEFINED image will be considered the "old layout" here
		/*
		transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
									VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mMipLevels);
		*/
		//clear up memory
		vkDestroyBuffer(mpDevSel->selectedDevice(), stagingBuffer, nullptr);
		vkFreeMemory(mpDevSel->selectedDevice(), stagingBufferMemory, nullptr);

		generateMipmaps(object.msTextureImage, VK_FORMAT_R8G8B8A8_UNORM, textureWidth, textureHeight, object.msMipLevels);
	}
}


void DemoApplication::createTextureImageView()
{
	for (auto& object : objects)
	{
		object.msTextureImageView = createImageView(object.msTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, object.msMipLevels);
	}
}


void DemoApplication::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	//specifies how to interpolate texels that are magnified or minified
	//	great for what we set up for mipmaps
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	//texture space coords
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	//anistrophic filtering
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;				//lower value is better performace

	//which color is returned when sampling beyond the image WITH CLAMP TO BORDER ADDRESSING MODE
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;		//true means you use coords of 0 to textureWidth/Height
														//false means its 0 to 1

	samplerInfo.compareEnable = VK_FALSE;			//true means texels will first be compared to value 
													//	then the result will be used in filter operation
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	//MIP MAPS ADJUST HERE
	for (auto& object : objects)
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;				//change values here with static_cast<float>(mipLevels / k); where k is a real number
		samplerInfo.maxLod = static_cast<float>(object.msMipLevels);
		samplerInfo.mipLodBias = 0.0f;

		if (vkCreateSampler(mpDevSel->selectedDevice(), &samplerInfo, nullptr, &object.msTextureSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture sampler");
		}
	}
}


void DemoApplication::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	//uniform buffers with a new transformation every frame
	for (auto& object : objects)
	{
		object.msUniformBuffers.resize(mSwapChainImages.size());
		object.msUniformBuffersMemory.resize(mSwapChainImages.size());

	
		for (size_t i = 0; i < mSwapChainImages.size(); ++i)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				object.msUniformBuffers[i], object.msUniformBuffersMemory[i]);
		}
	}
}


void DemoApplication::createVertexBuffer() {

	VkDeviceSize bufferSize = sizeof(models.msVertices[0]) * models.msVertices.size();

	//actual staging buffer that only host visible buffer as temp buffer (then we later use device local as actual)
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	/*
	//without create buffer for staging

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;	
	bufferInfo.size = sizeof(mVertices[0]) * mVertices.size();	//easy mem byte size 
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;	//purpose for use
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		//means we cant get this from other queuefamilies


	if (vkCreateBuffer(mpDevSel->selectedDevice(), &bufferInfo, nullptr, &mVertexBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer");
	}

	//we have to know what mem type for GPU so this is the setup for that all
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mpDevSel->selectedDevice(), mVertexBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(mpDevSel->selectedDevice(), &allocInfo, nullptr, &mVertexBufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory");
	}

	vkBindBufferMemory(mpDevSel->selectedDevice(), mVertexBuffer, mVertexBufferMemory, 0);
	*/

	void* data;

	//mapping the buffer memory into CPU accessible memory
	vkMapMemory(mpDevSel->selectedDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);	//doesnt immdeiately copy into buffer mem
	memcpy(data, models.msVertices.data(), (size_t)bufferSize);	//We can do this from the above HOST_COHERENT_BIT
	vkUnmapMemory(mpDevSel->selectedDevice(), stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,		
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, models.msVertexBuffer, models.msVertexBufferMemory);
	/*
		SRC_BIT: Buffer can be used as source in a mem transfer op
		DST_BIT: Buffer can be used as destination in mem transfer op
	*/

	copyBuffer(stagingBuffer, models.msVertexBuffer, bufferSize);	//move vertex data to device local buffer
	vkDestroyBuffer(mpDevSel->selectedDevice(), stagingBuffer, nullptr);		//free out our staging buffer
	vkFreeMemory(mpDevSel->selectedDevice(), stagingBufferMemory, nullptr);
}


VKAPI_ATTR VkBool32 VKAPI_CALL DemoApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	//this all now gets handled in setupDebugMessenger, but this is another simpler way of doing it without
	//the callbacks, its easy and fast
	//cerr (like c error) is basically access to an error stream and can be loaded with <<
	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

	
	//this style allows for comparison statements what to do for what level of message
	//you can find what each had in Vulkan Part 4 of notes or on validation layers chapter
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << "\nWATCH OUT\n";
	}

	//setting up debug info
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	
	//filter what types to be notified about 
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	//you can also pass pUserData via the param
	debugCreateInfo.pfnUserCallback = debugCallback;

	debugCreateInfo.pUserData = nullptr;


	return VK_FALSE;
}


void DemoApplication::drawFrame()
{
	//waits for end of frame to be finished by waiting for signal
	vkWaitForFences(mpDevSel->selectedDevice(), 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;

	//UINT64_MAX specifies a timeout in nanoseconds for an image to become avaiable
	//the semaphores are for at what point in time we present the image
	//	(modded for multiple semaphore use)
	VkResult result = vkAcquireNextImageKHR(mpDevSel->selectedDevice(), mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	//if its out of date we have to check
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to aquire swap chain image");
	}

	//update our UBOs
	updateUniformBuffer(imageIndex);


	//check if previous frame is using this image
	if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(mpDevSel->selectedDevice(), 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	//mark image as being in use by current frame
	mImagesInFlight[imageIndex] = mInFlightFences[mCurrentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//we want to not write colors until image is available
	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	
	//which CBs actually submit for execution
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

	//specify which semaphores to signal once CB(s) have finished execution
	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	//more calls to the wait for fences means this should be here
	vkResetFences(mpDevSel->selectedDevice(), 1, &mInFlightFences[mCurrentFrame]);

	//the last param is for an optional fence so we used one after doing multiple Semaphores
	if (vkQueueSubmit(mpDevSel->graphicsQueue(), 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	//swap chains to present images to
	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	//presentInfo.pResults = nullptr;

	//result from vkAquireNextImageKHR above, reset its value
	result = vkQueuePresentKHR(mpDevSel->presentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFrameBufferResized)
	{
		//recreates if the result is unoptimal
		mFrameBufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	//advance the frame at the end
	//	we index loop after amount of queued frames, in this case MAX_FRAMES_IN_FLIGHt
	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void DemoApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);	//all we need to do is hold the copy, so i ends here

//execute buffer to complete transfer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(mpDevSel->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mpDevSel->graphicsQueue());	//we can do the fences thing here instead (come back to if you have time, can do multiple transfers)
	vkFreeCommandBuffers(mpDevSel->selectedDevice(), mCommandPool, 1, &commandBuffer);	//wipe the buffer after the copy over
}


VkFormat DemoApplication::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}


uint32_t DemoApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mpDevSel->selectedPhysicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&		//means that the Bit we have for thihs type is 1 (good to go)
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)	//back when we did the bitwise for HOST_VISIBLE and HOT_COHERENT this is what it was for
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type");
}



VkFormat DemoApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	//support depends on tiling mode and usage
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(mpDevSel->selectedPhysicalDevice(), format, &props);	//LEFT OFF HERE 11/19/19
	
		//use cases supported with linear tiling
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}

		//uses cases supported with optimal tiling
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format"); //either do this or return special value
}


void DemoApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	//the pointer provided is provided above this functions call in initWindow, 
	//	GLFW needs to identify the pointer of the user to the window first
	auto app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));

	app->mFrameBufferResized = true;
}


void DemoApplication::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
{
	//check if image format can actually support linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(mpDevSel->selectedPhysicalDevice(), imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		throw std::runtime_error("texture image format does not support linear blitting");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = textureWidth;
	int32_t mipHeight = textureHeight;

	for (uint32_t i = 1; i < mipLevels; ++i)
	{
		//same as how all barriers are set up
		//	remember, we are generating a 2D surface, all Z values are 1 and shit is linear

		barrier.subresourceRange.baseMipLevel = i - 1;	
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;	//waits for i - 1 to be filled
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		//our blit function
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };	//3D region data blitted from
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;		//our source iter place
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };	//regions divided by two beause we are halving
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;			//our destination iter place
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,	//blitting between different levels of same image
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	//	so we use same image for both
			1, &blit, VK_FILTER_LINEAR);	//same filter options as vksampler use linear to enable interpolation

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//transition mip level i - 1 to shader read only optimal
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		//divide mip levels by 2
		//	also handle if image isnt square
		if (mipWidth > 1)
		{
			mipWidth /= 2;
		}

		if (mipHeight > 1)
		{
			mipHeight /= 2;
		}

	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}




std::vector<const char*> DemoApplication::getRequiredExtensions()
{
	//agnostic API (extensions to interface with OS required)
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	//GLFW offers built in extension handler
	//get num of extensions in instance, and set the number of them on the uint32_t
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//get the actual extensions and put them in a vector
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers == true)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


bool DemoApplication::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void DemoApplication::initGUIWindow()
{
//	mpIGWindow->Surface = mIGSurface;
//	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mpDevSel->selectedPhysicalDevice());
//	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
//	const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
//
//	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
//	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
//	mpIGWindow->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(mpDevSel->selectedPhysicalDevice(), mpIGWindow->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);
//
//	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
//	mpIGWindow->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(mpDevSel->selectedPhysicalDevice(), mpIGWindow->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
//
//	//IM_ASSERT(g_MinImageCount >= 2);
//	ImGui_ImplVulkanH_CreateWindow(mInstance, mpDevSel->selectedPhysicalDevice(), mpDevSel->selectedDevice(), mpIGWindow, (uint32_t) - 1, nullptr, WINDOW_WIDTH, WINDOW_HEIGHT, 2);
//

	//we resize shit later, so turn that off
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	////create the window
	////	fourth param is for opening on a specific monitor, fifth is only for OpenGL stuff
	//mpIGWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan3D Options", nullptr, nullptr);

	//glfwSetWindowUserPointer(mpIGWindow, this);

	////this will autofill the arguements for us based on the window passed alongside it
	//glfwSetFramebufferSizeCallback(mpIGWindow, framebufferResizeCallback);

}

void DemoApplication::initScene()
{
	sourced3D obj1;
	sourced3D obj2;
	sourced3D obj3;

	obj1.msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0, 0.0, 1.0));
	obj2.msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(10.0, 0.0, 1.0));
	obj3.msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 10.0, 1.0));

	light3D light1;

	light1.lightPos = glm::vec3(2.0, 12.0, 7.0);
	light1.lightColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
	light1.lightIntensity = 4.0;
	light1.lightSize = 20.0;


	mScene.storeObject(obj1);
	mScene.storeObject(obj2);
	mScene.storeObject(obj3);

	mScene.storeLight(light1);
}

void DemoApplication::initVulkan()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	selectDevice();
	mpDevSel->initDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createColorResources();
	createDepthResource();
	createFramebuffers();
	createCommandPool();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	loadModel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}


void DemoApplication::initWindow() 
{

	//needed by GLFW
	glfwInit();

	//for non opengl 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//we resize shit later, so turn that off
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//create the window
	//	fourth param is for opening on a specific monitor, fifth is only for OpenGL stuff
	mpWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan3D Render", nullptr, nullptr);

	glfwSetWindowUserPointer(mpWindow, this);

	//this will autofill the arguements for us based on the window passed alongside it
	glfwSetFramebufferSizeCallback(mpWindow, framebufferResizeCallback);
}



void DemoApplication::loadModel()
{
	tinyobj::attrib_t attrib;	//this holds vertices, normals, and texcoord vectors
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};	//need to use this for index buffer
																//	removal of duplicate data
	
	//combine all faces in file into a single model (triangulation)
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			//using out predefined vertex struct
			Vertex vertex = {};

			
				vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],	//its stored as array of floats so we 
				attrib.vertices[3 * index.vertex_index + 1],	// need to multiply the index by 3 to compensate
				attrib.vertices[3 * index.vertex_index + 2]		//	for glm::vec3. Offsets 0, 1, and 2 = x, y, and z
			};

			vertex.textureCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //had to flip for vert component
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			//std::cout << " " << attrib.normals[3 * index.normal_index + 0] << " " << attrib.normals[3 * index.normal_index + 1] << " " << attrib.normals[3 * index.normal_index + 2] << std::endl;

			vertex.color = { 1.0f, 1.0f, 1.0f };

			//make sure we dont have duplicate vertices data
			//	check if we have already seen same pos (== is overloaded in Vertex)
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(models.msVertices.size());
				models.msVertices.push_back(vertex);
			}

			//only push back non repeated vertices
			models.msIndices.push_back(uniqueVertices[vertex]);
		}
	}
	
}


void DemoApplication::mainLoop() 
{
	bool show_options_window = true;

	while (!glfwWindowShouldClose(mpWindow))
	{
		glfwPollEvents(); //the min rec for this, keep it running till we get polled for an error
		drawFrame();	  //draw the frame 

	}

	vkDeviceWaitIdle(mpDevSel->selectedDevice());
} 



void DemoApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
{
	//setting up debug info
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	//filter what types to be notified about 
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	//you can also pass pUserData via the param
	debugCreateInfo.pfnUserCallback = debugCallback;
}

void DemoApplication::selectDevice()
{
	//DeviceSelection devSel(mpWindow, mSurface, mInstance);

	//mpDevSel->selectedDevice() = devSel.selectedDevice();
	//mpDevSel->selectedPhysicalDevice() = devSel.selectedPhysicalDevice();
	mpDevSel = new DeviceSelection(mpWindow, mSurface, mInstance);
}



std::vector<char> DemoApplication::readFile(const std::string& filename)
{
	//ate: start reading at eof
	//binary: read file as binary
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	//by starting at the end of the file we can determine size based on position
	size_t filesize = (size_t)file.tellg();
	std::vector<char> buffer(filesize);

	//go back to beginning, then read all the bytes at once
	file.seekg(0);
	file.read(buffer.data(), filesize);

	//close the file
	file.close();

	return buffer;
}


void DemoApplication::recreateSwapChain()
{
	//actual resize event handling
	int width = 0;
	int height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(mpWindow, &width, &height);
		glfwWaitEvents();
	}

	//we dont want to touch resources that may still be in use
	vkDeviceWaitIdle(mpDevSel->selectedDevice());

	//wipe the current swapChain (mem leaks and whatnot)
	cleanupSwapChain();

	//each of these depend of the creation of the other coming afterwards (dependencies down
	//	the chain)
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResource();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}


void DemoApplication::setupDebugMessenger()
{
	if (enableValidationLayers == false) return;

	//create the debuge util info extension
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	//use the populate function to set standards
	populateDebugMessengerCreateInfo(debugCreateInfo);

	//check to make sure that its running properly
	if (CreateDebugUtilsMessengerEXT(mInstance, &debugCreateInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}


}


void DemoApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	//barrier can also be used to transition image layouts and tansfer queuefamily ownership
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	
	//if used to transfer queue fam ownership, these should be the indices of the queue fam,
	//	otherwise call the ignore on it (not default feat)
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;	
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;		//only one layer defined because image isnt array

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;	//specify earliest possible stage for pre-barrier ops
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;	//not a real stage within Graphics and Compute pipelines 
															//	its a psuedostage for transfers
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	//you need to specify which types of operations involving resources must happen
	//	before tha barrier
	//barrier.srcAccessMask = 0;
	//barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}


void DemoApplication::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();	//our continuous time from first use
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();	//time since rendering

	glm::vec3 lightSource(2.0f, 12.0f, 7.0f);

	objects[0].msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 1.0, 1.0));	
	objects[1].msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(-20.0f, 0.0f, 0.0f));	
	objects[2].msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -20.0f, 0.0f));	
	objects[3].msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 20.0f, 0.0f));

	//floor
	objects[4].msUBO.model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 1.0f));
	objects[4].msUBO.model = glm::translate(objects[4].msUBO.model, glm::vec3(0.0f, 0.0f, -4.0f));
	objects[4].msUBO.model = glm::rotate(objects[4].msUBO.model, glm::radians(-190.0f),glm::vec3(0.0f, 0.0f, 1.0f));

	//light
	//objects[4].msUBO.model = glm::translate(glm::mat4(1.0f), lightSource);	
	objects[1].msUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));	

	
	//objects[1].msUBO.model = glm::rotate(objects[1].msUBO.model, time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    objects[1].msUBO.model = glm::rotate(objects[1].msUBO.model, time * glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    objects[2].msUBO.model = glm::rotate(objects[2].msUBO.model, time * glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	

	for (auto& object : objects)
	{
		object.msUBO.lightSource = lightSource;
		object.msUBO.eyePos = glm::vec3(20.0f, 20.0f, 30.0f);
		object.msUBO.aspectRatio = mSwapChainExtent.width / (float)mSwapChainExtent.height;
		object.msUBO.screenHeight = (float)mSwapChainExtent.height;
		object.msUBO.screenWidth = (float)mSwapChainExtent.width;
		object.msUBO.time = time;


		object.msUBO.view = glm::lookAt(object.msUBO.eyePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));	//take eye position, center position, and up axis as params
		object.msUBO.proj = glm::perspective(glm::radians(50.0f), object.msUBO.aspectRatio, 0.1f, 70.f);	//takes FOV, aspect ratio, and near and far clipping planes
		object.msUBO.proj[1][1] *= -1;	//WE NEED TO FLIP Y COORD OF CLIPS BECAUSE GLM WAS FOR OPENGL
		
		//light space matrix (for shadowmapping)
		//object.msUBO.lightSpaceMatrix = glm::mat4(1.0f) * glm::lookAt(object.msUBO.lightSource, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f))
		//									* glm::perspective(glm::radians(359.0f), 1.0f, 0.1f, 100.0f);
										

		void* data;
		vkMapMemory(mpDevSel->selectedDevice(), object.msUniformBuffersMemory[currentImage], 0, sizeof(object.msUBO), 0, &data);
		memcpy(data, &object.msUBO, sizeof(object.msUBO));
		vkUnmapMemory(mpDevSel->selectedDevice(), object.msUniformBuffersMemory[currentImage]);
	}
}
