#pragma once
#include <format>
#include <stdexcept>
#include <string>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

namespace enginez::err {
    enum class Code {
        NO_ERROR = 0,
        GLFW_INIT_FAIL,
        SURFACE_CREATION_FAIL,
        SWAPCHAIN_CREATION_FAIL,
        IMAGE_CREATION_FAIL,
        IMAGE_VIEW_CREATION_FAIL,
        NO_SUITABLE_FORMAT,
        NO_SUITABLE_PRESENT_MODE,
        VULKAN_CALL_FAIL,
        COLOR_SPACE_NOT_PRESENT,
        DESCRIPTOR_SET_LAYOUT_CREATION_FAILED,
        PIPELINE_LAYOUT_CREATION_FAILED,
        PIPELINE_CREATION_FAILED,
        DESCRIPTOR_POOL_CREATION_FAILED,
        DESCRIPTOR_ALLOCATION_FAILED
    };
    class ezError : public std::runtime_error {
      public:
        template <typename... Args>
        ezError(Code code, std::format_string<Args...> fmt, Args&&... args)
            : std::runtime_error(std::format("[{}] {}", codeName(code), std::format(fmt, std::forward<Args>(args)...))), code(code),
              vkResult(VK_SUCCESS) {
        }

        template <typename... Args>
        ezError(Code code, VkResult result, std::format_string<Args...> fmt, Args&&... args)
            : std::runtime_error(std::format("[{}] {}: {}", codeName(code), std::format(fmt, std::forward<Args>(args)...), string_VkResult(result))),
              code(code), vkResult(result) {
        }

        Code code;
        VkResult vkResult;

        static const char* codeName(Code c) {
            switch (c) {
            case Code::GLFW_INIT_FAIL:
                return "GLFW_INIT_FAIL";
            case Code::SURFACE_CREATION_FAIL:
                return "SURFACE_CREATION_FAIL";
            case Code::SWAPCHAIN_CREATION_FAIL:
                return "SWAPCHAIN_CREATION_FAIL";
            case Code::IMAGE_CREATION_FAIL:
                return "IMAGE_CREATION_FAIL";
            case Code::IMAGE_VIEW_CREATION_FAIL:
                return "IMAGE_VIEW_CREATION_FAIL";
            case Code::NO_SUITABLE_FORMAT:
                return "NO_SUITABLE_FORMAT";
            case Code::NO_SUITABLE_PRESENT_MODE:
                return "NO_SUITABLE_PRESENT_MODE";
            case Code::VULKAN_CALL_FAIL:
                return "VULKAN_CALL_FAIL";
            }
            return "UNKNOWN";
        }
    };

} // namespace enginez::err
#define VK_CHECK(expr, ...)                                                              \
    do {                                                                                 \
        VkResult _vkr = (expr);                                                          \
        if (_vkr != VK_SUCCESS) throw ezError(ezError::Code::VULKAN_CALL_FAIL, _vkr, __VA_ARGS__); \
    } while (0)

#define ERR(a) return std::expected<a, enginez::err::Code>(a)