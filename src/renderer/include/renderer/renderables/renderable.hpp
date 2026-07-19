#pragma once

#include <renderer/device.hpp>
#include <renderer/pipeline.hpp> 

namespace renderer {

    /**
     * Abstract interface for any object that can be rendered.
     * Implementations must provide their own vertex/index buffers,
     * update them when needed, and issue draw commands.
     */
    class Renderable {
    public:
        virtual ~Renderable() = default;

        // Return the unique pipeline key describing how this object should be rendered.
        virtual PipelineKey getPipelineKey() const = 0;

        // Called each frame. Update GPU buffers only if new data is available.
        // Return true if any update actually happened (for debugging/metrics).
        virtual bool updateGPU(Device& device) = 0;

        // Called during command buffer recording.
        // The pipeline is already bound; this method should bind its vertex buffers
        // and call vkCmdDraw (or vkCmdDrawIndexed).
        virtual void render(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline pipeline) const = 0;
    };

} // namespace renderer