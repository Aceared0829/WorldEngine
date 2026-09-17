#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginFmod/EditorPluginFmodDLL.h>
#include <Foundation/Strings/String.h>

class W_EDITORPLUGINFMOD_DLL WFmodProjectPreferences : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WFmodProjectPreferences, WPreferences);

public:
  WFmodProjectPreferences();
  ~WFmodProjectPreferences();

  void SetMute(bool bMute);
  bool GetMute() const { return m_bMute; }

  void SetVolume(float fVolume);
  float GetVolume() const { return m_fMasterVolume; }

  void SyncCVars();

private:
  void ProcessEventHandler(const WEditorEngineProcessConnection::Event& e);

  bool m_bMute = false;
  float m_fMasterVolume = 1.0f;
};
