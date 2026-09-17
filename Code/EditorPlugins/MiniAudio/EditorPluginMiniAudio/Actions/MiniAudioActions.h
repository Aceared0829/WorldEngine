#pragma once

#include <EditorPluginMiniAudio/EditorPluginMiniAudioDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WPreferences;

class W_EDITORPLUGINMINIAUDIO_DLL WMiniAudioActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapPluginMenuActions(WStringView sMapping);
  static void MapMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategoryMiniAudio;
  static WActionDescriptorHandle s_hMute;
  static WActionDescriptorHandle s_hVolume;
};


class W_EDITORPLUGINMINIAUDIO_DLL WMiniAudioAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioAction, WButtonAction);

public:
  enum class ActionType
  {
    Mute,
  };

  WMiniAudioAction(const WActionContext& context, const char* szName, ActionType type);
  ~WMiniAudioAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);

  ActionType m_Type;
};

class W_EDITORPLUGINMINIAUDIO_DLL WMiniAudioSliderAction : public WSliderAction
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSliderAction, WSliderAction);

public:
  enum class ActionType
  {
    Volume,
  };

  WMiniAudioSliderAction(const WActionContext& context, const char* szName, ActionType type);
  ~WMiniAudioSliderAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
