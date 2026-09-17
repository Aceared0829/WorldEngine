#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <GuiFoundation/PropertyGrid/Implementation/ExpressionPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>

#include <ToolsFoundation/Document/Document.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>


WRttiMappedObjectFactory<WQtPropertyWidget> WQtPropertyGridWidget::s_Factory;

static WQtPropertyWidget* StandardTypeCreator(const WRTTI* pRtti)
{
  W_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(WTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  if (pRtti == WGetStaticRTTI<WVariant>())
  {
    return new WQtVariantPropertyWidget();
  }

  switch (pRtti->GetVariantType())
  {
    case WVariant::Type::Bool:
      return new WQtPropertyEditorCheckboxWidget();

    case WVariant::Type::Time:
      return new WQtPropertyEditorTimeWidget();

    case WVariant::Type::Float:
    case WVariant::Type::Double:
      return new WQtPropertyEditorDoubleSpinboxWidget(1);

    case WVariant::Type::Vector2:
      return new WQtPropertyEditorDoubleSpinboxWidget(2);

    case WVariant::Type::Vector3:
      return new WQtPropertyEditorDoubleSpinboxWidget(3);

    case WVariant::Type::Vector4:
      return new WQtPropertyEditorDoubleSpinboxWidget(4);

    case WVariant::Type::Vector2I:
      return new WQtPropertyEditorIntSpinboxWidget(2, -2147483645, 2147483645);

    case WVariant::Type::Vector3I:
      return new WQtPropertyEditorIntSpinboxWidget(3, -2147483645, 2147483645);

    case WVariant::Type::Vector4I:
      return new WQtPropertyEditorIntSpinboxWidget(4, -2147483645, 2147483645);

    case WVariant::Type::Vector2U:
      return new WQtPropertyEditorIntSpinboxWidget(2, 0, 2147483645);

    case WVariant::Type::Vector3U:
      return new WQtPropertyEditorIntSpinboxWidget(3, 0, 2147483645);

    case WVariant::Type::Vector4U:
      return new WQtPropertyEditorIntSpinboxWidget(4, 0, 2147483645);

    case WVariant::Type::Quaternion:
      return new WQtPropertyEditorQuaternionWidget();

    case WVariant::Type::Transform:
      return new WQtPropertyEditorTransformWidget();

    case WVariant::Type::Int8:
      return new WQtPropertyEditorIntSpinboxWidget(1, -127, 127);

    case WVariant::Type::UInt8:
      return new WQtPropertyEditorIntSpinboxWidget(1, 0, 255);

    case WVariant::Type::Int16:
      return new WQtPropertyEditorIntSpinboxWidget(1, -32767, 32767);

    case WVariant::Type::UInt16:
      return new WQtPropertyEditorIntSpinboxWidget(1, 0, 65535);

    case WVariant::Type::Int32:
    case WVariant::Type::Int64:
      return new WQtPropertyEditorIntSpinboxWidget(1, -2147483645, 2147483645);

    case WVariant::Type::UInt32:
    case WVariant::Type::UInt64:
      return new WQtPropertyEditorIntSpinboxWidget(1, 0, 2147483645);

    case WVariant::Type::String:
    case WVariant::Type::StringView:
      return new WQtPropertyEditorLineEditWidget();

    case WVariant::Type::Color:
    case WVariant::Type::ColorGamma:
      return new WQtPropertyEditorColorWidget();

    case WVariant::Type::Angle:
      return new WQtPropertyEditorAngleWidget();

    case WVariant::Type::HashedString:
      return new WQtPropertyEditorLineEditWidget();

    default:
      W_REPORT_FAILURE("No default property widget available for type: {0}", pRtti->GetTypeName());
      return nullptr;
  }
}

static WQtPropertyWidget* VariantArrayCreator(const WRTTI* pRtti)
{
  return new WQtPropertyStandardTypeContainerWidget();
}

static WQtPropertyWidget* EnumCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorEnumWidget();
}

static WQtPropertyWidget* BitflagsCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorBitflagsWidget();
}

static WQtPropertyWidget* TagSetCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorTagSetWidget();
}

static WQtPropertyWidget* VarianceTypeCreator(const WRTTI* pRtti)
{
  return new WQtVarianceTypeWidget();
}

static WQtPropertyWidget* Curve1DTypeCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorCurve1DWidget();
}

static WQtPropertyWidget* ColorGradientTypeCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorColorGradientWidget();
}

static WQtPropertyWidget* ExpressionTypeCreator(const WRTTI* pRtti)
{
  return new WQtPropertyEditorExpressionWidget();
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<bool>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<float>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<double>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec2>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec3>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec4>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec2I32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec3I32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec4I32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec2U32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec3U32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVec4U32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WQuat>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WTransform>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WInt8>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WUInt8>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WInt16>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WUInt16>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WInt32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WUInt32>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WInt64>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WUInt64>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WConstCharPtr>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WString>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WStringView>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WTime>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WColor>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WColorGammaUB>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WAngle>(), StandardTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WHashedString>(), StandardTypeCreator);
    
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVariant>(), StandardTypeCreator);
    //WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVariantArray>(), VariantArrayCreator);

    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WEnumBase>(), EnumCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WBitflagsBase>(), BitflagsCreator);

    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WTagSetWidgetAttribute>(), TagSetCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WVarianceTypeBase>(), VarianceTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WSingleCurveData>(), Curve1DTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WColorGradient>(), ColorGradientTypeCreator);
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WExpressionWidgetAttribute>(), ExpressionTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<bool>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<float>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<double>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec2>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec3>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec4>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec2I32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec3I32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec4I32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec2U32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec3U32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVec4U32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WQuat>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WTransform>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WInt8>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WUInt8>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WInt16>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WUInt16>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WInt32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WUInt32>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WInt64>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WUInt64>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WConstCharPtr>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WString>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WStringView>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WTime>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WColor>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WColorGammaUB>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WAngle>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WHashedString>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVariant>());
    //WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVariantArray>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WEnumBase>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WBitflagsBase>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WTagSetWidgetAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WVarianceTypeBase>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WSingleCurveData>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WColorGradient>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WExpressionWidgetAttribute>());
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WRttiMappedObjectFactory<WQtPropertyWidget>& WQtPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

WQtPropertyGridWidget::WQtPropertyGridWidget(QWidget* pParent, WDocument* pDocument, bool bBindToSelectionManager)
  : QWidget(pParent)
{
  setObjectName("WQtPropertyGridWidget");

  m_pDocument = nullptr;

  m_pScroll = new QScrollArea(this);
  m_pScroll->setObjectName("QScrollArea1");
  m_pScroll->setContentsMargins(0, 0, 0, 0);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setObjectName("QVBoxLayout1");
  m_pLayout->setSpacing(0);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pLayout->addWidget(m_pScroll);

  m_pContent = new QWidget(this);
  m_pContent->setObjectName("MainQWidget");
  m_pScroll->setWidget(m_pContent);
  m_pScroll->setWidgetResizable(true);

  m_pContentLayout = new QVBoxLayout(m_pContent);
  m_pContentLayout->setObjectName("QVBoxLayout2");
  m_pContentLayout->setSpacing(1);
  m_pContentLayout->setContentsMargins(0, 0, 0, 0);
  m_pContent->setLayout(m_pContentLayout);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pContentLayout->addSpacerItem(m_pSpacer);

  m_pTypeWidget = nullptr;

  s_Factory.m_Events.AddEventHandler(WMakeDelegate(&WQtPropertyGridWidget::FactoryEventHandler, this));
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WQtPropertyGridWidget::TypeEventHandler, this));

  SetDocument(pDocument, bBindToSelectionManager);
}

WQtPropertyGridWidget::~WQtPropertyGridWidget()
{
  s_Factory.m_Events.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::FactoryEventHandler, this));
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::TypeEventHandler, this));

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::SelectionEventHandler, this));
  }
}


void WQtPropertyGridWidget::SetDocument(WDocument* pDocument, bool bBindToSelectionManager)
{
  m_bBindToSelectionManager = bBindToSelectionManager;
  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WQtPropertyGridWidget::SelectionEventHandler, this));
  }

  m_pDocument = pDocument;

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.AddEventHandler(WMakeDelegate(&WQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtPropertyGridWidget::SelectionEventHandler, this));
  }
}

void WQtPropertyGridWidget::ClearSelection()
{
  if (m_pTypeWidget)
  {
    m_pContentLayout->removeWidget(m_pTypeWidget);
    m_pTypeWidget->hide();

    m_pTypeWidget->PrepareToDie();

    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  m_Selection.Clear();
}

void WQtPropertyGridWidget::SetSelectionIncludeExcludeProperties(const char* szIncludeProperties /*= nullptr*/, const char* szExcludeProperties /*= nullptr*/)
{
  m_sSelectionIncludeProperties = szIncludeProperties;
  m_sSelectionExcludeProperties = szExcludeProperties;
}

void WQtPropertyGridWidget::SetSelection(const WDeque<const WDocumentObject*>& selection)
{
  WQtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    WTempHybridArray<WPropertySelection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      WPropertySelection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    const WRTTI* pCommonType = WQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new WQtTypeWidget(m_pContent, this, GetObjectAccessor(), pCommonType, m_sSelectionIncludeProperties, m_sSelectionExcludeProperties);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const WDocument* WQtPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

const WDocumentObjectManager* WQtPropertyGridWidget::GetObjectManager() const
{
  return m_pDocument->GetObjectManager();
}

WCommandHistory* WQtPropertyGridWidget::GetCommandHistory() const
{
  return m_pDocument->GetCommandHistory();
}


WObjectAccessorBase* WQtPropertyGridWidget::GetObjectAccessor() const
{
  return m_pDocument->GetObjectAccessor();
}

WQtPropertyWidget* WQtPropertyGridWidget::CreateMemberPropertyWidget(const WAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing WTypeWidgetAttribute.
  const WTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<WTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    WQtPropertyWidget* pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  WQtPropertyWidget* pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
  if (pWidget != nullptr)
    return pWidget;

  return new WQtUnsupportedPropertyWidget("No property grid widget registered");
}

WQtPropertyWidget* WQtPropertyGridWidget::CreatePropertyWidget(const WAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing WTypeWidgetAttribute.
      const WTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<WTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        WQtPropertyWidget* pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          return new WQtPropertyPointerWidget();
        else
          return new WQtUnsupportedPropertyWidget("Pointer: Use WPropertyFlags::PointerOwner or provide derived WTypeWidgetAttribute");
      }
      else
      {
        WQtPropertyWidget* pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
        if (pWidget != nullptr)
          return pWidget;

        if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
        {
          // Member struct / class
          return new WQtPropertyTypeWidget(true);
        }
      }
    }
    break;
    case WPropertyCategory::Set:
    case WPropertyCategory::Array:
    case WPropertyCategory::Map:
    {
      // Try to create a registered container widget for an existing WContainerWidgetAttribute.
      const WContainerWidgetAttribute* pAttrib = pProp->GetAttributeByType<WContainerWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        WQtPropertyWidget* pWidget = WQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      // Fallback to default container widgets.
      const bool bIsValueType = WReflectionUtils::IsValueType(pProp);
      if (bIsValueType)
      {
        return new WQtPropertyStandardTypeContainerWidget();
      }
      else
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
        {
          return new WQtUnsupportedPropertyWidget("Pointer: Use WPropertyFlags::PointerOwner or provide derived WContainerWidgetAttribute");
        }

        return new WQtPropertyTypeContainerWidget();
      }
    }
    break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return new WQtUnsupportedPropertyWidget();
}

void WQtPropertyGridWidget::SetCollapseState(WQtGroupBoxBase* pBox)
{
  WUInt32 uiHash = GetGroupBoxHash(pBox);
  bool bCollapsed = false;
  auto it = m_CollapseState.Find(uiHash);
  if (it.IsValid())
    bCollapsed = it.Value();

  pBox->SetCollapseState(bCollapsed);
}

void WQtPropertyGridWidget::OnCollapseStateChanged(bool bCollapsed)
{
  WQtGroupBoxBase* pBox = qobject_cast<WQtGroupBoxBase*>(sender());
  WUInt32 uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash] = pBox->GetCollapseState();
}

void WQtPropertyGridWidget::ObjectAccessorChangeEventHandler(const WObjectAccessorChangeEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void WQtPropertyGridWidget::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  // TODO: even when not binding to the selection manager we need to test whether our selection is still valid.
  if (!m_bBindToSelectionManager)
    return;

  switch (e.m_Type)
  {
    case WSelectionManagerEvent::Type::SelectionCleared:
    {
      ClearSelection();
    }
    break;
    case WSelectionManagerEvent::Type::SelectionSet:
    case WSelectionManagerEvent::Type::ObjectAdded:
    case WSelectionManagerEvent::Type::ObjectRemoved:
    {
      SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    }
    break;

    case WSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void WQtPropertyGridWidget::FactoryEventHandler(const WRttiMappedObjectFactory<WQtPropertyWidget>::Event& e)
{
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    WDeque<const WDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

void WQtPropertyGridWidget::TypeEventHandler(const WPhantomRttiManagerEvent& e)
{
  // Adding types cannot affect the property grid content.
  if (e.m_Type == WPhantomRttiManagerEvent::Type::TypeAdded)
    return;

  W_PROFILE_SCOPE("TypeEventHandler");
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    WDeque<const WDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

WUInt32 WQtPropertyGridWidget::GetGroupBoxHash(WQtGroupBoxBase* pBox) const
{
  WUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    WQtGroupBoxBase* pCurBox = qobject_cast<WQtGroupBoxBase*>(pCur);
    if (pCurBox != nullptr)
    {
      const QByteArray name = pCurBox->GetTitle().toUtf8().data();
      uiHash += WHashingUtils::xxHash32(name, name.length());
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
