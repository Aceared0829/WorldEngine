#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WDecalComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WDecalComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
