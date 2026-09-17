#pragma once

#include <EditorPluginFmod/EditorPluginFmodDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WPreferences;

class W_EDITORPLUGINFMOD_DLL WFmodActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapPluginMenuActions(WStringView sMapping);
  static void MapMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategoryFmod;
  static WActionDescriptorHandle s_hProjectSettings;
  static WActionDescriptorHandle s_hMuteSound;
  static WActionDescriptorHandle s_hMasterVolume;
};


class W_EDITORPLUGINFMOD_DLL WFmodAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WFmodAction, WButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
    MuteSound,
  };

  WFmodAction(const WActionContext& context, const char* szName, ActionType type);
  ~WFmodAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);

  ActionType m_Type;
};

class W_EDITORPLUGINFMOD_DLL WFmodSliderAction : public WSliderAction
{
  W_ADD_DYNAMIC_REFLECTION(WFmodSliderAction, WSliderAction);

public:
  enum class ActionType
  {
    MasterVolume,
  };

  WFmodSliderAction(const WActionContext& context, const char* szName, ActionType type);
  ~WFmodSliderAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
