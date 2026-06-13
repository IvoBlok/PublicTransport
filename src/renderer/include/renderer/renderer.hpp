#pragma once

#include <renderer/coreTypes.hpp>
#include <translation/ComputeWrapperBase.hpp>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <optional>
#include <iostream>

namespace renderer {
    class RenderEngine {
    public:
        RenderEngine();
        ~RenderEngine();

        void initialize();
        void handleFrame();
        void cleanup();
    
        bool shouldWindowClose();
        renderer::VulkanContext& getContext() const;

        std::vector<std::unique_ptr<ComputeWrapperBase>> wrappers;

    private:
        struct VulkanInternals;
        std::unique_ptr<VulkanInternals> internals;
    };


    struct RenderEngine::VulkanInternals {
        #ifdef TRACY_ENABLE
        tracy::VkCtx* tracyContext = nullptr;
        PFN_vkResetQueryPoolEXT vkResetQueryPool = nullptr;
        PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomains = nullptr;
        PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestamps = nullptr;
        #else
        void* tracyContext = nullptr;
        #endif

        uint64_t currentFrame = 0;
        uint64_t nextSignalValue = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> oldCurrentTime;
        std::chrono::microseconds deltaTime;

        // core info
        VulkanContext context;
        GLFWwindow* window;
        VkInstance instance;
        VkSurfaceKHR surface;

        // swapchain related
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages; // TODO switch to std::array with max_frames_in_flight length?
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkSemaphore> renderCompleteSemaphores;
        bool swapchainInvalid;

        // pipeline related
        VkRenderPass renderPass;

        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        std::vector<VkDescriptorSet> UBODescriptorSets;
        std::vector<VkBuffer> uniformBuffers; // holds the uniform buffer for each 'frame in flight'
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        // various variables for syncing when memory is safe to be used
        VkSemaphore timelineSemaphore;
        std::array<FrameResources, MAX_FRAMES_IN_FLIGHT> frameResources;

        // periferal stuff: camera, timing, metrics...
        glm::vec3 cameraPosition;
        glm::vec3 cameraFront;
        glm::vec3 cameraRight;


        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete();
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        VulkanInternals();

        void initWindow();
        void initVulkan();

        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void recreateSwapChain();
        void cleanupSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayouts();

        void createPipelines();

        void createFrameBuffers();
        void createCommandPool();

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage);
        void createDescriptorPool();
        void createDescriptorSets();
        void createCommandBuffers();
        void createSyncObjects();
        void recordCommandBuffer(std::vector<std::unique_ptr<ComputeWrapperBase>>& wrappers, FrameResources& frameResource, int frameIndex, uint32_t imageIndex);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createInstance();
        bool checkValidationLayerSupport();
        void createSurface();

        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        void handleUserInput();
        void cleanup();
    };
}
