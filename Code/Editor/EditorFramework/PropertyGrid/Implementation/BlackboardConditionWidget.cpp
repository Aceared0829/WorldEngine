#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/BlackboardConditionWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumMenuButton.moc.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

WQtBlackboardConditionWidget::WQtBlackboardConditionWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pEntryButton = new WQtDynamicStringEnumMenuButton(this);
  m_pEntryButton->installEventFilter(this);
  m_pLayout->addWidget(m_pEntryButton, 2);

  m_pOperator = new QComboBox(this);
  m_pOperator->installEventFilter(this);
  m_pLayout->addWidget(m_pOperator, 1);

  m_pComparisonValue = new WQtDoubleSpinBox(this);
  m_pComparisonValue->setMinimum(-WMath::Infinity<double>());
  m_pComparisonValue->setMaximum(WMath::Infinity<double>());
  m_pComparisonValue->setSingleStep(0.1);
  m_pComparisonValue->setAccelerated(true);
  m_pComparisonValue->installEventFilter(m_pComparisonValue);
  m_pLayout->addWidget(m_pComparisonValue, 1);
}

WQtBlackboardConditionWidget::~WQtBlackboardConditionWidget() = default;

void WQtBlackboardConditionWidget::OnInit()
{
  // Populate the operator drop down from the reflected enum.
  {
    WQtScopedBlockSignals bs(m_pOperator);

    WHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumValues;
    WReflectionUtils::GetEnumKeysAndValues(WGetStaticRTTI<WComparisonOperator>(), enumValues);
    for (auto& val : enumValues)
    {
      m_pOperator->addItem(WMakeQString(WTranslate(val.m_sKey)), val.m_iValue);
    }

    connect(m_pOperator, SIGNAL(currentIndexChanged(int)), this, SLOT(onOperatorChanged(int)));
  }

  // Retrieve the dynamic string enum used for the entry name from the reflected property.
  {
    if (const WAbstractProperty* pEntryProp = WGetStaticRTTI<WBlackboardCondition>()->FindPropertyByName("EntryName"))
    {
      if (const WDynamicStringEnumAttribute* pAttr = pEntryProp->GetAttributeByType<WDynamicStringEnumAttribute>())
      {
        m_pEntryButton->SetEnum(pAttr->GetDynamicEnumName());
        m_pEntryButton->SetDocument(m_pGrid->GetDocument());
      }
    }

    connect(m_pEntryButton, &WQtDynamicStringEnumMenuButton::ValueSelected, this,
      [this](const QString& sValue)
      { SetEntryName(sValue.toUtf8().data()); });
  }

  connect(m_pComparisonValue, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pComparisonValue, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged()));
}

void WQtBlackboardConditionWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals bs(m_pOperator, m_pComparisonValue);

  if (value.IsValid())
  {
    m_CurrentValue = value.Get<WBlackboardCondition>();

    m_pEntryButton->SetCurrentValue(m_CurrentValue.m_sEntryName.GetView());

    for (int i = 0; i < m_pOperator->count(); ++i)
    {
      if (m_pOperator->itemData(i).toInt() == m_CurrentValue.m_Operator.GetValue())
      {
        m_pOperator->setCurrentIndex(i);
        break;
      }
    }

    m_pComparisonValue->setValue(m_CurrentValue.m_fComparisonValue);
  }
  else
  {
    m_CurrentValue = WBlackboardCondition();
    m_pEntryButton->setText(QStringLiteral("<Multiple Values>"));
    m_pOperator->setCurrentIndex(-1);
    m_pComparisonValue->setValueInvalid();
  }
}

void WQtBlackboardConditionWidget::BroadcastCurrentValue()
{
  WVariant v;
  v.CopyTypedObject(&m_CurrentValue, WGetStaticRTTI<WBlackboardCondition>());
  BroadcastValueChanged(v);
}

void WQtBlackboardConditionWidget::SetEntryName(WStringView sName)
{
  m_CurrentValue.m_sEntryName.Assign(sName);
  m_pEntryButton->SetCurrentValue(sName);

  BroadcastCurrentValue();
}

void WQtBlackboardConditionWidget::onOperatorChanged(int iIndex)
{
  if (iIndex < 0)
    return;

  m_CurrentValue.m_Operator.SetValue(static_cast<WComparisonOperator::StorageType>(m_pOperator->currentData().toInt()));

  BroadcastCurrentValue();
}

void WQtBlackboardConditionWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void WQtBlackboardConditionWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtBlackboardConditionWidget::onValueChanged()
{
  onBeginTemporary();

  m_CurrentValue.m_fComparisonValue = m_pComparisonValue->value();

  BroadcastCurrentValue();
}
