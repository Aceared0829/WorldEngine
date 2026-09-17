#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/Actions/MiniAudioActions.h>
#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>
#include <GuiFoundation/Action/ActionManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WMiniAudioActions::s_hCategoryMiniAudio;
WActionDescriptorHandle WMiniAudioActions::s_hMute;
WActionDescriptorHandle WMiniAudioActions::s_hVolume;

void WMiniAudioActions::RegisterActions()
{
  s_hCategoryMiniAudio = W_REGISTER_CATEGORY("MiniAudio");
  s_hMute = W_REGISTER_ACTION_1("MiniAudio.Mute", WActionScope::Document, "MiniAudio", "", WMiniAudioAction, WMiniAudioAction::ActionType::Mute);
  s_hVolume = W_REGISTER_ACTION_1("MiniAudio.Volume", WActionScope::Document, "MiniAudio", "", WMiniAudioSliderAction, WMiniAudioSliderAction::ActionType::Volume);
}

void WMiniAudioActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategoryMiniAudio);
  WActionManager::UnregisterAction(s_hMute);
  WActionManager::UnregisterAction(s_hVolume);
}

void WMiniAudioActions::MapPluginMenuActions(WStringView sMapping)
{
  // WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  // W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");
  //  pMap->MapAction(s_hCategoryMiniAudio, "G.Plugins.Settings", 9.0f);

  // no plugin specific menu entries at the moment
}

void WMiniAudioActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryMiniAudio, "G.Scene", 5.0f);
  pMap->MapAction(s_hMute, "G.Scene", "MiniAudio", 0.0f);
  pMap->MapAction(s_hVolume, "G.Scene", "MiniAudio", 1.0f);
}

void WMiniAudioActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pSceneMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryMiniAudio, "", 12.0f);
  pSceneMap->MapAction(s_hMute, "MiniAudio", 0.0f);
}

WMiniAudioAction::WMiniAudioAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::Mute:
    {
      SetCheckable(true);

      WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WMiniAudioAction::OnPreferenceChange, this));

      if (pPreferences->GetMute())
        SetIconPath(":/Icons/SoundOff.svg");
      else
        SetIconPath(":/Icons/SoundOn.svg");

      SetChecked(pPreferences->GetMute());
    }
    break;
  }
}

WMiniAudioAction::~WMiniAudioAction()
{
  if (m_Type == ActionType::Mute)
  {
    WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WMiniAudioAction::OnPreferenceChange, this));
  }
}

void WMiniAudioAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::Mute)
  {
    WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();
    pPreferences->SetMute(!pPreferences->GetMute());

    if (GetContext().m_pDocument)
    {
      GetContext().m_pDocument->ShowDocumentStatus(WFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
    }
  }
}

void WMiniAudioAction::OnPreferenceChange(WPreferences* pref)
{
  WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();

  if (m_Type == ActionType::Mute)
  {
    if (pPreferences->GetMute())
      SetIconPath(":/Icons/SoundOff.svg");
    else
      SetIconPath(":/Icons/SoundOn.svg");

    SetChecked(pPreferences->GetMute());
  }
}

//////////////////////////////////////////////////////////////////////////

WMiniAudioSliderAction::WMiniAudioSliderAction(const WActionContext& context, const char* szName, ActionType type)
  : WSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::Volume:
    {
      WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();

      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WMiniAudioSliderAction::OnPreferenceChange, this));

      SetRange(0, 20);
    }
    break;
  }

  UpdateState();
}

WMiniAudioSliderAction::~WMiniAudioSliderAction()
{
  switch (m_Type)
  {
    case ActionType::Volume:
    {
      WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();
      pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WMiniAudioSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void WMiniAudioSliderAction::Execute(const WVariant& value)
{
  const WInt32 iValue = value.Get<WInt32>();

  switch (m_Type)
  {
    case ActionType::Volume:
    {
      WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();

      pPreferences->SetVolume(iValue / 20.0f);

      if (GetContext().m_pDocument)
      {
        GetContext().m_pDocument->ShowDocumentStatus(WFmt("Sound Volume: {}%%", (int)(pPreferences->GetVolume() * 100.0f)));
      }
    }
    break;
  }
}

void WMiniAudioSliderAction::OnPreferenceChange(WPreferences* pref)
{
  UpdateState();
}

void WMiniAudioSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::Volume:
    {
      WMiniAudioProjectPreferences* pPreferences = WPreferences::QueryPreferences<WMiniAudioProjectPreferences>();

      SetValue(WMath::Clamp((WInt32)(pPreferences->GetVolume() * 20.0f), 0, 20));
    }
    break;
  }
}
