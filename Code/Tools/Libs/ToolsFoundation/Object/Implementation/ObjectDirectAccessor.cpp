#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectDirectAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WObjectDirectAccessor::WObjectDirectAccessor(WDocumentObjectManager* pManager)
  : WObjectAccessorBase(pManager)
  , m_pManager(pManager)
{
}

const WDocumentObject* WObjectDirectAccessor::GetObject(const WUuid& object)
{
  return m_pManager->GetObject(object);
}

WStatus WObjectDirectAccessor::GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index)
{
  if (pProp == nullptr)
    return WStatus("Property is null.");

  WStatus res(W_SUCCESS);
  out_value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index, &res);
  return res;
}

WStatus WObjectDirectAccessor::SetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().SetValue(pProp->GetPropertyName(), newValue, index);
  return bRes ? W_SUCCESS : W_FAILURE;
}

WStatus WObjectDirectAccessor::InsertValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index)
{
  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), index, newValue);
  return bRes ? W_SUCCESS : W_FAILURE;
}

WStatus WObjectDirectAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index)
{
  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), index);
  return WStatus(bRes ? W_SUCCESS : W_FAILURE);
}

WStatus WObjectDirectAccessor::MoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().MoveValue(pProp->GetPropertyName(), oldIndex, newIndex);
  return WStatus(bRes ? W_SUCCESS : W_FAILURE);
}

WStatus WObjectDirectAccessor::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount)
{
  out_iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
  return WStatus(W_SUCCESS);
}

WStatus WObjectDirectAccessor::AddObject(
  const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  W_SUCCEED_OR_RETURN(m_pManager->CanAdd(pType, pParent, pParentProp->GetPropertyName(), index));

  WDocumentObject* pPar = m_pManager->GetObject(pParent->GetGuid());
  W_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  if (!inout_objectGuid.IsValid())
    inout_objectGuid = WUuid::MakeUuid();
  WDocumentObject* pObj = m_pManager->CreateObject(pType, inout_objectGuid);
  m_pManager->AddObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return WStatus(W_SUCCESS);
}

WStatus WObjectDirectAccessor::RemoveObject(const WDocumentObject* pObject)
{
  W_SUCCEED_OR_RETURN(m_pManager->CanRemove(pObject));

  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  m_pManager->RemoveObject(pObj);
  return WStatus(W_SUCCESS);
}

WStatus WObjectDirectAccessor::MoveObject(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index)
{
  W_SUCCEED_OR_RETURN(m_pManager->CanMove(pObject, pNewParent, pParentProp->GetPropertyName(), index));

  WDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  W_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  WDocumentObject* pPar = m_pManager->GetObject(pNewParent->GetGuid());
  W_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  m_pManager->MoveObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return WStatus(W_SUCCESS);
}

WStatus WObjectDirectAccessor::GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys)
{
  bool bRes = pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), out_keys);
  return WStatus(bRes ? W_SUCCESS : W_FAILURE);
}

WStatus WObjectDirectAccessor::GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values)
{
  bool bRes = pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), out_values);
  return WStatus(bRes ? W_SUCCESS : W_FAILURE);
}
