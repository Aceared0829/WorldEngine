#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Dialogs/FmodProjectSettingsDlg.moc.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>
#include <GuiFoundation/Action/ActionManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFmodAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFmodSliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WFmodActions::s_hCategoryFmod;
WActionDescriptorHandle WFmodActions::s_hProjectSettings;
WActionDescriptorHandle WFmodActions::s_hMuteSound;
WActionDescriptorHandle WFmodActions::s_hMasterVolume;

void WFmodActions::RegisterActions()
{
  s_hCategoryFmod = W_REGISTER_CATEGORY("FMOD");
  s_hProjectSettings = W_REGISTER_ACTION_1("FMOD.Settings.Project", WActionScope::Document, "FMOD", "", WFmodAction, WFmodAction::ActionType::ProjectSettings);
  s_hMuteSound = W_REGISTER_ACTION_1("FMOD.Mute", WActionScope::Document, "FMOD", "", WFmodAction, WFmodAction::ActionType::MuteSound);
  s_hMasterVolume = W_REGISTER_ACTION_1("FMOD.MasterVolume", WActionScope::Document, "FMOD", "", WFmodSliderAction, WFmodSliderAction::ActionType::MasterVolume);
}

void WFmodActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategoryFmod);
  WActionManager::UnregisterAction(s_hProjectSettings);
  WActionManager::UnregisterAction(s_hMuteSound);
  WActionManager::UnregisterAction(s_hMasterVolume);
}

void WFmodActions::MapPluginMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryFmod, "G.Plugins.Settings", 9.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "FMOD", 0.0f);
}

void WFmodActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryFmod, "G.Scene", 5.0f);
  pMap->MapAction(s_hMuteSound, "G.Scene", "FMOD", 0.0f);
  pMap->MapAction(s_hMasterVolume, "G.Scene", "FMOD", 1.0f);
}

void WFmodActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pSceneMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryFmod, "", 12.0f);
  pSceneMap->MapAction(s_hMuteSound, "FMOD", 0.0f);
}

WFmodAction::WFmodAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AssetIcons/Sound_Event.svg");
      break;

    case ActionType::MuteSound:
    {
      SetCheckable(true);

      WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WFmodAction::OnPreferenceChange, this));

      if (pPreferences->GetMute())
        SetIconPath(":/Icons/SoundOff.svg");
      else
        SetIconPath(":/Icons/SoundOn.svg");

      SetChecked(pPreferences->GetMute());
    }
    break;
  }
}

WFmodAction::~WFmodAction()
{
  if (m_Type == ActionType::MuteSound)
  {
    WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WFmodAction::OnPreferenceChange, this));
  }
}

void WFmodAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    WQtFmodProjectSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  if (m_Type == ActionType::MuteSound)
  {
    WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();
    pPreferences->SetMute(!pPreferences->GetMute());

    if (GetContext().m_pDocument)
    {
      GetContext().m_pDocument->ShowDocumentStatus(WFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
    }
  }
}

void WFmodAction::OnPreferenceChange(WPreferences* pref)
{
  WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();

  if (m_Type == ActionType::MuteSound)
  {
    if (pPreferences->GetMute())
      SetIconPath(":/Icons/SoundOff.svg");
    else
      SetIconPath(":/Icons/SoundOn.svg");

    SetChecked(pPreferences->GetMute());
  }
}

//////////////////////////////////////////////////////////////////////////

WFmodSliderAction::WFmodSliderAction(const WActionContext& context, const char* szName, ActionType type)
  : WSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();

      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WFmodSliderAction::OnPreferenceChange, this));

      SetRange(0, 20);
    }
    break;
  }

  UpdateState();
}

WFmodSliderAction::~WFmodSliderAction()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();
      pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WFmodSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void WFmodSliderAction::Execute(const WVariant& value)
{
  const WInt32 iValue = value.Get<WInt32>();

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();

      pPreferences->SetVolume(iValue / 20.0f);

      if (GetContext().m_pDocument)
      {
        GetContext().m_pDocument->ShowDocumentStatus(WFmt("Sound Volume: {}%%", (int)(pPreferences->GetVolume() * 100.0f)));
      }
    }
    break;
  }
}

void WFmodSliderAction::OnPreferenceChange(WPreferences* pref)
{
  UpdateState();
}

void WFmodSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      WFmodProjectPreferences* pPreferences = WPreferences::QueryPreferences<WFmodProjectPreferences>();

      SetValue(WMath::Clamp((WInt32)(pPreferences->GetVolume() * 20.0f), 0, 20));
    }
    break;
  }
}
