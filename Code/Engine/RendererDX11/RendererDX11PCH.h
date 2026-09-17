#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#ifdef W_USE_DXVK
// These are shims to make DXVK compile our RendererDX11 library

#  if W_ENABLED(W_PLATFORM_LINUX)
#    define INFINITE 0xFFFFFFFF
#  endif

#  include <Core/System/Window.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <windows_base.h>
namespace WMinWindows
{
  template <>
  struct ToNativeImpl<WWindowHandle>
  {
    using type = ::HWND;
    static W_ALWAYS_INLINE ::HWND ToNative(WWindowHandle hWnd)
    {
      W_ASSERT_DEV(hWnd.type == WWindowHandle::Type::GLFW, "Only GLFW is supported on DXVK");
      return reinterpret_cast<::HWND>(hWnd.glfwWindow);
    }
  };
} // namespace WMinWindows
#endif
