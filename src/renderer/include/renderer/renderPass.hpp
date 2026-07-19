#pragma once 

#include <renderer/device.hpp>

namespace renderer {
    /**
     * Wrapper for a vulkan VkRenderPass with a single color attachment.
     * Future extensions can add depth/stencil parts.
     */
    class RenderPass {
    public:
        RenderPass(Device& device, VkFormat colorFormat);
        ~RenderPass();

        // Non‑copyable, movable
        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;
        RenderPass(RenderPass&& other) noexcept;
        RenderPass& operator=(RenderPass&&) = delete;

        VkRenderPass get() const { return renderPass; }
        VkFormat getColorFormat() const { return colorFormat; }

    private:
        Device& device;
        VkFormat colorFormat;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        void createRenderPass();
        void destroy();
    };
}