#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class WMaterialDragDropHandler : public WAssetDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialDragDropHandler, WAssetDragDropHandler);

public:
protected:
  virtual void RequestConfiguration(WDragDropConfig* pConfigToFillOut) override;
  virtual float CanHandle(const WDragDropInfo* pInfo) const override;
  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const WDragDropInfo* pInfo) override;
  virtual void OnDragCancel() override;
  virtual void OnDrop(const WDragDropInfo* pInfo) override;

  WUuid m_AppliedToComponent;
  WInt32 m_iAppliedToSlot;
};
