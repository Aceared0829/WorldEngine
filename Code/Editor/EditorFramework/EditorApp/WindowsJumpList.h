#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

class WRecentFilesList;

/// Helper class for managing Windows taskbar jump lists
class W_EDITORFRAMEWORK_DLL WWindowsJumpList
{
public:
  /// Updates the Windows taskbar jump list with recent projects
  static void UpdateJumpList(const WRecentFilesList& recentProjects);
};

#endif
