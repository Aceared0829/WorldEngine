#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WRmlUiComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiComponentDragDropHandler, WComponentDragDropHandler);

public:
  float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
