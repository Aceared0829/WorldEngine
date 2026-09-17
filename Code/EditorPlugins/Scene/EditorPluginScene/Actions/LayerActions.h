#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WScene2Document;
struct WScene2LayerEvent;

///
class W_EDITORPLUGINSCENE_DLL WLayerActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapContextMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hLayerCategory;
  static WActionDescriptorHandle s_hCreateLayer;
  static WActionDescriptorHandle s_hDeleteLayer;
  static WActionDescriptorHandle s_hSaveLayer;
  static WActionDescriptorHandle s_hSaveActiveLayer;
  static WActionDescriptorHandle s_hLayerLoaded;
  static WActionDescriptorHandle s_hLayerVisible;
  static WActionDescriptorHandle s_hSwitchOnSelection;
};

///
class W_EDITORPLUGINSCENE_DLL WLayerAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WLayerAction, WButtonAction);

public:
  enum class ActionType
  {
    CreateLayer,
    DeleteLayer,
    SaveLayer,
    SaveActiveLayer,
    LayerLoaded,
    LayerVisible,
    SwitchOnSelection,
  };

  WLayerAction(const WActionContext& context, const char* szName, ActionType type);
  ~WLayerAction();

  static void ToggleLayerLoaded(WScene2Document* pM_pSceneDocument, WUuid layerGuid);
  virtual void Execute(const WVariant& value) override;

private:
  void LayerEventHandler(const WScene2LayerEvent& e);
  void DocumentEventHandler(const WDocumentEvent& e);
  void UpdateEnableState();
  WUuid GetCurrentSelectedLayer() const;

private:
  WScene2Document* m_pSceneDocument;
  ActionType m_Type;
};
