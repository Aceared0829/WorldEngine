#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

/// Stores project specific preferences for the current user
class W_EDITORFRAMEWORK_DLL WProjectPreferencesUser : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WProjectPreferencesUser, WPreferences);

public:
  WProjectPreferencesUser();

  // which apps to launch as external 'Players' (other than WPlayer.exe)
  WDynamicArray<WString> m_PlayerApps;

  // the directory where the project should be exported to
  WString m_sExportFolder;

  // path to a folder where shared materials should be stored
  WString m_sSharedMaterialFolder;

  // the default mesh include tag used to indicate that a sub-mesh is an LOD
  WString m_sMeshLodPrefix;
};
