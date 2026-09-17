#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectAccessorBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WObjectAccessorBase::StartTransaction(WStringView sDisplayString) {}


void WObjectAccessorBase::CancelTransaction() {}


void WObjectAccessorBase::FinishTransaction() {}


void WObjectAccessorBase::BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void WObjectAccessorBase::CancelTemporaryCommands() {}


void WObjectAccessorBase::FinishTemporaryCommands() {}


WStatus WObjectAccessorBase::GetValueByName(const WDocumentObject* pObject, WStringView sProp, WVariant& out_value, WVariant index /*= WVariant()*/)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


WStatus WObjectAccessorBase::SetValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


WStatus WObjectAccessorBase::InsertValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


WStatus WObjectAccessorBase::RemoveValueByName(const WDocumentObject* pObject, WStringView sProp, WVariant index /*= WVariant()*/)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


WStatus WObjectAccessorBase::MoveValueByName(const WDocumentObject* pObject, WStringView sProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


WStatus WObjectAccessorBase::GetCountByName(const WDocumentObject* pObject, WStringView sProp, WInt32& out_iCount)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


WStatus WObjectAccessorBase::AddObjectByName(const WDocumentObject* pParent, WStringView sParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  const WAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

WStatus WObjectAccessorBase::MoveObjectByName(const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProp, const WVariant& index)
{
  const WAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


WStatus WObjectAccessorBase::GetKeysByName(const WDocumentObject* pObject, WStringView sProp, WDynamicArray<WVariant>& out_keys)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


WStatus WObjectAccessorBase::GetValuesByName(const WDocumentObject* pObject, WStringView sProp, WDynamicArray<WVariant>& out_values)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const WDocumentObject* WObjectAccessorBase::GetChildObjectByName(const WDocumentObject* pObject, WStringView sProp, WVariant index)
{
  WVariant value;
  if (GetValueByName(pObject, sProp, value, index).Succeeded() && value.IsA<WUuid>())
  {
    return GetObject(value.Get<WUuid>());
  }
  return nullptr;
}

WStatus WObjectAccessorBase::ClearByName(const WDocumentObject* pObject, WStringView sProp)
{
  const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return WStatus(WFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));

  WTempHybridArray<WVariant, 8> keys;
  WStatus res = GetKeys(pObject, pProp, keys);
  if (res.Failed())
    return res;

  for (WInt32 i = keys.GetCount() - 1; i >= 0; --i)
  {
    res = RemoveValue(pObject, pProp, keys[i]);
    if (res.Failed())
      return res;
  }
  return WStatus(W_SUCCESS);
}

const WAbstractProperty* WObjectAccessorBase::FindPropertyByName(const WDocumentObject* pObject, WStringView sProp)
{
  return pObject->GetType()->FindPropertyByName(sProp);
}

WObjectAccessorBase::WObjectAccessorBase(const WDocumentObjectManager* pManager)
  : m_pConstManager(pManager)
{
}

WObjectAccessorBase::~WObjectAccessorBase() = default;

const WDocumentObjectManager* WObjectAccessorBase::GetObjectManager() const
{
  return m_pConstManager;
}

void WObjectAccessorBase::FireDocumentObjectStructureEvent(const WDocumentObjectStructureEvent& e)
{
  m_pConstManager->m_StructureEvents.Broadcast(e);
}

void WObjectAccessorBase::FireDocumentObjectPropertyEvent(const WDocumentObjectPropertyEvent& e)
{
  m_pConstManager->m_PropertyEvents.Broadcast(e);
}
