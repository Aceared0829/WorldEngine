#include <Core/CorePCH.h>

#if W_ENABLED(W_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Configuration/Startup.h>

#  include <GLFW/glfw3.h>

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#    ifdef APIENTRY
#      undef APIENTRY
#    endif

#    include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#  endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    WLog::Error("GLFW error {}: {}", errorCode, msg);
  }
} // namespace


// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      WLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    else
    {
      // Set the error callback after init, so we don't print an error if init fails.
      glfwSetErrorCallback(&glfwErrorCallback);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    glfwSetErrorCallback(nullptr);
    glfwTerminate();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  WResult WGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if (errorCode != GLFW_NO_ERROR)
    {
      WLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return W_FAILURE;
    }
    return W_SUCCESS;
  }
} // namespace

#  define W_GLFW_RETURN_FAILURE_ON_ERROR()         \
    do                                              \
    {                                               \
      if (WGlfwError(__FILE__, __LINE__).Failed()) \
        return W_FAILURE;                          \
    } while (false)

WWindowGLFW::~WWindowGLFW()
{
  DestroyWindow();
}

WResult WWindowGLFW::InitializeWindow()
{
  W_LOG_BLOCK("WWindowGLFW::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    DestroyWindow();
  }

  W_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  GLFWmonitor* pMonitor = nullptr; // nullptr for windowed, fullscreen otherwise

  switch (m_CreationDescription.m_WindowMode)
  {
    case WWindowMode::WindowResizable:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      W_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case WWindowMode::WindowFixedResolution:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      W_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case WWindowMode::FullscreenFixedResolution:
    case WWindowMode::FullscreenBorderlessNativeResolution:
      if (m_CreationDescription.m_iMonitor == -1)
      {
        pMonitor = glfwGetPrimaryMonitor();
        W_GLFW_RETURN_FAILURE_ON_ERROR();
      }
      else
      {
        int iMonitorCount = 0;
        GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
        W_GLFW_RETURN_FAILURE_ON_ERROR();
        if (m_CreationDescription.m_iMonitor >= iMonitorCount)
        {
          WLog::Error("Can not create window on monitor {} only {} monitors connected", m_CreationDescription.m_iMonitor, iMonitorCount);
          return W_FAILURE;
        }
        pMonitor = pMonitors[m_CreationDescription.m_iMonitor];
      }

      if (m_CreationDescription.m_WindowMode == WWindowMode::FullscreenBorderlessNativeResolution)
      {
        const GLFWvidmode* pVideoMode = glfwGetVideoMode(pMonitor);
        W_GLFW_RETURN_FAILURE_ON_ERROR();
        if (pVideoMode == nullptr)
        {
          WLog::Error("Failed to get video mode for monitor");
          return W_FAILURE;
        }
        m_CreationDescription.m_Resolution.width = pVideoMode->width;
        m_CreationDescription.m_Resolution.height = pVideoMode->height;
        m_CreationDescription.m_Position.x = 0;
        m_CreationDescription.m_Position.y = 0;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        W_GLFW_RETURN_FAILURE_ON_ERROR();
      }

      break;
  }


  glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_CreationDescription.m_bSetForegroundOnInit ? GLFW_TRUE : GLFW_FALSE);
  W_GLFW_RETURN_FAILURE_ON_ERROR();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  W_GLFW_RETURN_FAILURE_ON_ERROR();

  GLFWwindow* pWindow = glfwCreateWindow(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, m_CreationDescription.m_Title.GetData(), pMonitor, NULL);
  W_GLFW_RETURN_FAILURE_ON_ERROR();

  if (pWindow == nullptr)
  {
    WLog::Error("Failed to create glfw window");
    return W_FAILURE;
  }
#  if W_ENABLED(W_PLATFORM_LINUX)
  m_hWindowHandle.type = WWindowHandle::Type::GLFW;
  m_hWindowHandle.glfwWindow = pWindow;
#  else
  m_hWindowHandle = pWindow;
#  endif

  if (m_CreationDescription.m_Position != WVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    W_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowIconifyCallback(pWindow, &WWindowGLFW::IconifyCallback);
  glfwSetWindowSizeCallback(pWindow, &WWindowGLFW::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &WWindowGLFW::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &WWindowGLFW::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &WWindowGLFW::FocusCallback);
  glfwSetKeyCallback(pWindow, &WWindowGLFW::KeyCallback);
  glfwSetCharCallback(pWindow, &WWindowGLFW::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &WWindowGLFW::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &WWindowGLFW::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &WWindowGLFW::ScrollCallback);
  W_GLFW_RETURN_FAILURE_ON_ERROR();

#  if W_ENABLED(W_PLATFORM_LINUX)
  W_ASSERT_DEV(m_hWindowHandle.type == WWindowHandle::Type::GLFW, "not a GLFW handle");
  auto pInput = W_DEFAULT_NEW(WInputDeviceMouseKeyboard_GLFW, m_hWindowHandle.glfwWindow);
#  else
  auto pInput = W_DEFAULT_NEW(WInputDeviceMouseKeyboard_Win, m_hWindowHandle);
#  endif

  pInput->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? WMouseCursorClipMode::ClipToWindowImmediate : WMouseCursorClipMode::NoClip);
  pInput->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_pInputDevice = std::move(pInput);

  m_bInitialized = true;
  WLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return W_SUCCESS;
}

void WWindowGLFW::DestroyWindow()
{
  if (m_bInitialized)
  {
    W_LOG_BLOCK("WWindowGLFW::Destroy");

    m_pInputDevice = nullptr;

#  if W_ENABLED(W_PLATFORM_LINUX)
    W_ASSERT_DEV(m_hWindowHandle.type == WWindowHandle::Type::GLFW, "GLFW handle expected");
    glfwDestroyWindow(m_hWindowHandle.glfwWindow);
#  else
    glfwDestroyWindow(m_hWindowHandle);
#  endif
    m_hWindowHandle = INVALID_INTERNAL_WINDOW_HANDLE_VALUE;

    m_bInitialized = false;
  }
}

WResult WWindowGLFW::Resize(const WSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return W_FAILURE;

#  if W_ENABLED(W_PLATFORM_LINUX)
  W_ASSERT_DEV(m_hWindowHandle.type == WWindowHandle::Type::GLFW, "Expected GLFW handle");
  glfwSetWindowSize(m_hWindowHandle.glfwWindow, newWindowSize.width, newWindowSize.height);
#  else
  glfwSetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#  endif
  W_GLFW_RETURN_FAILURE_ON_ERROR();

  return W_SUCCESS;
}

void WWindowGLFW::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  // if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

#  if W_ENABLED(W_PLATFORM_LINUX)
  W_ASSERT_DEV(m_hWindowHandle.type == WWindowHandle::Type::GLFW, "Expected GLFW handle");
  if (glfwWindowShouldClose(m_hWindowHandle.glfwWindow))
  {
    DestroyWindow();
  }
#  else
  if (glfwWindowShouldClose(m_hWindowHandle))
  {
    DestroyWindow();
  }
#  endif
}

void WWindowGLFW::IconifyCallback(GLFWwindow* window, int iconified)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
    self->OnVisibleChange(!iconified);
}

void WWindowGLFW::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(WSizeU32(static_cast<WUInt32>(width), static_cast<WUInt32>(height)));
  }
}

void WWindowGLFW::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void WWindowGLFW::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void WWindowGLFW::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void WWindowGLFW::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnKey(key, scancode, action, mods);
    }
  }
}

void WWindowGLFW::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCharacter(codepoint);
    }
  }
}

void WWindowGLFW::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCursorPosition(xpos, ypos);
    }
  }
}

void WWindowGLFW::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnMouseButton(button, action, mods);
    }
  }
}

void WWindowGLFW::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<WWindowGLFW*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnScroll(xoffset, yoffset);
    }
  }
}

WWindowHandle WWindowGLFW::GetNativeWindowHandle() const
{
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  return WMinWindows::FromNative<HWND>(glfwGetWin32Window(m_hWindowHandle));
#  else
  return m_hWindowHandle;
#  endif
}

#endif


W_STATICLINK_FILE(Core, Core_Platform_GLFW_Window_GLFW);
