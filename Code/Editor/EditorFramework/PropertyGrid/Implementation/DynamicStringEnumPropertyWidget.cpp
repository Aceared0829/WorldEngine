#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicStringEnumMenuButton.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

WQtDynamicStringEnumPropertyWidget::WQtDynamicStringEnumPropertyWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new WQtDynamicStringEnumMenuButton(this);

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  connect(m_pButton, &WQtDynamicStringEnumMenuButton::ValueSelected, this,
    [this](const QString& sValue)
    { SetNewValue(sValue.toUtf8().data()); });

  m_pLayout->addWidget(m_pButton);
}

void WQtDynamicStringEnumPropertyWidget::OnInit()
{
  W_ASSERT_DEV(m_pProp->GetAttributeByType<WDynamicStringEnumAttribute>() != nullptr,
    "WQtDynamicStringEnumPropertyWidget was created without a WDynamicStringEnumAttribute!");
  WVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  W_IGNORE_UNUSED(type);
  W_ASSERT_DEV(type == WVariantType::String || type == WVariantType::HashedString || type == WVariantType::StringView, "WDynamicStringEnumAttribute can only be used with string types");

  const WDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<WDynamicStringEnumAttribute>();

  m_pButton->SetEnum(pAttr->GetDynamicEnumName());
  m_pButton->SetDocument(m_pGrid->GetDocument());

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<WDefaultValueAttribute>())
  {
    m_pButton->GetEnum()->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<WString>(), true);
  }
}

void WQtDynamicStringEnumPropertyWidget::InternalSetValue(const WVariant& value)
{
  m_pButton->SetCurrentValue(value.ConvertTo<WString>());
}

void WQtDynamicStringEnumPropertyWidget::SetNewValue(WStringView sNewValue)
{
  WVariant v;
  WVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  if (type == WVariantType::String || type == WVariantType::StringView)
  {
    v = WVariant(sNewValue);
  }
  else if (type == WVariantType::HashedString)
  {
    WHashedString s;
    s.Assign(sNewValue);
    v = s;
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  InternalSetValue(v);
  BroadcastValueChanged(v);
}
