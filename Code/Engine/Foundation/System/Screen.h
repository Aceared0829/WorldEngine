#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

struct WScreenResolution
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiResolutionX = 0;
  WUInt32 m_uiResolutionY = 0;
  WUInt16 m_uiRefreshRate = 0;
  WUInt8 m_uiBitsPerPixel = 0;

  inline bool operator<(const WScreenResolution& rhs) const
  {
    if (m_uiBitsPerPixel != rhs.m_uiBitsPerPixel)
      return m_uiBitsPerPixel < rhs.m_uiBitsPerPixel;

    if (m_uiRefreshRate != rhs.m_uiRefreshRate)
      return m_uiRefreshRate < rhs.m_uiRefreshRate;

    if (m_uiResolutionX != rhs.m_uiResolutionX)
      return m_uiResolutionX < rhs.m_uiResolutionX;

    return m_uiResolutionY < rhs.m_uiResolutionY;
  }
};

/// Describes the properties of a screen
struct W_FOUNDATION_DLL WScreenInfo
{
  WString m_sDisplayID;   ///< Internal name used by the OS to identify the monitor.
  WString m_sDisplayName; ///< Some OS provided name for the screen, typically the manufacturer and model name.

  WInt32 m_iOffsetX;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  WInt32 m_iOffsetY;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  WInt32 m_iResolutionX;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  WInt32 m_iResolutionY;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool m_bIsPrimary;       ///< Whether this is the primary/main screen.

  WDynamicArray<WScreenResolution> m_SupportedResolutions;
};

/// Provides functionality to detect available monitors
class W_FOUNDATION_DLL WScreen
{
public:
  /// Enumerates all available screens. When it returns W_SUCCESS, at least one screen has been found.
  static WResult EnumerateScreens(WDynamicArray<WScreenInfo>& out_screens);

  /// Prints the available screen information to the provided log.
  static void PrintScreenInfo(const WArrayPtr<WScreenInfo>& screens, WLogInterface* pLog = WLog::GetThreadLocalLogSystem());
};
