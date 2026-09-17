#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WVisualScriptComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WVisualScriptComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
