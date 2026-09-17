#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WSoundEventComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WSoundEventComponentDragDropHandler, WComponentDragDropHandler);

public:
  float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
