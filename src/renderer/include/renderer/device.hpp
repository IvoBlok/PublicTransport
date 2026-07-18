#pragma once

#include <renderer/settings.hpp>

#include <vector>
#include <optional>
#include <string>
#include <set>

namespace renderer {

    /**
     * Wrapper for the vulkan instance, physical and logical device, and queues. 
     * It also provides helper function for stuff like creating buffers, images,
     * and managing one-time command buffers.
     */
    class Device {
    public:
        Device(bool enableValidation, GLFWwindow* window);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        // Getters
        VkInstance getInstance() const { return instance; }
        VkSurfaceKHR getSurface() const { return surface; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkDevice getDevice() const { return device; }
        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        VkQueue getPresentQueue() const { return presentQueue; }
        uint32_t getGraphicsQueueFamily() const { return graphicsFamily.value(); }
        uint32_t getPresentQueueFamily() const { return presentFamily.value(); }

        // Memory type selection
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        // Buffer and image creation helpers
        VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties, VkDeviceMemory& memory) const;
        VkImage createImage(uint32_t width, uint32_t height, VkFormat format,
                            VkImageTiling tiling, VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties, VkDeviceMemory& memory) const;
        VkImageView createImageView(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspectFlags) const;

        // Copy operations (using one‑time command buffers)
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void transitionImageLayout(VkImage image, VkFormat format,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) const;

        // One‑time command buffer management
        VkCommandBuffer beginSingleTimeCommands() const;
        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };
        SwapChainSupportDetails querySwapChainSupport() const;
        VkSurfaceFormatKHR chooseSurfaceFormat() const;

    private:
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        VkCommandPool singleTimePool = VK_NULL_HANDLE;

        bool validationEnabled;

        void createInstance();
        void createSurface(GLFWwindow* window);
        void pickPhysicalDevice();
        void createLogicalDevice();

        bool checkValidationLayerSupport() const;
        bool checkDeviceExtensionSupport(VkPhysicalDevice phys) const;
        bool isDeviceSuitable(VkPhysicalDevice phys) const;

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice phys) const;

        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
        };
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice phys) const;
    };
}