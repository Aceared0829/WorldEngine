
#if W_ENABLED(W_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

using WWindowHandle = WMinWindows::HWND;
using WWindowInternalHandle = GLFWwindow*;
#  define INVALID_WINDOW_HANDLE_VALUE (WWindowHandle)(0)
#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE nullptr

#else

#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <InputDevice_Platform.h>

using WWindowHandle = WMinWindows::HWND;
using WWindowInternalHandle = WWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE (WWindowHandle)(0)

#endif
