#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
// Needed for vulkan.hpp which includes headers that include windows.h which then define min, breaking std::min used in vulkan.hpp :-/
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#endif

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif W_ENABLED(W_PLATFORM_LINUX)
#  define VK_USE_PLATFORM_XCB_KHR
#  include <xcb/xcb.h>

#  include <vulkan/vulkan_core.h>
#  include <vulkan/vulkan_xcb.h>
#elif W_ENABLED(W_PLATFORM_ANDROID)
#  define VK_USE_PLATFORM_ANDROID_KHR
#endif

#define VULKAN_HPP_NO_NODISCARD_WARNINGS // TODO: temporarily disable warnings to make it compile. Need to fix all the warnings later.

#if W_ENABLED(W_VULKAN_DYNAMIC_DISPATCH)
#  define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include <vulkan/vulkan.hpp>

#if W_ENABLED(W_PLATFORM_ANDROID)
#  include <vulkan/vulkan_android.h>
#elif W_ENABLED(W_PLATFORM_WINDOWS)
#  include <vulkan/vulkan_win32.h>
#endif

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define W_RENDERERVULKAN_DLL W_DECL_EXPORT
#  else
#    define W_RENDERERVULKAN_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
// #define VK_LOG_LAYOUT_CHANGES

#define VK_ASSERT_DEBUG(code)                                                                                           \
  do                                                                                                                    \
  {                                                                                                                     \
    auto s = (code);                                                                                                    \
    W_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      W_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), W_SOURCE_FILE, W_SOURCE_LINE);         \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                           \
  do                                                                                                                  \
  {                                                                                                                   \
    auto s = (code);                                                                                                  \
    W_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      W_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), W_SOURCE_FILE, W_SOURCE_LINE);       \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                   \
  do                                                                                                                                                                         \
  {                                                                                                                                                                          \
    auto s = (code);                                                                                                                                                         \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                  \
    {                                                                                                                                                                        \
      WLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", W_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), W_SOURCE_FILE, W_SOURCE_LINE); \
    }                                                                                                                                                                        \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                       \
  do                                                                                                                                                                         \
  {                                                                                                                                                                          \
    auto s = (code);                                                                                                                                                         \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                  \
    {                                                                                                                                                                        \
      WLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", W_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), W_SOURCE_FILE, W_SOURCE_LINE); \
      return s;                                                                                                                                                              \
    }                                                                                                                                                                        \
  } while (false)

#define VK_SUCCEED_OR_RETURN_W_FAILURE(code)                                                                                                                                \
  do                                                                                                                                                                         \
  {                                                                                                                                                                          \
    auto s = (code);                                                                                                                                                         \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                  \
    {                                                                                                                                                                        \
      WLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", W_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), W_SOURCE_FILE, W_SOURCE_LINE); \
      return W_FAILURE;                                                                                                                                                     \
    }                                                                                                                                                                        \
  } while (false)
