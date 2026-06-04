#pragma once

#include <renderer/coreTypes.hpp>
#include <renderer/objectTypes/baseType.hpp>

#include <vector>

// TODO: write this into a LineSet class instead, such that it can store multiple lines within a single buffer (saving atomics and draw calls). If done properly, I could probably still keep it usable if I have a set of 1 line total
namespace renderer {
    struct LineSetVertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
    };


    class LineSet {
    public:
        LineSet(VulkanContext& renderContext);
        ~LineSet();
        
		LineSet(const LineSet&) = delete;
		LineSet& operator=(const LineSet&) = delete;

		LineSet(LineSet&& other) noexcept;
		LineSet& operator=(LineSet&& other) noexcept;

        bool needsUpdate() const;
        void updateGPU();

        void render(VkCommandBuffer commandBuffer) const;

        auto& startWrite() { return vertices.startWrite(); }
        void endWrite() { vertices.endWrite(); }

        void beginStrip() {
            currentStripPoints.clear();
            currentStripColors.clear();
        }

        void addPoint(const glm::vec3 pos, glm::vec3 color) {
            currentStripPoints.push_back(pos);
            currentStripColors.push_back(color);
        }

        void endStrip() {
            if (currentStripPoints.size() < 2) return;

            auto& vertices = startWrite();
            for (size_t i = 0; i < currentStripPoints.size(); i++) {
                vertices.push_back({.pos = currentStripPoints[i], .color = currentStripColors[i]});

                // account for the line_list format; to create a connected line from line segments, we need to copy the non-outer points
                if (i > 0 && i < currentStripPoints.size() - 1)
                    vertices.push_back({.pos = currentStripPoints[i], .color = currentStripColors[i]});
            }

            endWrite();
        }

    private:
        void createRenderBuffers(const std::vector<LineSetVertex>& vertices);
        void destroy();

        VulkanContext& renderContext;

        DoubleBuffer<std::vector<LineSetVertex>> vertices;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        size_t vertexCount;

        std::vector<glm::vec3> currentStripPoints;
        std::vector<glm::vec3> currentStripColors;
    };
}