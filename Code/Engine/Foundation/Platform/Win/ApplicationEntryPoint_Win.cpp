#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/ApplicationEntryPoint_Platform.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

namespace WApplicationDetails
{
  void SetConsoleCtrlHandler(WMinWindows::BOOL(W_WINDOWS_WINAPI* consoleHandler)(WMinWindows::DWORD dwCtrlType))
  {
    ::SetConsoleCtrlHandler(consoleHandler, TRUE);
  }

  static WMutex s_shutdownMutex;

  WMutex& GetShutdownMutex()
  {
    return s_shutdownMutex;
  }

} // namespace WApplicationDetails
#endif
