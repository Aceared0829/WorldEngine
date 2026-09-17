#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class WPrefabComponentDragDropHandler : public WComponentDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WPrefabComponentDragDropHandler, WComponentDragDropHandler);

protected:
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;
  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const WDragDropInfo* pInfo) override;

private:
  void CreatePrefab(const WVec3& vPosition, const WUuid& AssetGuid, WUuid parent, WInt32 iInsertChildIndex);
};
