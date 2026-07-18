#pragma once

#include <renderer/device.hpp>
#include <renderer/renderPass.hpp>

#include <vector>

namespace renderer {

    /**
     * Manages the vulkan swapchain, its images, views and framebuffers.
     */
    class SwapChain {
    public:
        SwapChain(Device& device, RenderPass& renderPass, GLFWwindow* window);
        ~SwapChain();

        // Non‑copyable, movable
        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&& other) noexcept;
        SwapChain& operator=(SwapChain&& other) = delete;

        // Acquire next image from swapchain
        VkResult acquireNextImage(uint32_t& imageIndex, VkSemaphore signalSemaphore);

        // Present the image
        VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore);

        // Recreate swapchain (call when window resized)
        void recreate();
        
        // Setters
        void markValid() { invalidated = false; }
        void markInvalid() { invalidated = true; }

        // Getters
        VkSwapchainKHR getSwapchain() const { return swapChain; }
        VkFormat getFormat() const { return imageFormat; }
        VkExtent2D getExtent() const { return extent; }
        const std::vector<VkFramebuffer>& getFramebuffers() const { return framebuffers; }
        const std::vector<VkImageView>& getImageViews() const { return imageViews; }
        const std::vector<VkImage>& getImages() const { return images; }
        bool isInvalid() const { return invalidated; }

    private:
        Device& device;
        RenderPass& renderPass;
        GLFWwindow* window;

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        VkFormat imageFormat;
        VkExtent2D extent;
        VkPresentModeKHR presentMode;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;

        bool invalidated = false;

        // Internal helpers
        void createSwapChain();
        void createImageViews();
        void createFramebuffers();
        void destroySwapChain();

        // Support queries
        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) const;
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& available) const;
        VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps) const;
    };
}