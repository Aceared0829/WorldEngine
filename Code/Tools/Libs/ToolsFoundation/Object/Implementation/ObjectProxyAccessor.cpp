#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectProxyAccessor, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WObjectProxyAccessor::WObjectProxyAccessor(WObjectAccessorBase* pSource)
  : WObjectAccessorBase(pSource->GetObjectManager())
  , m_pSource(pSource)
{
}

WObjectProxyAccessor::~WObjectProxyAccessor() = default;

void WObjectProxyAccessor::StartTransaction(WStringView sDisplayString)
{
  m_pSource->StartTransaction(sDisplayString);
}

void WObjectProxyAccessor::CancelTransaction()
{
  m_pSource->CancelTransaction();
}

void WObjectProxyAccessor::FinishTransaction()
{
  m_pSource->FinishTransaction();
}

void WObjectProxyAccessor::BeginTemporaryCommands(WStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pSource->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void WObjectProxyAccessor::CancelTemporaryCommands()
{
  m_pSource->CancelTemporaryCommands();
}

void WObjectProxyAccessor::FinishTemporaryCommands()
{
  m_pSource->FinishTemporaryCommands();
}

const WDocumentObject* WObjectProxyAccessor::GetObject(const WUuid& object)
{
  return m_pSource->GetObject(object);
}

WStatus WObjectProxyAccessor::GetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index /*= WVariant()*/)
{
  return m_pSource->GetValue(pObject, pProp, out_value, index);
}

WStatus WObjectProxyAccessor::SetValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  return m_pSource->SetValue(pObject, pProp, newValue, index);
}

WStatus WObjectProxyAccessor::InsertValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index /*= WVariant()*/)
{
  return m_pSource->InsertValue(pObject, pProp, newValue, index);
}

WStatus WObjectProxyAccessor::RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index /*= WVariant()*/)
{
  return m_pSource->RemoveValue(pObject, pProp, index);
}

WStatus WObjectProxyAccessor::MoveValue(
  const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex)
{
  return m_pSource->MoveValue(pObject, pProp, oldIndex, newIndex);
}

WStatus WObjectProxyAccessor::GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount)
{
  return m_pSource->GetCount(pObject, pProp, out_iCount);
}

WStatus WObjectProxyAccessor::AddObject(
  const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType, WUuid& inout_objectGuid)
{
  return m_pSource->AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
}

WStatus WObjectProxyAccessor::RemoveObject(const WDocumentObject* pObject)
{
  return m_pSource->RemoveObject(pObject);
}

WStatus WObjectProxyAccessor::MoveObject(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index)
{
  return m_pSource->MoveObject(pObject, pNewParent, pParentProp, index);
}

WStatus WObjectProxyAccessor::GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys)
{
  return m_pSource->GetKeys(pObject, pProp, out_keys);
}

WStatus WObjectProxyAccessor::GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values)
{
  return m_pSource->GetValues(pObject, pProp, out_values);
}
