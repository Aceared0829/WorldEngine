#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WManipulatorAdapter::WManipulatorAdapter()
{
  WQtDocumentWindow::s_Events.AddEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentWindowEventHandler, this));
}

WManipulatorAdapter::~WManipulatorAdapter()
{
  WQtDocumentWindow::s_Events.RemoveEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void WManipulatorAdapter::SetManipulator(const WManipulatorAttribute* pAttribute, const WDocumentObject* pObject)
{
  m_pManipulatorAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(WMakeDelegate(&WManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bManipulatorIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}

void WManipulatorAdapter::DocumentObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == m_pObject)
  {
    if (e.m_sProperty == m_pManipulatorAttr->m_sProperty1 || e.m_sProperty == m_pManipulatorAttr->m_sProperty2 || e.m_sProperty == m_pManipulatorAttr->m_sProperty3 || e.m_sProperty == m_pManipulatorAttr->m_sProperty4 || e.m_sProperty == m_pManipulatorAttr->m_sProperty5 ||
        e.m_sProperty == m_pManipulatorAttr->m_sProperty6)
    {
      Update();
    }
  }
}

void WManipulatorAdapter::DocumentWindowEventHandler(const WQtDocumentWindowEvent& e)
{
  if (e.m_Type == WQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void WManipulatorAdapter::DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & WDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bManipulatorIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

WTransform WManipulatorAdapter::GetOffsetTransform() const
{
  return WTransform::MakeIdentity();
}

WTransform WManipulatorAdapter::GetObjectTransform() const
{
  WTransform tObj;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, tObj).IgnoreResult();

  const WTransform offset = GetOffsetTransform();

  WTransform tGlobal = WTransform::MakeGlobalTransform(tObj, offset);

  return tGlobal;
}

WObjectAccessorBase* WManipulatorAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const WAbstractProperty* WManipulatorAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}

void WManipulatorAdapter::BeginTemporaryInteraction()
{
  GetObjectAccessor()->BeginTemporaryCommands("Adjust Object");
}

void WManipulatorAdapter::EndTemporaryInteraction()
{
  GetObjectAccessor()->FinishTemporaryCommands();
}

void WManipulatorAdapter::CancelTemporayInteraction()
{
  GetObjectAccessor()->CancelTemporaryCommands();
}

void WManipulatorAdapter::ClampProperty(const char* szProperty, WVariant& value) const
{
  WResult status(W_FAILURE);
  const double fCur = value.ConvertTo<double>(&status);

  if (status.Failed())
    return;

  const WClampValueAttribute* pClamp = GetProperty(szProperty)->GetAttributeByType<WClampValueAttribute>();
  if (pClamp == nullptr)
    return;

  if (pClamp->GetMinValue().IsValid())
  {
    const double fMin = pClamp->GetMinValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur < fMin)
        value = pClamp->GetMinValue();
    }
  }

  if (pClamp->GetMaxValue().IsValid())
  {
    const double fMax = pClamp->GetMaxValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur > fMax)
        value = pClamp->GetMaxValue();
    }
  }
}

void WManipulatorAdapter::ChangeProperties(const char* szProperty1, WVariant value1, const char* szProperty2 /*= nullptr*/, WVariant value2 /*= WVariant()*/, const char* szProperty3 /*= nullptr*/, WVariant value3 /*= WVariant()*/, const char* szProperty4 /*= nullptr*/,
  WVariant value4 /*= WVariant()*/, const char* szProperty5 /*= nullptr*/, WVariant value5 /*= WVariant()*/, const char* szProperty6 /*= nullptr*/, WVariant value6 /*= WVariant()*/)
{
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  pObjectAccessor->StartTransaction("Change Properties");

  if (!WStringUtils::IsNullOrEmpty(szProperty1))
  {
    ClampProperty(szProperty1, value1);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty1), value1).AssertSuccess();
  }

  if (!WStringUtils::IsNullOrEmpty(szProperty2))
  {
    ClampProperty(szProperty2, value2);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty2), value2).AssertSuccess();
  }

  if (!WStringUtils::IsNullOrEmpty(szProperty3))
  {
    ClampProperty(szProperty3, value3);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty3), value3).AssertSuccess();
  }

  if (!WStringUtils::IsNullOrEmpty(szProperty4))
  {
    ClampProperty(szProperty4, value4);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty4), value4).AssertSuccess();
  }

  if (!WStringUtils::IsNullOrEmpty(szProperty5))
  {
    ClampProperty(szProperty5, value5);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty5), value5).AssertSuccess();
  }

  if (!WStringUtils::IsNullOrEmpty(szProperty6))
  {
    ClampProperty(szProperty6, value6);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty6), value6).AssertSuccess();
  }

  pObjectAccessor->FinishTransaction();
}
