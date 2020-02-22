#pragma once
#include "DeviceSelection.h"


//***************//
//    const      //
//***************//

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;




/*
	Git is a stupid fucking piece of fucking shit
	i took extensive fucking notes on this fucking section and they all fucking vanished because
	git couldnt handle a fucking file over 100 mbs so i had to do a revert on the commit tree to update
	lfs and then ALL THIS FUCKING SHIT GOT LOST. fuck git. i learned this.
*/
struct Vertex {
	glm::vec3 pos;				//v (xyz)
	glm::vec3 color;
	glm::vec2 textureCoord;		//vt (uv)
	glm::vec3 normal;			//vn s

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		/*
		float: VK_FORMAT_R32_SFLOAT
		vec2: VK_FORMAT_R32G32_SFLOAT
		vec3: VK_FORMAT_R32G32B32_SFLOAT
		vec4: VK_FORMAT_R32G32B32A32_SFLOAT
		*/

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}

	//use for unordered map, loading models (equality test and hash calculation)
	bool operator==(const Vertex& rhs) const {
		return pos == rhs.pos && color == rhs.color && textureCoord == rhs.textureCoord && rhs.normal == normal;
		//return pos == rhs.pos && textureCoord == rhs.textureCoord;
	}
};

//this is a hash function that combines fields of a struct (in this case vertex) to create 
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			//return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
			//	(hash<glm::vec2>()(vertex.textureCoord) << 1);
			
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) ^ (hash<glm::vec2>()(vertex.normal)) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.textureCoord) << 1);

			//return (hash<glm::vec3>()(vertex.pos) >> 1 ) ^ (hash<glm::vec2>()(vertex.textureCoord) << 1);
		}
	};
	
}

struct UniformBufferObject
{
	//oh here we go again...
	//they say use align as with this stuff because nested for loops can ruin the contiguous mem

	//alignas(16) glm::mat4 model;
	//alignas(16) glm::mat4 view;
	//alignas(16) glm::mat4 proj;

	//if you GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	//my stuff
	glm::float32 aspectRatio;
	glm::float32 screenHeight;
	glm::float32 screenWidth;
	glm::float32 time;
	glm::vec3 lightSource;
	glm::vec3 eyePos;

	//lighting


};


struct sourced3D
{
	UniformBufferObject msUBO = {};

	std::vector<VkDescriptorSet> msDescriptorSets;

	std::vector<VkBuffer> msUniformBuffers;
	std::vector<VkDeviceMemory> msUniformBuffersMemory;
	uint32_t msMipLevels;
	VkImage msTextureImage;
	VkDeviceMemory msTextureImageMemory;
	VkImageView msTextureImageView;
	VkSampler msTextureSampler;

	//std::string msModelPath = "../models/chalet2.obj";
	//std::string msModelPath = "../models/cube.obj";
	//std::string msTexturePath = "../textures/chalet.jpg";
	std::string msTexturePath = "../textures/grey.jpg";

	std::string msVertShaderPath = "../shaders/vert.spv";
	std::string msFragShaderPath = "../shaders/frag.spv";
	VkPipelineShaderStageCreateInfo msShaderStages;	//this may need a number in it
};



class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();	//preparation
		initGUIWindow();
		mainLoop();
		cleanup();
	}


	struct modelDatas
	{
		VkBuffer msVertexBuffer;
		VkDeviceMemory msVertexBufferMemory;
		VkBuffer msIndexBuffer;
		VkDeviceMemory msIndexBufferMemory;
		std::vector<Vertex> msVertices;
		std::vector<uint32_t> msIndices;
	} models;

	std::array<sourced3D, 5> objects;

private:
	

	//we need initWindow because GLFW stock is OpenGL and we need to tell
	//it not to do that
	void initWindow();

	void initGUIWindow();

	void createInstance();

	void selectDevice();

	void initVulkan();

	void setupDebugMessenger();

	void mainLoop();
	
	void cleanup();

	//additional functions
	//checks if all requested layers are available
	bool checkValidationLayerSupport();

	//returns list of extensions if validation layers are enabled
	std::vector<const char*> getRequiredExtensions();

	//just a fast way to fill a debugCreateInfo
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	////find Queue families for varies GPU shit
	//QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	////choosing a GPU to do the job
	//void pickPhysicalDevice();

	////logical device creation to link it to the physical GPU
	//void createLogicalDevice();
	//
	////checks if GPU is suitable to perform operations we want
	////returns true if is supported and has a geometry shader
	//bool isDeviceSuitable(VkPhysicalDevice device);

	////more methodical way of picking a GPU, gives a score based on the 
	////available features at hand, trumps isDeviceSuitable
	//int rateDeviceSuitability(VkPhysicalDevice device);

	////part of swapchain stuff, also fits into device choice
	//bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	//GLFW dependent surface creation
	void createSurface();

	////to fill data in struct for requirements of swapchain
	//SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	//SC PROP 1: surface format detailing search
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	//SC PROP 2: Presentation modes: the only mode garaunteed is VK_PRESENT_MODE_FIFO_KHR
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	//SC PROP 3: resolution of the swap chain images (usually just resolution of the window)
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	//finally create the swapchain given the properties functions
	void createSwapChain();

	//basic image views for every image in swapchain to use them as color targets
	void createImageViews();

	//graphicsPipleine creation (vertex shader -> tesselation -> geometry shader ...)
	void createGraphicsPipeline();

	//a wrapper for SPIR-V code to get into the pipeline
	VkShaderModule createShaderModule(const std::vector<char>& code);

	//framebuffer attachments used while rendering
	void createRenderPass();

	//currently set up for renderpas to expect a single framebuffer with same format as swapchain images
	void createFramebuffers();

	//command pools for mem management of command buffers
	void createCommandPool();

	//records and allocates commands for each swap chain image
	void createCommandBuffers();

	//finally draw the frame
	void drawFrame();

	//sync checking (both semaphores (for GPU-GPU) and fences (for CPU_GPU))
	void createSyncObjects();

	//recreating the swapchain if window resizes to not crash
	void recreateSwapChain();

	//for recreateSC and also cutting into regular cleanup
	void cleanupSwapChain();

	//vertex buffers
	void createVertexBuffer();

	//gets the type of memory used by GPU
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	//general buffer info and memory alloc/ bind creation
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
						VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	//general copy command used in place of vkMapMemory (we can use device local mem types)
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	//index buffer (very similar to createVertexBuffer)
	void createIndexBuffer();

	//Descriptor layout for during pipeline creation
	void createDescriptorSetLayout();

	//create uniform buffers for MVP or skeletal animation stuff
	void createUniformBuffers();

	//create uniform buffer 
	void updateUniformBuffer(uint32_t currentImage);

	//we need to allocate from a pool like CBs
	void createDescriptorPool();

	//allocate descriptor sets
	void createDescriptorSets();

	//image loading into buffers
	void createTextureImage();

	//loading buffer into image objects
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
							VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	//record and execute command buffer again
	VkCommandBuffer beginSingleTimeCommands();

	//end cb
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	//layout transitions to finish the vkCmdCopyBufferToImage command
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	
	//called before finishing createTextureImage
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	//access through image views rather than directly
	void createTextureImageView();

	//generalized createImageView function (imageViews/ textureImageViews)
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	//sampler stuff
	void createTextureSampler();

	//basically we set stuff up with image, memory, and image view
	void createDepthResource();

	//find support for format of depth testing
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	//select format with depth component that supporst usage as depth attachment
	VkFormat findDepthFormat();

	//if chosen depth has stencil component return true
	bool hasStencilComponent(VkFormat format);

	//model loading
	void loadModel();

	//mipmap generation, uses a mem barrier and CB
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels);

	////be able to check how high we can go on antialiasing/ multisampling
	//VkSampleCountFlagBits getMaxUsableSampleCount();

	//multisampled color buffer
	void createColorResources();

	//for shadowmapping
	void createDepthSampler();

	//----------------------//
	//		static stuff	//
	//----------------------//
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
														void* pUserData);

	//shader loading
	static std::vector<char> readFile(const std::string& filename);

	//resizing (has to be static because GLFW cant call member function with "this" pointer
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


	//*********************//
	//     Member Data     //
	//*********************//

	//consts
	const int MAX_FRAMES_IN_FLIGHT = 2;
	
	//model paths
	//const std::string MODEL_PATH = "../models/lulamerga.OBJ";
	//const std::string TEXTURE_PATH = "../textures/internal_ground_ao_texture.jpeg";
	//const std::string MODEL_PATH = "../models/chalet2.obj";
	const std::string MODEL_PATH = "../models/cube.obj";
	//const std::string TEXTURE_PATH = "../textures/chalet.jpg";
	//const std::string TEXTURE_PATH = "../textures/grey.jpg";

	//AUTOMATED VERTICES AND INDICES
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;

	//GLFW
	GLFWwindow* mpWindow;

	//ImGUI Vulkan
	//ImGui_ImplVulkanH_Window* mpIGWindow;
	//VkSurfaceKHR mIGSurface;

	//ImGUI OpenGL
	GLFWwindow* mpIGWindow;

	//Instance creation
	VkInstance mInstance;
	VkApplicationInfo mAppInfo;
	VkInstanceCreateInfo mCreateInfo;

	//surfacing
	VkSurfaceKHR mSurface;

	//swapchain
	VkSwapchainCreateInfoKHR mSwapChainCreateInfo;
	VkSwapchainKHR mSwapChain;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;

	//Image
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;

	//final pipeline
	VkRenderPass mRenderPass;
	VkDescriptorSetLayout mDescriptorSetLayout;
	VkPipelineLayout mPipelineLayout;
	VkPipeline mGraphicsPipeline;

	//framebuffers
	std::vector<VkFramebuffer> mSwapChainFrameBuffers;

	//command buffers
	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;

	//Semaphores for sync
	VkSemaphore mImageAvailableSemaphore;
	VkSemaphore mRenderFinishedSemaphore;

	//Frame Delay and Waiting (with semaphores)
	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mInFlightFences;	//to handle CPU-GPU sync
	std::vector<VkFence> mImagesInFlight;	//to prevent rendering images already in flight
	size_t mCurrentFrame = 0;	//to keep track of when to use right semaphore
	bool mFrameBufferResized = false;	//flags when resize of window happens



	VkDescriptorPool mDescriptorPool;


	//depth testing
	VkImage mDepthImage;
	VkDeviceMemory mDepthImageMemory;
	VkImageView mDepthImageView;
	VkSampler mDepthImageSampler;

	//msaa
	VkSampleCountFlagBits mMSAASamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage mColorImage;		//this is to store desired samples per pixel
	VkDeviceMemory mColorImageMemory;
	VkImageView mColorImageView;

	//Debugging
	VkDebugUtilsMessengerEXT mDebugMessenger;

	//storing extension property information (for myself)
	std::vector<VkExtensionProperties>* mpExtensionsProps;

	//VkPhysicalDevice mPhysicalDevice;
	DeviceSelection* mpDevSel;

	//Query
};