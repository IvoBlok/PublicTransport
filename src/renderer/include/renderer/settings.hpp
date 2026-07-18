#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLMF_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <vector>
#include <array>

namespace renderer {
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    const float NEAR_PLANE = 0.01f;
    const float FAR_PLANE = 10.f;

    const float DEFAULT_CAMERA_MOVE_VELOCITY = 0.7f;
    const float DEFAULT_CAMERA_ROTATE_VELOCITY = 0.7f;
    const glm::vec4 CLEAR_COLOR = glm::vec4{ 0.7f, 0.7f, 0.7f, 1.f };

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
        VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
    };

    #ifdef NDEBUG
    const bool enableValidationLayers = false;
    #else
    const bool enableValidationLayers = true;
    #endif

    // TODO: move UBO somewhere else
    // Uniform buffer shared by all shaders
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
}