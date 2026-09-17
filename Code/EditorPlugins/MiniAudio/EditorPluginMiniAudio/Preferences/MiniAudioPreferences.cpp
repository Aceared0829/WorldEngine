#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioProjectPreferences, 1, WRTTIDefaultAllocator<WMiniAudioProjectPreferences>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Mute", m_bMute),
    W_MEMBER_PROPERTY("Volume", m_fMasterVolume)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMiniAudioProjectPreferences::WMiniAudioProjectPreferences()
  : WPreferences(Domain::Project, "MiniAudio")
{
  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WMiniAudioProjectPreferences::ProcessEventHandler, this));
}

WMiniAudioProjectPreferences::~WMiniAudioProjectPreferences()
{
  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WMiniAudioProjectPreferences::ProcessEventHandler, this));
}

void WMiniAudioProjectPreferences::SetMute(bool bMute)
{
  m_bMute = bMute;

  SyncCVars();
}

void WMiniAudioProjectPreferences::SetVolume(float fVolume)
{
  m_fMasterVolume = WMath::Clamp(fVolume, 0.0f, 1.0f);

  SyncCVars();
}

void WMiniAudioProjectPreferences::SyncCVars()
{
  TriggerPreferencesChangedEvent();

  {
    WChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "MiniAudio.Mute";
    msg.m_NewValue = m_bMute;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  {
    WChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "MiniAudio.Volume";
    msg.m_NewValue = m_fMasterVolume;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void WMiniAudioProjectPreferences::ProcessEventHandler(const WEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == WEditorEngineProcessConnection::Event::Type::ProcessRestarted)
  {
    SyncCVars();
  }
}
