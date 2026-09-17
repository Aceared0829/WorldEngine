#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class W_EDITORFRAMEWORK_DLL WScenePreferencesUser : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WScenePreferencesUser, WPreferences);

public:
  WScenePreferencesUser();

  void SetCameraSpeed(WInt32 value);
  WInt32 GetCameraSpeed() const { return m_iCameraSpeed; }

  void SetShowGrid(bool bShow);
  bool GetShowGrid() const { return m_bShowGrid; }

protected:
  bool m_bShowGrid;
  int m_iCameraSpeed;
};
