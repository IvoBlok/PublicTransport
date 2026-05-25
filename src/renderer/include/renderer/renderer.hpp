#pragma once

#include <renderer/coreTypes.hpp>

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

    private:
        struct VulkanInternals;
        std::unique_ptr<VulkanInternals> internals;
    };


    struct RenderEngine::VulkanInternals {
        VulkanContext context;

        GLFWwindow* window;

        VkInstance instance;
        VkSurfaceKHR surface;

        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkRenderPass renderPass;

        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        // holds the uniform buffer for each 'frame in flight'
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        std::vector<VkDescriptorSet> UBODescriptorSets;
        VkDescriptorSet compositeDescriptorSet;

        // holds the command buffer for each 'frame in flight'
        std::vector<VkCommandBuffer> commandBuffers;

        // various variables for syncing when memory is safe to be used
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        // 'frameBufferResized' describes if the user resized the window, triggering an update of internal buffers
        bool frameBufferResized;

        // 'currentFrame' stores which of the 'frames in flight' is currently being used
        uint32_t currentFrame;
        // 'oldCurrentTime' is used to calculate the delta time
        std::chrono::time_point<std::chrono::high_resolution_clock> oldCurrentTime;

        glm::vec3 cameraPosition;
        glm::vec3 cameraFront;
        glm::vec3 cameraRight;
        std::chrono::microseconds deltaTime;


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
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
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
