#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/////
class W_EDITORPLUGINSCENE_DLL WSceneGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hGreyBoxingGizmo;
};
