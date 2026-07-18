#pragma once

#include <renderer/settings.hpp>
#include <renderer/renderables/renderable.hpp>
#include <renderer/renderables/doubleBuffer.hpp>

#include <vector>
#include <array>

namespace renderer {

    // Vertex structure for lines
    struct LineSetVertex {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    /**
     * A set of connected lines (line strips) stored as a double‑buffered list.
     * Implements Renderable.
     */
    class LineSet : public Renderable {
    public:
        explicit LineSet(Device& device);
        ~LineSet();

        // Non‑copyable, movable
        LineSet(const LineSet&) = delete;
        LineSet& operator=(const LineSet&) = delete;
        LineSet(LineSet&& other) noexcept;
        LineSet& operator=(LineSet&& other) = delete;

        // Renderable interface
        PipelineKey getPipelineKey() const override;
        bool updateGPU(Device& device) override;
        void render(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline pipeline) const override;

        // Double‑buffer writing
        auto& startWrite() { return vertices.startWrite(); }
        void endWrite() { vertices.endWrite(); }

        // Convenience methods for building line strips
        void beginStrip();
        void addPoint(const glm::vec3& pos, const glm::vec3& color);
        void endStrip();

    private:
        Device& device;   // reference to the Device (for buffer creation)

        DoubleBuffer<std::vector<LineSetVertex>> vertices;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        size_t vertexCount = 0;

        // Temporary storage for strip building
        std::vector<glm::vec3> currentStripPoints;
        std::vector<glm::vec3> currentStripColors;

        // Internal helpers
        void destroyBuffers();
        void createBuffers(Device& device, const std::vector<LineSetVertex>& data);
    };

}