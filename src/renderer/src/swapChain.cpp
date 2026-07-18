#include <renderer/swapChain.hpp>

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace renderer {

    // ------------------------------------------------------------------------
    // Construction / Destruction
    // ------------------------------------------------------------------------

    SwapChain::SwapChain(Device& dev, RenderPass& rp, GLFWwindow* win)
        : device(dev), renderPass(rp), window(win) {
        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    SwapChain::~SwapChain() {
        destroySwapChain();
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept
        : device(other.device),
          renderPass(other.renderPass),
          window(other.window),
          swapChain(other.swapChain),
          imageFormat(other.imageFormat),
          extent(other.extent),
          presentMode(other.presentMode),
          images(std::move(other.images)),
          imageViews(std::move(other.imageViews)),
          framebuffers(std::move(other.framebuffers)),
          invalidated(other.invalidated) {
        other.swapChain = VK_NULL_HANDLE;
        other.invalidated = false;
    }

    // ------------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------------

    VkResult SwapChain::acquireNextImage(uint32_t& imageIndex, VkSemaphore signalSemaphore) {
        return vkAcquireNextImageKHR(device.getDevice(), swapChain, UINT64_MAX,
                                     signalSemaphore, VK_NULL_HANDLE, &imageIndex);
    }

    VkResult SwapChain::present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        return vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
    }

    void SwapChain::recreate() {
        ZoneScoped;

        // Wait for device idle before recreating
        vkDeviceWaitIdle(device.getDevice());

        destroySwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();

        invalidated = false;
    }

    // ------------------------------------------------------------------------
    // Private Helpers
    // ------------------------------------------------------------------------

    void SwapChain::createSwapChain() {
        ZoneScoped;

        auto support = device.querySwapChainSupport();
        VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
        presentMode = choosePresentMode(support.presentModes);
        extent = chooseExtent(support.capabilities);

        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 &&
            imageCount > support.capabilities.maxImageCount) {
            imageCount = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device.getSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Determine queue families
        uint32_t graphicsFamily = device.getGraphicsQueueFamily();
        uint32_t presentFamily = device.getPresentQueueFamily();
        uint32_t queueFamilies[] = { graphicsFamily, presentFamily };

        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilies;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }

        // Retrieve images
        vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device.getDevice(), swapChain, &imageCount, images.data());

        imageFormat = surfaceFormat.format;
    }

    void SwapChain::createImageViews() {
        ZoneScoped;

        imageViews.resize(images.size());
        for (size_t i = 0; i < images.size(); ++i) {
            imageViews[i] = device.createImageView(images[i], imageFormat,
                                                   VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void SwapChain::createFramebuffers() {
        ZoneScoped;

        framebuffers.resize(imageViews.size());
        for (size_t i = 0; i < imageViews.size(); ++i) {
            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = &imageViews[i];
            fbInfo.width = extent.width;
            fbInfo.height = extent.height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device.getDevice(), &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void SwapChain::destroySwapChain() {
        ZoneScoped;

        for (auto fb : framebuffers) {
            if (fb != VK_NULL_HANDLE)
                vkDestroyFramebuffer(device.getDevice(), fb, nullptr);
        }
        framebuffers.clear();

        for (auto iv : imageViews) {
            if (iv != VK_NULL_HANDLE)
                vkDestroyImageView(device.getDevice(), iv, nullptr);
        }
        imageViews.clear();

        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device.getDevice(), swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }
    }

    // ------------------------------------------------------------------------
    // Selection Helpers
    // ------------------------------------------------------------------------

    VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) const {
        for (const auto& fmt : available) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
                fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return fmt;
            }
        }
        return available[0];
    }

    VkPresentModeKHR SwapChain::choosePresentMode(const std::vector<VkPresentModeKHR>& available) const {
        for (auto mode : available) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps) const {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return caps.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };
            actual.width = std::clamp(actual.width, caps.minImageExtent.width, caps.maxImageExtent.width);
            actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            return actual;
        }
    }
    
}