#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/Preferences/FmodPreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFmodProjectPreferences, 1, WRTTIDefaultAllocator<WFmodProjectPreferences>)
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

WFmodProjectPreferences::WFmodProjectPreferences()
  : WPreferences(Domain::Project, "FMOD")
{
  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WFmodProjectPreferences::ProcessEventHandler, this));
}

WFmodProjectPreferences::~WFmodProjectPreferences()
{
  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WFmodProjectPreferences::ProcessEventHandler, this));
}

void WFmodProjectPreferences::SetMute(bool bMute)
{
  m_bMute = bMute;

  SyncCVars();
}

void WFmodProjectPreferences::SetVolume(float fVolume)
{
  m_fMasterVolume = WMath::Clamp(fVolume, 0.0f, 1.0f);

  SyncCVars();
}

void WFmodProjectPreferences::SyncCVars()
{
  TriggerPreferencesChangedEvent();

  {
    WChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "FMOD.Mute";
    msg.m_NewValue = m_bMute;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  {
    WChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "FMOD.MasterVolume";
    msg.m_NewValue = m_fMasterVolume;

    WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void WFmodProjectPreferences::ProcessEventHandler(const WEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == WEditorEngineProcessConnection::Event::Type::ProcessRestarted)
  {
    SyncCVars();
  }
}
