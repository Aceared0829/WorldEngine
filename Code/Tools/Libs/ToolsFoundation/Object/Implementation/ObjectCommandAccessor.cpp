#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectCommandAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WObjectCommandAccessor::WObjectCommandAccessor(WCommandHistory* pHistory)
  : WObjectDirectAccessor(const_cast<WDocumentObjectManager*>(pHistory->GetDocument()->GetObjectManager()))
  , m_pHistory(pHistory)
{
}

void WObjectCommandAccessor::StartTransaction(WStringView sDisplayString)
{
  m_pHistory->StartTransaction(sDisplayString);
}

void WObjectCommandAccessor::CancelTransaction()
{
  m_pHistory->CancelTransaction();
}

void WObjectCommandAccessor::FinishTransaction()
{
  m_pHistory->FinishTransaction();
}

void WObjectCommandAccessor::BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pHistory->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void WObjectCommandAccessor::CancelTemporaryCommands()
{
  m_pHistory->CancelTemporaryCommands();
}

void WObjectCommandAccessor::FinishTemporaryCommands()
{
  m_pHistory->FinishTemporaryCommands();
}

WStatus WObjectCommandAccessor::SetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  WSetObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

WStatus WObjectCommandAccessor::InsertValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  WInsertObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

WStatus WObjectCommandAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  WRemoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

WStatus WObjectCommandAccessor::MoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  WMoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_OldIndex = oldIndex;
  cmd.m_NewIndex = newIndex;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

WStatus WObjectCommandAccessor::AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  WAddObjectCommand cmd;
  cmd.m_Parent = pParent ? pParent->GetGuid() : WUuid();
  cmd.m_Index = index;
  cmd.m_pType = pType;
  cmd.m_NewObjectGuid = inout_objectGuid;
  cmd.m_sParentProperty = pParentProp ? pParentProp->GetPropertyName() : "Children";
  WStatus res = m_pHistory->AddCommand(cmd);
  if (res.Succeeded())
    inout_objectGuid = cmd.m_NewObjectGuid;
  return res;
}

WStatus WObjectCommandAccessor::RemoveObject(const WDocumentObject* pObject)
{
  WRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  return m_pHistory->AddCommand(cmd);
}

WStatus WObjectCommandAccessor::MoveObject(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index)
{
  WMoveObjectCommand cmd;
  cmd.m_NewParent = pNewParent ? pNewParent->GetGuid() : WUuid();
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sParentProperty = pParentProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}
