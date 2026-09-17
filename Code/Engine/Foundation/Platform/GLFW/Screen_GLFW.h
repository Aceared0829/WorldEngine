#include <Foundation/FoundationInternal.h>
W_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/Screen.h>
#include <GLFW/glfw3.h>

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

#define W_GLFW_RETURN_FAILURE_ON_ERROR()         \
  do                                              \
  {                                               \
    if (WGlfwError(__FILE__, __LINE__).Failed()) \
      return W_FAILURE;                          \
  } while (false)

WResult WScreen::EnumerateScreens(WDynamicArray<WScreenInfo>& out_Screens)
{
  out_Screens.Clear();

  int iMonitorCount = 0;
  GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
  W_GLFW_RETURN_FAILURE_ON_ERROR();
  if (iMonitorCount == 0)
  {
    return W_FAILURE;
  }

  GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
  W_GLFW_RETURN_FAILURE_ON_ERROR();
  if (pPrimaryMonitor == nullptr)
  {
    return W_FAILURE;
  }

  for (int i = 0; i < iMonitorCount; ++i)
  {
    WScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = glfwGetMonitorName(pMonitors[i]);
    W_GLFW_RETURN_FAILURE_ON_ERROR();

    const GLFWvidmode* mode = glfwGetVideoMode(pMonitors[i]);
    W_GLFW_RETURN_FAILURE_ON_ERROR();
    if (mode == nullptr)
    {
      return W_FAILURE;
    }
    screen.m_iResolutionX = mode->width;
    screen.m_iResolutionY = mode->height;

    glfwGetMonitorPos(pMonitors[i], &screen.m_iOffsetX, &screen.m_iOffsetY);
    W_GLFW_RETURN_FAILURE_ON_ERROR();

    screen.m_bIsPrimary = pMonitors[i] == pPrimaryMonitor;
  }

  return W_SUCCESS;
}
