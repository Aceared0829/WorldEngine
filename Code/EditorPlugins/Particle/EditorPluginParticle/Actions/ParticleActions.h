#pragma once

#include <EditorPluginParticle/EditorPluginParticleDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WParticleEffectAssetDocument;
struct WParticleEffectAssetEvent;

class WParticleActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hPauseEffect;
  static WActionDescriptorHandle s_hRestartEffect;
  static WActionDescriptorHandle s_hAutoRestart;
  static WActionDescriptorHandle s_hSimulationSpeedMenu;
  static WActionDescriptorHandle s_hSimulationSpeed[10];
  static WActionDescriptorHandle s_hRenderVisualizers;
};

class WParticleAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WParticleAction, WButtonAction);

public:
  enum class ActionType
  {
    PauseEffect,
    RestartEffect,
    AutoRestart,
    SimulationSpeed,
    RenderVisualizers,
  };

  WParticleAction(const WActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~WParticleAction();

  virtual void Execute(const WVariant& value) override;

private:
  void EffectEventHandler(const WParticleEffectAssetEvent& e);
  void UpdateState();

  WParticleEffectAssetDocument* m_pEffectDocument;
  ActionType m_Type;
  float m_fSimSpeed;
};
