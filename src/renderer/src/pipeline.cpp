#include <renderer/pipeline.hpp>

#include <tracy/Tracy.hpp>

#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace renderer {

    // ========================================================================
    // Pipeline
    // ========================================================================

    Pipeline::Pipeline(Device& dev, VkPipeline pipe, VkPipelineLayout lay)
        : device(dev), pipeline(pipe), layout(lay) {}

    Pipeline::~Pipeline() { destroy(); }

    Pipeline::Pipeline(Pipeline&& other) noexcept
        : device(other.device), pipeline(other.pipeline), layout(other.layout) {
        other.pipeline = VK_NULL_HANDLE;
        other.layout = VK_NULL_HANDLE;
    }

    Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
        if (this != &other) {
            destroy();
            pipeline = other.pipeline;
            layout = other.layout;
            other.pipeline = VK_NULL_HANDLE;
            other.layout = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Pipeline::destroy() {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device.getDevice(), pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.getDevice(), layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    }

    // ========================================================================
    // PipelineManager
    // ========================================================================

    PipelineManager::PipelineManager(Device& dev, RenderPass& rp, VkDescriptorSetLayout dsl)
        : device(dev), renderPass(rp), descriptorSetLayout(dsl) {}

    const Pipeline& PipelineManager::getPipeline(const PipelineKey& key) {
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;

        auto [newIt, inserted] = cache.emplace(key, createPipeline(key));
        return newIt->second;
    }

    Pipeline PipelineManager::createPipeline(const PipelineKey& key) {
        ZoneScoped;

        // 1. Load shader bytecode
        auto vertCode = readShaderFile(key.vertexShaderPath);
        auto fragCode = readShaderFile(key.fragmentShaderPath);

        VkShaderModule vertModule = createShaderModule(vertCode);
        VkShaderModule fragModule = createShaderModule(fragCode);

        // 2. Shader stages
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

        // 3. Vertex input
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &key.binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(key.attributes.size());
        vertexInput.pVertexAttributeDescriptions = key.attributes.data();

        // 4. Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = key.topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 5. Viewport / scissor (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 6. Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = key.polygonMode;
        rasterizer.lineWidth = key.lineWidth;
        rasterizer.cullMode = key.cullMode;
        rasterizer.frontFace = key.frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 7. Multisample
        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 8. Colour blending
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &blendAttachment;

        // 9. Dynamic states
        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // 10. Pipeline layout (uses the shared descriptor set layout)
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;

        VkPipelineLayout pipelineLayout;
        if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            vkDestroyShaderModule(device.getDevice(), vertModule, nullptr);
            vkDestroyShaderModule(device.getDevice(), fragModule, nullptr);
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // 11. Graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass.get();
        pipelineInfo.subpass = 0;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            vkDestroyPipelineLayout(device.getDevice(), pipelineLayout, nullptr);
            vkDestroyShaderModule(device.getDevice(), vertModule, nullptr);
            vkDestroyShaderModule(device.getDevice(), fragModule, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        // 12. Cleanup shader modules
        vkDestroyShaderModule(device.getDevice(), vertModule, nullptr);
        vkDestroyShaderModule(device.getDevice(), fragModule, nullptr);

        return Pipeline(device, pipeline, pipelineLayout);
    }

    std::vector<char> PipelineManager::readShaderFile(const std::string& relativePath) const {
        const std::vector<std::filesystem::path> searchPaths = {
            std::filesystem::current_path() / "build/src/renderer" / relativePath,
            std::filesystem::current_path() / "src/renderer" / relativePath
        };
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                std::ifstream file(path, std::ios::ate | std::ios::binary);
                if (!file) continue;
                size_t size = static_cast<size_t>(file.tellg());
                std::vector<char> buffer(size);
                file.seekg(0);
                file.read(buffer.data(), size);
                return buffer;
            }
        }
        throw std::runtime_error("Failed to find shader: " + relativePath);
    }

    VkShaderModule PipelineManager::createShaderModule(const std::vector<char>& code) const {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module;
        if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }
        return module;
    }

}