#pragma once

#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginMiniAudio/EditorPluginMiniAudioDLL.h>
#include <Foundation/Strings/String.h>

class W_EDITORPLUGINMINIAUDIO_DLL WMiniAudioProjectPreferences : public WPreferences
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioProjectPreferences, WPreferences);

public:
  WMiniAudioProjectPreferences();
  ~WMiniAudioProjectPreferences();

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
