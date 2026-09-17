#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

/// Base class for drag and drop handler that drop on a WSceneLayer.
class WLayerDragDropHandler : public WDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WLayerDragDropHandler, WDragDropHandler);

public:
  virtual void OnDragBegin(const WDragDropInfo* pInfo) override {}
  virtual void OnDragUpdate(const WDragDropInfo* pInfo) override {}
  virtual void OnDragCancel() override {}

protected:
  const WRTTI* GetCommonBaseType(const WDragDropInfo* pInfo) const;
};

class WLayerOnLayerDragDropHandler : public WLayerDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WLayerOnLayerDragDropHandler, WLayerDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;
  virtual void OnDrop(const WDragDropInfo* pInfo) override;
};

class WGameObjectOnLayerDragDropHandler : public WLayerDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectOnLayerDragDropHandler, WLayerDragDropHandler);

public:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;
  virtual void OnDrop(const WDragDropInfo* pInfo) override;
};
