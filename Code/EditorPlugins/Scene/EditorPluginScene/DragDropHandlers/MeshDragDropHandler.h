#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WMeshComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WMeshComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class WAnimatedMeshComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WAnimatedMeshComponentDragDropHandler, WComponentDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
};
