#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>

class W_EDITORPLUGINPROCGEN_DLL WProcPlacementComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WProcPlacementComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
