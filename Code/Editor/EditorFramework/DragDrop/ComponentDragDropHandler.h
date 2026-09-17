#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class WDocument;
class WDragDropInfo;

class W_EDITORFRAMEWORK_DLL WComponentDragDropHandler : public WAssetDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WComponentDragDropHandler, WAssetDragDropHandler);

protected:
  void CreateDropObject(const WVec3& vPosition, const char* szType, const char* szProperty, const WVariant& value, WUuid parent, WInt32 iInsertChildIndex);

  void AttachComponentToObject(const char* szType, const char* szProperty, const WVariant& value, WUuid ObjectGuid);

  void MoveObjectToPosition(const WUuid& guid, const WVec3& vPosition, const WQuat& qRotation);

  void MoveDraggedObjectsToPosition(WVec3 vPosition, bool bAllowSnap, const WVec3& normal);

  void SelectCreatedObjects();

  void BeginTemporaryCommands();

  void EndTemporaryCommands();

  void CancelTemporaryCommands();

  WDocument* m_pDocument;
  WHybridArray<WUuid, 16> m_DraggedObjects;

  virtual void OnDragBegin(const WDragDropInfo* pInfo) override;

  virtual void OnDragUpdate(const WDragDropInfo* pInfo) override;

  virtual void OnDragCancel() override;

  virtual void OnDrop(const WDragDropInfo* pInfo) override;

  virtual float CanHandle(const WDragDropInfo* pInfo) const override;

  WVec3 m_vAlignAxisWithNormal = WVec3::MakeZero();
  bool m_bSelectionAsRuntimeOverride = true;
};
