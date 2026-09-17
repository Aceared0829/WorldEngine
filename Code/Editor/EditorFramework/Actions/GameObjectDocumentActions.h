#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WPreferences;
struct WGameObjectEvent;
class WGameObjectDocument;
///
class W_EDITORFRAMEWORK_DLL WGameObjectDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping);
  static void MapMenuSimulationSpeed(WStringView sMapping);

  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hGameObjectCategory;
  static WActionDescriptorHandle s_hRenderSelectionOverlay;
  static WActionDescriptorHandle s_hRenderVisualizers;
  static WActionDescriptorHandle s_hRenderShapeIcons;
  static WActionDescriptorHandle s_hRenderGrid;
  static WActionDescriptorHandle s_hAddAmbientLight;
  static WActionDescriptorHandle s_hSimulationSpeedMenu;
  static WActionDescriptorHandle s_hSimulationSpeed[10];
  static WActionDescriptorHandle s_hCameraSpeed;
  static WActionDescriptorHandle s_hPickTransparent;
};

///
class W_EDITORFRAMEWORK_DLL WGameObjectDocumentAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectDocumentAction, WButtonAction);

public:
  enum class ActionType
  {
    RenderSelectionOverlay,
    RenderVisualizers,
    RenderShapeIcons,
    RenderGrid,
    AddAmbientLight,
    SimulationSpeed,
    PickTransparent,
  };

  WGameObjectDocumentAction(const WActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~WGameObjectDocumentAction();

  virtual void Execute(const WVariant& value) override;

private:
  void SceneEventHandler(const WGameObjectEvent& e);
  void OnPreferenceChange(WPreferences* pref);

  float m_fSimSpeed;
  WGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};


class W_EDITORFRAMEWORK_DLL WCameraSpeedSliderAction : public WSliderAction
{
  W_ADD_DYNAMIC_REFLECTION(WCameraSpeedSliderAction, WSliderAction);

public:
  enum class ActionType
  {
    CameraSpeed,
  };

  WCameraSpeedSliderAction(const WActionContext& context, const char* szName, ActionType type);
  ~WCameraSpeedSliderAction();

  virtual void Execute(const WVariant& value) override;

private:
  void OnPreferenceChange(WPreferences* pref);
  void UpdateState();

  WGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};
