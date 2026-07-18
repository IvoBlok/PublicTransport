#pragma once

#include <renderer/device.hpp>
#include <renderer/renderPass.hpp>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace renderer {

    // ------------------------------------------------------------------------
    // PipelineKey: uniquely identifies a graphics pipeline configuration
    // ------------------------------------------------------------------------

    struct PipelineKey {
        // Shader paths (relative to search paths)
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

        // Vertex input
        VkVertexInputBindingDescription binding{};
        std::vector<VkVertexInputAttributeDescription> attributes;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        float lineWidth = 1.0f;

        bool operator==(const PipelineKey& other) const {
            if (vertexShaderPath != other.vertexShaderPath) return false;
            if (fragmentShaderPath != other.fragmentShaderPath) return false;
            if (binding.binding != other.binding.binding) return false;
            if (binding.stride != other.binding.stride) return false;
            if (binding.inputRate != other.binding.inputRate) return false;
            if (topology != other.topology) return false;
            if (polygonMode != other.polygonMode) return false;
            if (cullMode != other.cullMode) return false;
            if (frontFace != other.frontFace) return false;
            if (lineWidth != other.lineWidth) return false;
            if (attributes.size() != other.attributes.size()) return false;

            for (size_t i = 0; i < attributes.size(); ++i) {
                const auto& a = attributes[i];
                const auto& b = other.attributes[i];
                if (a.location != b.location) return false;
                if (a.binding != b.binding) return false;
                if (a.format != b.format) return false;
                if (a.offset != b.offset) return false;
            }
            
            return true;
        }

        struct Hash {
            size_t operator()(const PipelineKey& key) const noexcept {
                size_t h = std::hash<std::string>{}(key.vertexShaderPath);
                h ^= std::hash<std::string>{}(key.fragmentShaderPath) << 1;
                h ^= std::hash<uint32_t>{}(key.binding.binding) << 2;
                h ^= std::hash<uint32_t>{}(key.binding.stride) << 3;
                h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(key.binding.inputRate)) << 4;
                h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(key.topology)) << 5;
                h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(key.polygonMode)) << 6;
                h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(key.cullMode)) << 7;
                h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(key.frontFace)) << 8;
                h ^= std::hash<float>{}(key.lineWidth) << 9;
                // Hash the attribute vector
                for (const auto& attr : key.attributes) {
                    h ^= std::hash<uint32_t>{}(attr.location) ^
                         std::hash<uint32_t>{}(attr.binding) ^
                         std::hash<VkFormat>{}(attr.format) ^
                         std::hash<uint32_t>{}(attr.offset);
                }
                return h;
            }
        };
    };

    // ------------------------------------------------------------------------
    // Pipeline: Wrapper for VkPipeline and its layout
    // ------------------------------------------------------------------------

    class Pipeline {
    public:
        Pipeline(Device& device, VkPipeline pipeline, VkPipelineLayout layout);
        ~Pipeline();

        // Move-only
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        VkPipeline get() const { return pipeline; }
        VkPipelineLayout getLayout() const { return layout; }

    private:
        Device& device;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;

        void destroy();
    };

    // ------------------------------------------------------------------------
    // PipelineManager – creates and caches pipelines
    // ------------------------------------------------------------------------

    class PipelineManager {
    public:
        // The descriptorSetLayout is shared by all pipelines
        PipelineManager(Device& device, RenderPass& renderPass, VkDescriptorSetLayout descriptorSetLayout);
        ~PipelineManager() = default;

        // Returns a cached or newly created pipeline for the given key
        const Pipeline& getPipeline(const PipelineKey& key);

    private:
        Device& device;
        RenderPass& renderPass;
        VkDescriptorSetLayout descriptorSetLayout;

        std::unordered_map<PipelineKey, Pipeline, PipelineKey::Hash> cache;

        // Internal pipeline creator
        Pipeline createPipeline(const PipelineKey& key);

        // Helpers
        std::vector<char> readShaderFile(const std::string& relativePath) const;
        VkShaderModule createShaderModule(const std::vector<char>& code) const;
    };

}