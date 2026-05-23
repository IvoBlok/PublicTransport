#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

int main() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available Vulkan layers: " << layerCount << std::endl;
    for (const auto& layer : availableLayers) {
        std::cout << "  - " << layer.layerName << std::endl;
    }

    return 0;
}