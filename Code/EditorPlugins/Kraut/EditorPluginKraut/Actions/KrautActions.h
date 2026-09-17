#pragma once

#include <EditorPluginKraut/EditorPluginKrautDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WKrautTreeAssetDocument;
struct WKrautTreeAssetEvent;

/// Wind strength presets available in the Kraut tree asset editor preview.
struct WKrautWindStrength
{
  enum Enum
  {
    Off,
    Light,
    Moderate,
    Strong,

    Default = Light
  };
};

class WKrautActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);

  static WActionDescriptorHandle s_hCategory;
  static WActionDescriptorHandle s_hWindStrengthMenu;
  static WActionDescriptorHandle s_hWindStrength[4];
  static WActionDescriptorHandle s_hToggleFrondsLeaves;
};

/// Button action used in the Kraut tree asset editor toolbar.
class WKrautAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WKrautAction, WButtonAction);

public:
  enum class ActionType
  {
    WindStrength,       ///< Sets the preview wind strength to a specific WKrautWindStrength preset.
    ToggleFrondsLeaves, ///< Toggles visibility of frond and leaf geometry in the preview.
  };

  WKrautAction(const WActionContext& context, const char* szName, ActionType type, WKrautWindStrength::Enum windStrength = WKrautWindStrength::Light);
  ~WKrautAction();

  virtual void Execute(const WVariant& value) override;

private:
  void KrautEventHandler(const WKrautTreeAssetEvent& e);
  void UpdateState();

  WKrautTreeAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
  WKrautWindStrength::Enum m_WindStrength = WKrautWindStrength::Default;
};
