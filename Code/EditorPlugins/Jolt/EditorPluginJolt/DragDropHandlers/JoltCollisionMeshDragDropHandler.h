#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WJoltCollisionMeshComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
