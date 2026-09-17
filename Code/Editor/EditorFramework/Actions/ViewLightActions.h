#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Actions for configuring the engine view light settings.
class W_EDITORFRAMEWORK_DLL WViewLightActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hLightMenu;
  static WActionDescriptorHandle s_hSkyBox;
  static WActionDescriptorHandle s_hSkyLight;
  static WActionDescriptorHandle s_hSkyLightCubeMap;
  static WActionDescriptorHandle s_hSkyLightIntensity;
  static WActionDescriptorHandle s_hDirLight;
  static WActionDescriptorHandle s_hDirLightAngle;
  static WActionDescriptorHandle s_hDirLightShadows;
  static WActionDescriptorHandle s_hDirLightIntensity;
  static WActionDescriptorHandle s_hFog;
  static WActionDescriptorHandle s_hSetAsDefault;
};

class W_EDITORFRAMEWORK_DLL WViewLightButtonAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WViewLightButtonAction, WButtonAction);

public:
  WViewLightButtonAction(const WActionContext& context, const char* szName, WEngineViewLightSettingsEvent::Type button);
  ~WViewLightButtonAction();

  virtual void Execute(const WVariant& value) override;
  void LightSettingsEventHandler(const WEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  WEngineViewLightSettingsEvent::Type m_ButtonType;
  WEngineViewLightSettings* m_pSettings = nullptr;
  WEventSubscriptionID m_SettingsID;
};

class W_EDITORFRAMEWORK_DLL WViewLightSliderAction : public WSliderAction
{
  W_ADD_DYNAMIC_REFLECTION(WViewLightSliderAction, WSliderAction);

public:
  WViewLightSliderAction(const WActionContext& context, const char* szName, WEngineViewLightSettingsEvent::Type button);
  ~WViewLightSliderAction();

  virtual void Execute(const WVariant& value) override;
  void LightSettingsEventHandler(const WEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  WEngineViewLightSettingsEvent::Type m_ButtonType;
  WEngineViewLightSettings* m_pSettings = nullptr;
  WEventSubscriptionID m_SettingsID;
};
