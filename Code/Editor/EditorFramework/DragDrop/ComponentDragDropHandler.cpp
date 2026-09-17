#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WComponentDragDropHandler, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

void WComponentDragDropHandler::CreateDropObject(const WVec3& vPosition, const char* szType, const char* szProperty, const WVariant& value, WUuid parent, WInt32 iInsertChildIndex)
{
  WVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  WUuid ObjectGuid = WUuid::MakeUuid();

  WAddObjectCommand cmd;
  cmd.m_Parent = parent;
  cmd.m_Index = iInsertChildIndex;
  cmd.SetType("WGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

  WSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPos;
  W_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");

  AttachComponentToObject(szType, szProperty, value, ObjectGuid);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void WComponentDragDropHandler::AttachComponentToObject(const char* szType, const char* szProperty, const WVariant& value, WUuid ObjectGuid)
{
  auto history = m_pDocument->GetCommandHistory();

  WUuid CmpGuid = WUuid::MakeUuid();

  WAddObjectCommand cmd;

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  W_VERIFY(history->AddCommand(cmd).Succeeded(), "AddCommand failed");

  if (value.IsA<WVariantArray>())
  {
    WResizeAndSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value.Get<WVariantArray>()[0];
    cmd2.m_Index = 0;
    W_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }
  else
  {
    WSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value;
    W_VERIFY(history->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }
}

void WComponentDragDropHandler::MoveObjectToPosition(const WUuid& guid, const WVec3& vPosition, const WQuat& qRotation)
{
  auto history = m_pDocument->GetCommandHistory();

  WSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2).AssertSuccess();

  if (qRotation.IsValid())
  {
    cmd2.m_sProperty = "LocalRotation";
    cmd2.m_NewValue = qRotation;
    history->AddCommand(cmd2).AssertSuccess();
  }
}

void WComponentDragDropHandler::MoveDraggedObjectsToPosition(WVec3 vPosition, bool bAllowSnap, const WVec3& normal)
{
  if (m_DraggedObjects.IsEmpty() || !vPosition.IsValid())
    return;

  if (bAllowSnap)
  {
    WSnapProvider::SnapTranslation(vPosition);
  }

  auto history = m_pDocument->GetCommandHistory();

  WGameObjectDocument* pGameDoc = WDynamicCast<WGameObjectDocument*>(m_pDocument);

  history->StartTransaction("Move to Position");

  WQuat rot;
  rot.SetIdentity();

  if (normal.IsValid() && !m_vAlignAxisWithNormal.IsZero(0.01f))
  {
    rot = WQuat::MakeShortestRotation(m_vAlignAxisWithNormal, normal);
  }

  for (const auto& guid : m_DraggedObjects)
  {
    WVec3 vNewPos = vPosition;
    WQuat qNewRot = rot;

    if (pGameDoc)
    {
      const WDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(guid);
      if (const WDocumentObject* pParent = pObject->GetParent())
      {
        const WTransform tParent = pGameDoc->GetGlobalTransform(pParent);
        const WTransform rRel = WTransform::MakeLocalTransform(tParent, WTransform(vNewPos, qNewRot));

        vNewPos = rRel.m_vPosition;
        qNewRot = rRel.m_qRotation;
      }
    }

    MoveObjectToPosition(guid, vNewPos, qNewRot);
  }

  history->FinishTransaction();
}

void WComponentDragDropHandler::SelectCreatedObjects()
{
  WDeque<const WDocumentObject*> NewSel;
  for (const auto& id : m_DraggedObjects)
  {
    NewSel.PushBack(m_pDocument->GetObjectManager()->GetObject(id));
  }

  if (m_bSelectionAsRuntimeOverride)
  {
    m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection(NewSel);
  }
  else
  {
    m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection({});
    m_pDocument->GetSelectionManager()->SetSelection(NewSel);
  }
}

void WComponentDragDropHandler::BeginTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Adjust Objects");
}

void WComponentDragDropHandler::EndTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
}

void WComponentDragDropHandler::CancelTemporaryCommands()
{
  if (m_DraggedObjects.IsEmpty())
    return;

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void WComponentDragDropHandler::OnDragBegin(const WDragDropInfo* pInfo)
{
  m_pDocument = WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  W_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->StartTransaction("Drag Object");
}

void WComponentDragDropHandler::OnDragUpdate(const WDragDropInfo* pInfo)
{
  WVec3 vPos = pInfo->m_vDropPosition;

  if (vPos.IsNaN() || !pInfo->m_TargetObject.IsValid())
    vPos.SetZero();

  WVec3 vNormal = pInfo->m_vDropNormal;

  if (!vNormal.IsValid() || vNormal.IsZero())
    vNormal = WVec3(1, 0, 0);

  MoveDraggedObjectsToPosition(vPos, !pInfo->m_bShiftKeyDown, vNormal);
}

void WComponentDragDropHandler::OnDragCancel()
{
  CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->CancelTransaction();

  m_DraggedObjects.Clear();

  m_pDocument->GetSelectionManager()->SetRuntimeOverrideSelection({});
}

void WComponentDragDropHandler::OnDrop(const WDragDropInfo* pInfo)
{
  EndTemporaryCommands();
  m_pDocument->GetCommandHistory()->FinishTransaction();

  m_bSelectionAsRuntimeOverride = false;
  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float WComponentDragDropHandler::CanHandle(const WDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" && pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const WDocument* pDocument = WDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  const WRTTI* pRttiScene = WRTTI::FindTypeByName("WSceneDocument");

  if (pRttiScene == nullptr)
    return 0.0f;

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom(pRttiScene))
    return 0.0f;

  return 1.0f;
}
