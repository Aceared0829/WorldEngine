#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <QSlider>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtVarianceTypeWidget::WQtVarianceTypeWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pValueWidget = new WQtDoubleSpinBox(this);
  m_pValueWidget->installEventFilter(m_pValueWidget);
  m_pValueWidget->setMinimum(-WMath::Infinity<double>());
  m_pValueWidget->setMaximum(WMath::Infinity<double>());
  m_pValueWidget->setSingleStep(0.1f);
  m_pValueWidget->setAccelerated(true);
  m_pValueWidget->setDecimals(3);
  m_pValueWidget->setMinimumWidth(60);

  m_pVarianceWidget = new QSlider(this);
  m_pVarianceWidget->setOrientation(Qt::Orientation::Horizontal);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);

  QLabel* pText = new QLabel("Variance:");
  pText->setToolTip("Random deviation of base value:\nSlider to the left -> 0 variance, no randomness at all.\nSlider in the middle -> 0.5 variance, value will be in range [0.5 * base ... 1.5 * base]\nSlider to the right -> full variance, value will be in range [0 ... 2 * base]\n\nNote that values deviate from base using a Bell curve, meaning that values close to 'base' are more likely.");

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(pText);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pVarianceWidget, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotVarianceChanged()));
}

void WQtVarianceTypeWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtStandardPropertyWidget::SetSelection(items);
  W_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<WVarianceTypeBase>(), "Selection does not match WVarianceType.");
}

void WQtVarianceTypeWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void WQtVarianceTypeWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtVarianceTypeWidget::SlotValueChanged()
{
  onBeginTemporary();

  WVariant value;
  WToolsReflectionUtils::GetVariantFromFloat(m_pValueWidget->value(), m_pValueProp->GetSpecificType()->GetVariantType(), value);

  auto obj = m_OldValue.Get<WTypedObject>();
  void* pCopy = WReflectionSerializer::Clone(obj.m_pObject, obj.m_pType);
  WReflectionUtils::SetMemberPropertyValue(m_pValueProp, pCopy, value);
  WVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}


void WQtVarianceTypeWidget::SlotVarianceChanged()
{
  double variance = WMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);

  WVariant newValue = m_OldValue;
  WTypedPointer ptr = newValue.GetWriteAccess();
  WReflectionUtils::SetMemberPropertyValue(m_pVarianceProp, ptr.m_pObject, variance);

  BroadcastValueChanged(newValue);
}

void WQtVarianceTypeWidget::OnInit()
{
  m_pValueProp = static_cast<const WAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Value"));
  m_pVarianceProp = static_cast<const WAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Variance"));

  // Property type adjustments
  WQtScopedBlockSignals bs(m_pValueWidget);
  const WRTTI* pValueType = m_pValueProp->GetSpecificType();
  if (pValueType == WGetStaticRTTI<WTime>())
  {
    m_pValueWidget->setDisplaySuffix(" sec");
  }
  else if (pValueType == WGetStaticRTTI<WAngle>())
  {
    m_pValueWidget->setDisplaySuffix(WStringUtf8(L"\u00B0").GetData());
  }

  // Handle attributes
  if (const WSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<WSuffixAttribute>())
  {
    m_pValueWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }
  if (const WClampValueAttribute* pClamp = m_pProp->GetAttributeByType<WClampValueAttribute>())
  {
    if (pClamp->GetMinValue().CanConvertTo<double>() || pClamp->GetMinValue().IsA<WTime>() || pClamp->GetMinValue().IsA<WAngle>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue());
    }
    else if (const WRTTI* pType = pClamp->GetMinValue().GetReflectedType(); pType && pType->IsDerivedFrom<WVarianceTypeBase>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue()["Value"]);
      m_pVarianceWidget->setMinimum(static_cast<WInt32>(pClamp->GetMinValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    if (pClamp->GetMaxValue().CanConvertTo<double>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue());
    }
    else if (const WRTTI* pType = pClamp->GetMaxValue().GetReflectedType(); pType && pType->IsDerivedFrom<WVarianceTypeBase>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue()["Value"]);
      m_pVarianceWidget->setMaximum(static_cast<WInt32>(pClamp->GetMaxValue()["Variance"].ConvertTo<double>() * 100.0));
    }
  }
  if (const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>())
  {
    if (pDefault->GetValue().CanConvertTo<double>() || pDefault->GetValue().IsA<WTime>() || pDefault->GetValue().IsA<WAngle>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue());
    }
    else if (const WRTTI* pType = pDefault->GetValue().GetReflectedType(); pType && pType->IsDerivedFrom<WVarianceTypeBase>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue()["Value"]);
    }
  }
}

void WQtVarianceTypeWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals bs(m_pValueWidget, m_pVarianceWidget);
  if (value.IsValid())
  {
    m_pValueWidget->setValue(value["Value"]);
    m_pVarianceWidget->setValue(value["Variance"].ConvertTo<double>() * 100.0);
  }
  else
  {
    m_pValueWidget->setValueInvalid();
    m_pVarianceWidget->setValue(50);
  }
}
