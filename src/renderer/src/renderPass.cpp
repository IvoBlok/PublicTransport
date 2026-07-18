#include <renderer/renderPass.hpp>

#include <tracy/Tracy.hpp>

#include <stdexcept>


namespace renderer {
    RenderPass::RenderPass(Device& dev, VkFormat fmt) : device(dev), colorFormat(fmt) {
        createRenderPass();
    }

    RenderPass::~RenderPass() {
        destroy();
    }

    RenderPass::RenderPass(RenderPass&& other) noexcept
        : device(other.device),
          colorFormat(other.colorFormat),
          renderPass(other.renderPass) {
        other.renderPass = VK_NULL_HANDLE;
    }

    void RenderPass::destroy() {
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device.getDevice(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }

    void RenderPass::createRenderPass() {
        ZoneScoped;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = 1;
        rpInfo.pAttachments = &colorAttachment;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device.getDevice(), &rpInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }
}