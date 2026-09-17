#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <GuiFoundation/Dialogs/ColorGradientEditDlg.moc.h>
#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <qcheckbox.h>
#include <qlayout.h>


/// *** CHECKBOX ***

WQtPropertyEditorCheckboxWidget::WQtPropertyEditorCheckboxWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  W_VERIFY(connect(m_pWidget, &QCheckBox::checkStateChanged, this, &WQtPropertyEditorCheckboxWidget::on_StateChanged_triggered) != nullptr, "signal/slot connection failed");
#else
  W_VERIFY(connect(m_pWidget, &QCheckBox::stateChanged, this, &WQtPropertyEditorCheckboxWidget::on_StateChanged_triggered) != nullptr, "signal/slot connection failed");
#endif
}

void WQtPropertyEditorCheckboxWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setTristate(false);
    m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
  }
  else
  {
    m_pWidget->setTristate(true);
    m_pWidget->setCheckState(Qt::CheckState::PartiallyChecked);
  }
}

void WQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* pEv)
{
  QWidget::mousePressEvent(pEv);

  m_pWidget->toggle();
}

void WQtPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    WQtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}


/// *** DOUBLE SPINBOX ***

WQtPropertyEditorDoubleSpinboxWidget::WQtPropertyEditorDoubleSpinboxWidget(WInt8 iNumComponents)
  : WQtStandardPropertyWidget()
{
  W_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  const char* szLabels[] = {"X",
    "Y",
    "Z",
    "W"};

  const WColorGammaUB labelColors[] = {WColorScheme::LightUI(WColorScheme::Red),
    WColorScheme::LightUI(WColorScheme::Green),
    WColorScheme::LightUI(WColorScheme::Blue),
    WColorScheme::LightUI(WColorScheme::Gray)};

  for (WInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new WQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-WMath::Infinity<double>());
    m_pWidget[c]->setMaximum(WMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    if (m_iNumComponents > 1)
    {
      QLabel* pLabel = new QLabel(szLabels[c]);
      QPalette palette = pLabel->palette();
      palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
      pLabel->setPalette(palette);
      m_pLayout->addWidget(pLabel);
    }

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void WQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<WNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const WClampValueAttribute* pClamp = m_pProp->GetAttributeByType<WClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        WQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());
        break;
      }
      case 2:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<WVec2>())
        {
          WVec2 value = pClamp->GetMinValue().ConvertTo<WVec2>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec2>())
        {
          WVec2 value = pClamp->GetMaxValue().ConvertTo<WVec2>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<WVec3>())
        {
          WVec3 value = pClamp->GetMinValue().ConvertTo<WVec3>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec3>())
        {
          WVec3 value = pClamp->GetMaxValue().ConvertTo<WVec3>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<WVec4>())
        {
          WVec4 value = pClamp->GetMinValue().ConvertTo<WVec4>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec4>())
        {
          WVec4 value = pClamp->GetMaxValue().ConvertTo<WVec4>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        WQtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
      case 2:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<WVec2>())
        {
          WVec2 value = pDefault->GetValue().ConvertTo<WVec2>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<WVec3>())
        {
          WVec3 value = pDefault->GetValue().ConvertTo<WVec3>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<WVec4>())
        {
          WVec4 value = pDefault->GetValue().ConvertTo<WVec4>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const WSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<WSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const WMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<WMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void WQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();
  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = WVariantType::Double;
  }

  if (value.IsValid())
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValue(value.ConvertTo<double>());
        break;
      case 2:
        m_pWidget[0]->setValue(value.ConvertTo<WVec2>().x);
        m_pWidget[1]->setValue(value.ConvertTo<WVec2>().y);
        break;
      case 3:
        m_pWidget[0]->setValue(value.ConvertTo<WVec3>().x);
        m_pWidget[1]->setValue(value.ConvertTo<WVec3>().y);
        m_pWidget[2]->setValue(value.ConvertTo<WVec3>().z);
        break;
      case 4:
        m_pWidget[0]->setValue(value.ConvertTo<WVec4>().x);
        m_pWidget[1]->setValue(value.ConvertTo<WVec4>().y);
        m_pWidget[2]->setValue(value.ConvertTo<WVec4>().z);
        m_pWidget[3]->setValue(value.ConvertTo<WVec4>().w);
        break;
    }
  }
  else
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValueInvalid();
        break;
      case 2:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        break;
      case 3:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        break;
      case 4:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        m_pWidget[3]->setValueInvalid();
        break;
    }
  }
}

void WQtPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(WVariant(m_pWidget[0]->value()).ConvertTo(m_OriginalType));
      break;
    case 2:
      BroadcastValueChanged(WVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
      break;
    case 3:
      BroadcastValueChanged(WVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
      break;
    case 4:
      BroadcastValueChanged(WVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
      break;
  }
}


/// *** TIME SPINBOX ***

WQtPropertyEditorTimeWidget::WQtPropertyEditorTimeWidget()
  : WQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new WQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(" sec");
    m_pWidget->setMinimum(-WMath::Infinity<double>());
    m_pWidget->setMaximum(WMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void WQtPropertyEditorTimeWidget::OnInit()
{
  const WClampValueAttribute* pClamp = m_pProp->GetAttributeByType<WClampValueAttribute>();
  if (pClamp)
  {
    WQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>();
  if (pDefault)
  {
    WQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }
}

void WQtPropertyEditorTimeWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void WQtPropertyEditorTimeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorTimeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(WTime::MakeFromSeconds(m_pWidget->value()));
}


/// *** ANGLE SPINBOX ***

WQtPropertyEditorAngleWidget::WQtPropertyEditorAngleWidget()
  : WQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new WQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(WStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-WMath::Infinity<double>());
    m_pWidget->setMaximum(WMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(1);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void WQtPropertyEditorAngleWidget::OnInit()
{
  const WClampValueAttribute* pClamp = m_pProp->GetAttributeByType<WClampValueAttribute>();
  if (pClamp)
  {
    WQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>();
  if (pDefault)
  {
    WQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }

  const WSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<WSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const WMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<WMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(pMinValueText->GetText());
  }
}

void WQtPropertyEditorAngleWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void WQtPropertyEditorAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(WAngle::MakeFromDegree(m_pWidget->value()));
}

/// *** INT SPINBOX ***

WQtPropertyEditorIntSpinboxWidget::WQtPropertyEditorIntSpinboxWidget(WInt8 iNumComponents, WInt32 iMinValue, WInt32 iMaxValue)
  : WQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (WInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new WQtDoubleSpinBox(this, true);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(iMinValue);
    m_pWidget[c]->setMaximum(iMaxValue);
    m_pWidget[c]->setSingleStep(1);
    m_pWidget[c]->setAccelerated(true);

    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

WQtPropertyEditorIntSpinboxWidget::~WQtPropertyEditorIntSpinboxWidget() = default;

void WQtPropertyEditorIntSpinboxWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  for (WUInt32 i = 0; i < 4; ++i)
  {
    if (m_pWidget[i])
      m_pWidget[i]->setReadOnly(bReadOnly);
  }

  if (m_pSlider)
  {
    m_pSlider->setDisabled(bReadOnly);
  }
}

void WQtPropertyEditorIntSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<WNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction = (pNoTemporaryTransactions == nullptr);

  if (const WClampValueAttribute* pClamp = m_pProp->GetAttributeByType<WClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        const WInt32 iMinValue = pClamp->GetMinValue().ConvertTo<WInt32>();
        const WInt32 iMaxValue = pClamp->GetMaxValue().ConvertTo<WInt32>();

        WQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (iMaxValue - iMinValue) < 256 && m_bUseTemporaryTransaction)
        {
          WQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the WClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->installEventFilter(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(iMinValue);
          m_pSlider->setMaximum(iMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space

          connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
        }

        break;
      }
      case 2:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<WVec2I32>())
        {
          WVec2I32 value = pClamp->GetMinValue().ConvertTo<WVec2I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec2I32>())
        {
          WVec2I32 value = pClamp->GetMaxValue().ConvertTo<WVec2I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<WVec3I32>())
        {
          WVec3I32 value = pClamp->GetMinValue().ConvertTo<WVec3I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec3I32>())
        {
          WVec3I32 value = pClamp->GetMaxValue().ConvertTo<WVec3I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<WVec4I32>())
        {
          WVec4I32 value = pClamp->GetMinValue().ConvertTo<WVec4I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<WVec4I32>())
        {
          WVec4I32 value = pClamp->GetMaxValue().ConvertTo<WVec4I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pSlider);

        if (pDefault->GetValue().CanConvertTo<WInt32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<WInt32>());

          if (m_pSlider)
          {
            m_pSlider->setValue(pDefault->GetValue().ConvertTo<WInt32>());
          }
        }
        break;
      }
      case 2:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<WVec2I32>())
        {
          WVec2I32 value = pDefault->GetValue().ConvertTo<WVec2I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<WVec3I32>())
        {
          WVec3I32 value = pDefault->GetValue().ConvertTo<WVec3I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<WVec4I32>())
        {
          WVec4I32 value = pDefault->GetValue().ConvertTo<WVec4I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const WSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<WSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const WMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<WMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void WQtPropertyEditorIntSpinboxWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

  auto prop = GetProperty();
  const WRTTI* type = prop->GetSpecificType();
  m_OriginalType = type->GetVariantType();
  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = WVariantType::Int32;
  }

  switch (m_iNumComponents)
  {
    case 1:
      m_pWidget[0]->setValue(value.ConvertTo<WInt32>());

      if (m_pSlider)
      {
        m_pSlider->setValue(value.ConvertTo<WInt32>());
      }

      break;
    case 2:
      m_pWidget[0]->setValue(value.ConvertTo<WVec2I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<WVec2I32>().y);
      break;
    case 3:
      m_pWidget[0]->setValue(value.ConvertTo<WVec3I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<WVec3I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<WVec3I32>().z);
      break;
    case 4:
      m_pWidget[0]->setValue(value.ConvertTo<WVec4I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<WVec4I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<WVec4I32>().z);
      m_pWidget[3]->setValue(value.ConvertTo<WVec4I32>().w);
      break;
  }
}

void WQtPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  WVariant newValue;
  switch (m_iNumComponents)
  {
    case 1:
      newValue = m_pWidget[0]->value();

      if (m_pSlider)
      {
        WQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((WInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      newValue = WVec2I32(m_pWidget[0]->value(), m_pWidget[1]->value());
      break;
    case 3:
      newValue = WVec3I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value());
      break;
    case 4:
      newValue = WVec4I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value());
      break;
  }

  BroadcastValueChanged(newValue.ConvertTo(m_OriginalType));
}

void WQtPropertyEditorIntSpinboxWidget::onBeginTemporary()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void WQtPropertyEditorIntSpinboxWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  {
    WQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(WVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void WQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WMap<WString, WQtImageSliderWidget::ImageGeneratorFunc> WQtImageSliderWidget::s_ImageGenerators;

WQtImageSliderWidget::WQtImageSliderWidget(ImageGeneratorFunc generator, double fMinValue, double fMaxValue, QWidget* pParent)
  : QWidget(pParent)
{
  m_Generator = generator;
  m_fMinValue = fMinValue;
  m_fMaxValue = fMaxValue;

  setAutoFillBackground(false);
}

void WQtImageSliderWidget::SetValue(double fValue)
{
  if (m_fValue == fValue)
    return;

  m_fValue = fValue;
  update();
}

void WQtImageSliderWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);

  const QRect area = rect();

  if (area.width() != m_Image.width())
    UpdateImage();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  const float factor = WMath::Unlerp(m_fMinValue, m_fMaxValue, m_fValue);

  const double pos = (int)(factor * area.width()) + 0.5f;

  const double top = area.top() + 0.5;
  const double bot = area.bottom() + 0.5;
  const double len = 5.0;
  const double wid = 2.0;

  const QColor col = qRgb(80, 80, 80);

  painter.setPen(col);
  painter.setBrush(col);

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, top));
    path.lineTo(QPointF(pos, top + len));
    path.lineTo(QPointF(pos + wid, top));
    path.closeSubpath();

    painter.drawPath(path);
  }

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, bot));
    path.lineTo(QPointF(pos, bot - len));
    path.lineTo(QPointF(pos + wid, bot));
    path.closeSubpath();

    painter.drawPath(path);
  }
}

void WQtImageSliderWidget::UpdateImage()
{
  const int width = rect().width();

  if (m_Generator)
  {
    m_Image = m_Generator(rect().width(), rect().height(), m_fMinValue, m_fMaxValue);
  }
  else
  {
    m_Image = QImage(width, 1, QImage::Format::Format_RGB32);

    WColorGammaUB cg = WColor::HotPink;
    for (int x = 0; x < width; ++x)
    {
      m_Image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
    }
  }
}

void WQtImageSliderWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int x = WMath::Clamp(coord.x(), 0, width - 1);

    const double fx = (double)x / (width - 1);
    const double val = WMath::Lerp(m_fMinValue, m_fMaxValue, fx);

    valueChanged(val);
  }

  event->accept();
}

void WQtImageSliderWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderPressed();
  }

  mouseMoveEvent(event);

  event->accept();
}

void WQtImageSliderWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderReleased();
  }
  event->accept();
}

/// *** SLIDER ***

WQtPropertyEditorSliderWidget::WQtPropertyEditorSliderWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
}

WQtPropertyEditorSliderWidget::~WQtPropertyEditorSliderWidget() = default;

void WQtPropertyEditorSliderWidget::OnInit()
{
  const WImageSliderUiAttribute* pSliderAttr = m_pProp->GetAttributeByType<WImageSliderUiAttribute>();
  const WClampValueAttribute* pRange = m_pProp->GetAttributeByType<WClampValueAttribute>();
  W_ASSERT_DEV(pRange != nullptr, "WImageSliderUiAttribute always has to be compined with WClampValueAttribute to specify the valid range.");
  W_ASSERT_DEV(pRange->GetMinValue().IsValid() && pRange->GetMaxValue().IsValid(), "The min and max values used with WImageSliderUiAttribute both have to be valid.");

  m_fMinValue = pRange->GetMinValue().ConvertTo<double>();
  m_fMaxValue = pRange->GetMaxValue().ConvertTo<double>();

  m_pSlider = new WQtImageSliderWidget(WQtImageSliderWidget::s_ImageGenerators[pSliderAttr->m_sImageGenerator], m_fMinValue, m_fMaxValue, this);

  m_pLayout->insertWidget(0, m_pSlider);
  connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pSlider, SIGNAL(valueChanged(double)), this, SLOT(SlotSliderValueChanged(double)));

  if (const WDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<WDefaultValueAttribute>())
  {
    WQtScopedBlockSignals bs(m_pSlider);

    if (pDefault->GetValue().CanConvertTo<double>())
    {
      m_pSlider->SetValue(pDefault->GetValue().ConvertTo<double>());
    }
  }
}

void WQtPropertyEditorSliderWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals bs(m_pSlider);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = WVariantType::Double;
  }

  m_pSlider->SetValue(value.ConvertTo<double>());
}

void WQtPropertyEditorSliderWidget::SlotSliderValueChanged(double fValue)
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  BroadcastValueChanged(WVariant(fValue).ConvertTo(m_OriginalType));

  m_pSlider->SetValue(fValue);
}

void WQtPropertyEditorSliderWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

void WQtPropertyEditorSliderWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void WQtPropertyEditorSliderWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

/// *** QUATERNION ***

WQtPropertyEditorQuaternionWidget::WQtPropertyEditorQuaternionWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  const char* szLabels[] = {"R",
    "P",
    "Y"};
  const char* szTooltip[] = {"Roll (Rotation around the forward axis)",
    "Pitch (Rotation around the side axis)",
    "Yaw (Rotation around the up axis)"};

  const WColorGammaUB labelColors[] = {WColorScheme::LightUI(WColorScheme::Red),
    WColorScheme::LightUI(WColorScheme::Green),
    WColorScheme::LightUI(WColorScheme::Blue)};

  for (WInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new WQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-WMath::Infinity<double>());
    m_pWidget[c]->setMaximum(WMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    QLabel* pLabel = new QLabel(szLabels[c]);
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(szTooltip[c]);

    m_pLayout->addWidget(pLabel);
    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void WQtPropertyEditorQuaternionWidget::OnInit() {}

void WQtPropertyEditorQuaternionWidget::InternalSetValue(const WVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  WQtScopedBlockSignals b0(m_pWidget[0]);
  WQtScopedBlockSignals b1(m_pWidget[1]);
  WQtScopedBlockSignals b2(m_pWidget[2]);

  if (value.IsValid())
  {
    const WQuat qRot = value.ConvertTo<WQuat>();
    WAngle x, y, z;
    qRot.GetAsEulerAngles(x, y, z);

    m_pWidget[0]->setValue(x.GetDegree());
    m_pWidget[1]->setValue(y.GetDegree());
    m_pWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pWidget[0]->setValueInvalid();
    m_pWidget[1]->setValueInvalid();
    m_pWidget[2]->setValueInvalid();
  }
}

void WQtPropertyEditorQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  WAngle x = WAngle::MakeFromDegree(m_pWidget[0]->value());
  WAngle y = WAngle::MakeFromDegree(m_pWidget[1]->value());
  WAngle z = WAngle::MakeFromDegree(m_pWidget[2]->value());

  WQuat qRot = WQuat::MakeFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** TRANSFORM ***

WQtPropertyEditorTransformWidget::WQtPropertyEditorTransformWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  const char* szXYZLabels[] = {"X",
    "Y",
    "Z"};
  const char* szRotLabels[] = {"R",
    "P",
    "Y"};
  const char* szRotTooltip[] = {"Roll (Rotation around the forward axis)",
    "Pitch (Rotation around the side axis)",
    "Yaw (Rotation around the up axis)"};

  const WColorGammaUB labelColors[] = {WColorScheme::LightUI(WColorScheme::Red),
    WColorScheme::LightUI(WColorScheme::Green),
    WColorScheme::LightUI(WColorScheme::Blue)};

  WUInt32 uiCurrentWidget = 0;
  auto AddWidget = [&](QLayout* layout, double fStep, const char* szLabel, const char* szTooltip, WColorGammaUB color, const char* szDisplaySuffix)
  {
    auto pSpinBox = new WQtDoubleSpinBox(this);
    pSpinBox->installEventFilter(this);
    pSpinBox->setMinimum(-WMath::Infinity<double>());
    pSpinBox->setMaximum(WMath::Infinity<double>());
    pSpinBox->setSingleStep(fStep);
    pSpinBox->setAccelerated(true);
    pSpinBox->setDisplaySuffix(szDisplaySuffix);

    policy.setHorizontalStretch(2);
    pSpinBox->setSizePolicy(policy);

    QLabel* pLabel = new QLabel(szLabel);
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(color.r, color.g, color.b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(szTooltip);

    layout->addWidget(pLabel);
    layout->addWidget(pSpinBox);

    connect(pSpinBox, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));

    m_pWidget[uiCurrentWidget] = pSpinBox;
    ++uiCurrentWidget;
  };

  // Position
  {
    auto pSubLayout = new QHBoxLayout();
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (WUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 0.1, szXYZLabels[c], "", labelColors[c], "");
    }
  }

  // Rotation
  {
    auto pSubLayout = new QHBoxLayout();
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (WUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 1.0, szRotLabels[c], szRotTooltip[c], labelColors[c], "\xC2\xB0");
    }
  }

  // Scale
  {
    auto pSubLayout = new QHBoxLayout();
    pSubLayout->setSpacing(6);
    m_pLayout->addLayout(pSubLayout);

    for (WUInt32 c = 0; c < 3; ++c)
    {
      AddWidget(pSubLayout, 0.1, szXYZLabels[c], "", labelColors[c], "");
    }
  }
}

void WQtPropertyEditorTransformWidget::OnInit() {}

void WQtPropertyEditorTransformWidget::InternalSetValue(const WVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  WQtScopedBlockSignals b0(m_pWidget[0]);
  WQtScopedBlockSignals b1(m_pWidget[1]);
  WQtScopedBlockSignals b2(m_pWidget[2]);
  WQtScopedBlockSignals b3(m_pWidget[3]);
  WQtScopedBlockSignals b4(m_pWidget[4]);
  WQtScopedBlockSignals b5(m_pWidget[5]);
  WQtScopedBlockSignals b6(m_pWidget[6]);
  WQtScopedBlockSignals b7(m_pWidget[7]);
  WQtScopedBlockSignals b8(m_pWidget[8]);

  if (value.IsValid())
  {
    const WTransform t = value.ConvertTo<WTransform>();

    m_pWidget[0]->setValue(t.m_vPosition.x);
    m_pWidget[1]->setValue(t.m_vPosition.y);
    m_pWidget[2]->setValue(t.m_vPosition.z);

    WAngle x, y, z;
    t.m_qRotation.GetAsEulerAngles(x, y, z);
    m_pWidget[3]->setValue(x.GetDegree());
    m_pWidget[4]->setValue(y.GetDegree());
    m_pWidget[5]->setValue(z.GetDegree());

    m_pWidget[6]->setValue(t.m_vScale.x);
    m_pWidget[7]->setValue(t.m_vScale.y);
    m_pWidget[8]->setValue(t.m_vScale.z);
  }
  else
  {
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_pWidget); ++i)
    {
      m_pWidget[i]->setValueInvalid();
    }
  }
}

void WQtPropertyEditorTransformWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtPropertyEditorTransformWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }

  WTransform t;

  t.m_vPosition.x = m_pWidget[0]->value();
  t.m_vPosition.y = m_pWidget[1]->value();
  t.m_vPosition.z = m_pWidget[2]->value();

  WAngle x = WAngle::MakeFromDegree(m_pWidget[3]->value());
  WAngle y = WAngle::MakeFromDegree(m_pWidget[4]->value());
  WAngle z = WAngle::MakeFromDegree(m_pWidget[5]->value());
  t.m_qRotation = WQuat::MakeFromEulerAngles(x, y, z);

  t.m_vScale.x = m_pWidget[6]->value();
  t.m_vScale.y = m_pWidget[7]->value();
  t.m_vScale.z = m_pWidget[8]->value();

  BroadcastValueChanged(t);
}

/// *** LINEEDIT ***

WQtPropertyEditorLineEditWidget::WQtPropertyEditorLineEditWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pWarningIcon = new QLabel(this);
  m_pWarningIcon->setPixmap(WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg").pixmap(16, 16));
  m_pWarningIcon->setToolTip(QStringLiteral("This property is required and must not be empty."));
  m_pWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_pWarningIcon->setVisible(false);

  m_pLayout->addWidget(m_pWidget, 1);
  m_pLayout->addWidget(m_pWarningIcon);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
}

void WQtPropertyEditorLineEditWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  m_pWidget->setReadOnly(bReadOnly);
}

void WQtPropertyEditorLineEditWidget::OnInit()
{
  if (m_pProp->GetAttributeByType<WReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    setEnabled(true);
    WQtScopedBlockSignals bs(m_pWidget);

    m_pWidget->setReadOnly(true);
    QPalette palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void WQtPropertyEditorLineEditWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == WVariantType::Invalid)
  {
    m_OriginalType = WVariantType::String;
  }

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
    m_pWarningIcon->setVisible(false);
  }
  else
  {
    const WString sValue = value.ConvertTo<WString>();

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sValue.GetData()));

    const bool bRequired = m_pProp->GetAttributeByType<WRequiredAttribute>() != nullptr;
    m_pWarningIcon->setVisible(bRequired && sValue.IsEmpty());
  }
}

void WQtPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(WVariant(value.toUtf8().data()).ConvertTo(m_OriginalType));
}

void WQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(WVariant(m_pWidget->text().toUtf8().data()).ConvertTo(m_OriginalType));
}


/// *** COLOR ***

WQtColorButtonWidget::WQtColorButtonWidget(QWidget* pParent)
  : QFrame(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
}

void WQtColorButtonWidget::SetColor(const WVariant& color)
{
  if (color.IsValid())
  {
    WColor col0 = color.ConvertTo<WColor>();
    col0.NormalizeToLdrRange();

    const WColorGammaUB col = col0;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));
    setPalette(m_Pal);
  }
  else
  {
    const WColorGammaUB col = WColor::LightGrey;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::DiagCrossPattern));
    setPalette(m_Pal);
  }
}

void WQtColorButtonWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  QFrame::showEvent(event);
}

void WQtColorButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

QSize WQtColorButtonWidget::sizeHint() const
{
  return minimumSizeHint();
}

QSize WQtColorButtonWidget::minimumSizeHint() const
{
  QFontMetrics fm(font());

  QStyleOptionFrame opt;
  initStyleOption(&opt);
  return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(20, fm.height()), this);
}

WQtPropertyEditorColorWidget::WQtPropertyEditorColorWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new WQtColorButtonWidget(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  W_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void WQtPropertyEditorColorWidget::OnInit()
{
  m_bExposeAlpha = (m_pProp->GetAttributeByType<WExposeColorAlphaAttribute>() != nullptr);
  m_bExposeAlpha |= (m_pProp->GetSpecificType() == WGetStaticRTTI<WVariant>());
}

void WQtPropertyEditorColorWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);

  m_bIsHDR = value.GetType() == WVariantType::Color;
}

void WQtPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(WPropertyEvent::Type::BeginTemporary);

  WColor temp = WColor::White;
  if (m_OriginalValue.IsValid())
  {
    temp = m_OriginalValue.ConvertTo<WColor>();
  }

  WQtUiServices::GetSingleton()->ShowColorDialog(temp, m_bExposeAlpha, m_bIsHDR, this, SLOT(on_CurrentColor_changed(const WColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void WQtPropertyEditorColorWidget::on_CurrentColor_changed(const WColor& color)
{
  WVariant col;

  if (!m_bIsHDR)
  {
    // WVariant does not down-cast to WColorGammaUB automatically
    col = WColorGammaUB(color);
  }
  else
  {
    col = color;
  }

  m_pWidget->SetColor(col);
  BroadcastValueChanged(col);
}

void WQtPropertyEditorColorWidget::on_Color_reset()
{
  m_pWidget->SetColor(m_OriginalValue);
  Broadcast(WPropertyEvent::Type::CancelTemporary);
}

void WQtPropertyEditorColorWidget::on_Color_accepted()
{
  m_OriginalValue = GetOldValue();
  Broadcast(WPropertyEvent::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

WQtPropertyEditorEnumWidget::WQtPropertyEditorEnumWidget()
  : WQtStandardPropertyWidget()
{

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
}

void WQtPropertyEditorEnumWidget::OnInit()
{
  const WRTTI* pType = m_pProp->GetSpecificType();

  const WUInt32 uiCount = pType->GetProperties().GetCount();

  WTempHybridArray<const WAbstractProperty*, 16> props;

  // Start at 1 to skip default value.
  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != WPropertyCategory::Constant)
      continue;

    props.PushBack(pProp);
  }

  // this code path implements using multiple buttons in a row instead of a combobox, for small number of entries
  // it works for 2 elements, but often already looks bad with 3 elements
  // but even with 2 elements, it just adds visual clutter (unused values are now visible)
  // so I'm not going to enable it, but keep it in, in case we want to try it again in the future
  constexpr bool bUseButtons = false;

  if (bUseButtons && props.GetCount() <= W_ARRAY_SIZE(m_pButtons))
  {
    for (WUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const WAbstractConstantProperty* pConstant = static_cast<const WAbstractConstantProperty*>(pProp);

      m_pButtons[i] = new QPushButton(this);
      m_pButtons[i]->setText(WMakeQString(WTranslate(pConstant->GetPropertyName())));
      m_pButtons[i]->setCheckable(true);
      m_pButtons[i]->setProperty("value", pConstant->GetConstant().ConvertTo<WInt64>());

      connect(m_pButtons[i], SIGNAL(clicked(bool)), this, SLOT(on_ButtonClicked_changed(bool)));

      m_pLayout->addWidget(m_pButtons[i]);
    }
  }
  else
  {
    m_pWidget = new QComboBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));

    WQtScopedBlockSignals bs(m_pWidget);

    for (WUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const WAbstractConstantProperty* pConstant = static_cast<const WAbstractConstantProperty*>(pProp);

      m_pWidget->addItem(WMakeQString(WTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<WInt64>());
    }
  }
}

void WQtPropertyEditorEnumWidget::InternalSetValue(const WVariant& value)
{

  if (m_pWidget)
  {
    WInt32 iIndex = -1;
    if (value.IsValid())
    {
      iIndex = m_pWidget->findData(value.ConvertTo<WInt64>());
      W_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    }

    WQtScopedBlockSignals b(m_pWidget);
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    const WInt64 iValue = value.ConvertTo<WInt64>();

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_pButtons); ++i)
    {
      if (m_pButtons[i])
      {
        const WInt64 iButtonValue = m_pButtons[i]->property("value").toLongLong();

        WQtScopedBlockSignals b(m_pButtons[i]);
        m_pButtons[i]->setChecked(iButtonValue == iValue);
      }
    }
  }
}

void WQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  const WInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}

void WQtPropertyEditorEnumWidget::on_ButtonClicked_changed(bool checked)
{
  if (QPushButton* pButton = qobject_cast<QPushButton*>(sender()))
  {
    const WInt64 iValue = pButton->property("value").toLongLong();
    BroadcastValueChanged(iValue);
  }
}

/// *** BITFLAGS COMBOBOX ***

WQtPropertyEditorBitflagsWidget::WQtPropertyEditorBitflagsWidget()
  : WQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

WQtPropertyEditorBitflagsWidget::~WQtPropertyEditorBitflagsWidget()
{
  m_pWidget->setMenu(nullptr);
  delete m_pMenu;
}

void WQtPropertyEditorBitflagsWidget::OnInit()
{
  const WRTTI* enumType = m_pProp->GetSpecificType();

  const WRTTI* pType = enumType;
  WUInt32 uiCount = pType->GetProperties().GetCount();

  // Start at 1 to skip default value.
  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != WPropertyCategory::Constant)
      continue;

    const WAbstractConstantProperty* pConstant = static_cast<const WAbstractConstantProperty*>(pProp);

    QWidgetAction* pAction = new QWidgetAction(m_pMenu);
    QCheckBox* pCheckBox = new QCheckBox(WMakeQString(WTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<WInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }

  // sets all bits to clear or set
  {
    QWidgetAction* pAllAction = new QWidgetAction(m_pMenu);
    m_pAllButton = new QPushButton(QString::fromUtf8("All"), m_pMenu);
    connect(m_pAllButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(true); });
    pAllAction->setDefaultWidget(m_pAllButton);
    m_pMenu->addAction(pAllAction);

    QWidgetAction* pClearAction = new QWidgetAction(m_pMenu);
    m_pClearButton = new QPushButton(QString::fromUtf8("Clear"), m_pMenu);
    connect(m_pClearButton, &QPushButton::clicked, this, [this](bool bChecked)
      { SetAllChecked(false); });
    pClearAction->setDefaultWidget(m_pClearButton);
    m_pMenu->addAction(pClearAction);
  }
}

void WQtPropertyEditorBitflagsWidget::InternalSetValue(const WVariant& value)
{
  WQtScopedBlockSignals b(m_pWidget);
  m_iCurrentBitflags = value.ConvertTo<WInt64>();

  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = (it.Key() & m_iCurrentBitflags) != 0;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
    }
    it.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);
}

void WQtPropertyEditorBitflagsWidget::SetAllChecked(bool bChecked)
{
  for (auto& pCheckBox : m_Constants)
  {
    pCheckBox.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
}

void WQtPropertyEditorBitflagsWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void WQtPropertyEditorBitflagsWidget::on_Menu_aboutToHide()
{
  WInt64 iValue = 0;
  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool bChecked = it.Value()->checkState() == Qt::Checked;
    QString sName = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
      iValue |= it.Key();
    }
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);

  if (m_iCurrentBitflags != iValue)
  {
    m_iCurrentBitflags = iValue;
    BroadcastValueChanged(m_iCurrentBitflags);
  }
}


/// *** CURVE1D ***

WQtCurve1DButtonWidget::WQtCurve1DButtonWidget(QWidget* pParent)
  : QLabel(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
}

void WQtCurve1DButtonWidget::UpdatePreview(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange)
{
  WInt32 iNumPoints = 0;
  pObjectAccessor->GetCountByName(pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();

  WVariant v;
  WTempHybridArray<WVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = static_cast<double>(WColorGradient::TimeToTick(fLowerExtents));
  double maxX = static_cast<double>(WColorGradient::TimeToTick(fUpperExtents));

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (WInt32 i = 0; i < iNumPoints; ++i)
  {
    const WDocumentObject* pPoint = pObjectAccessor->GetChildObjectByName(pCurveObject, "ControlPoints", i);

    WVec2d p;

    pObjectAccessor->GetValueByName(pPoint, "Tick", v).AssertSuccess();
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValueByName(pPoint, "Value", v).AssertSuccess();
    p.y = v.ConvertTo<double>();

    points.PushBack(p);

    if (!bLowerFixed)
      minX = WMath::Min(minX, p.x);

    if (!bUpperFixed)
      maxX = WMath::Max(maxX, p.x);

    minY = WMath::Min(minY, p.y);
    maxY = WMath::Max(maxY, p.y);
  }

  const double pW = WMath::Max(10, size().width());
  const double pH = WMath::Clamp(size().height(), 5, 24);

  QPixmap pixmap((int)pW, (int)pH);
  pixmap.fill(palette().base().color());

  QPainter pt(&pixmap);
  pt.setPen(color);
  pt.setRenderHint(QPainter::RenderHint::Antialiasing);

  if (!points.IsEmpty())
  {
    points.Sort([](const WVec2d& lhs, const WVec2d& rhs) -> bool
      { return lhs.x < rhs.x; });

    const double normX = 1.0 / (maxX - minX);
    const double normY = 1.0 / (maxY - minY);

    QPainterPath path;

    {
      double startX = WMath::Min(minX, points[0].x);
      double startY = points[0].y;

      startX = (startX - minX) * normX;
      startY = 1.0 - ((startY - minY) * normY);

      path.moveTo((int)(startX * pW), (int)(startY * pH));
    }

    for (WUInt32 i = 0; i < points.GetCount(); ++i)
    {
      auto pt0 = points[i];
      pt0.x = (pt0.x - minX) * normX;
      pt0.y = 1.0 - ((pt0.y - minY) * normY);

      path.lineTo((int)(pt0.x * pW), (int)(pt0.y * pH));
    }

    {
      double endX = WMath::Max(maxX, points.PeekBack().x);
      double endY = points.PeekBack().y;

      endX = (endX - minX) * normX;
      endY = 1.0 - ((endY - minY) * normY);

      path.lineTo((int)(endX * pW), (int)(endY * pH));
    }

    pt.drawPath(path);
  }
  else
  {
    const double normY = 1.0 / (maxY - minY);
    double valY = 1.0 - ((fDefaultValue - minY) * normY);

    pt.drawLine(0, (int)(valY * pH), (int)pW, (int)(valY * pH));
  }

  setPixmap(pixmap);
}

void WQtCurve1DButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

WQtPropertyEditorCurve1DWidget::WQtPropertyEditorCurve1DWidget()
  : WQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new WQtCurve1DButtonWidget(this);
  m_pButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pButton);

  W_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void WQtPropertyEditorCurve1DWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtPropertyWidget::SetSelection(items);

  UpdatePreview();
}

void WQtPropertyEditorCurve1DWidget::OnInit()
{
  m_pObjectAccessor->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtPropertyEditorCurve1DWidget::PropertyEventHandler, this), m_Unsub);
  m_pObjectAccessor->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WQtPropertyEditorCurve1DWidget::StructureEventHandler, this), m_Unsub2);
}

void WQtPropertyEditorCurve1DWidget::DoPrepareToDie()
{
  m_Unsub.Unsubscribe();
}

void WQtPropertyEditorCurve1DWidget::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (m_Items.IsEmpty())
    return;

  const WDocumentObject* pParent = m_Items[0].m_pObject;
  const WDocumentObject* pCurve = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});

  if (pCurve == nullptr)
    return;

  if (e.m_pObject == pCurve || e.m_pObject->GetParent() == pCurve)
  {
    UpdatePreview();
  }
}

void WQtPropertyEditorCurve1DWidget::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_EventType != WDocumentObjectStructureEvent::Type::AfterObjectAdded && e.m_EventType != WDocumentObjectStructureEvent::Type::AfterObjectRemoved)
    return;

  if (IsUndead())
    return;

  if (m_Items.IsEmpty())
    return;

  if (e.m_sParentProperty != "ControlPoints")
    return;

  UpdatePreview();
}

void WQtPropertyEditorCurve1DWidget::UpdatePreview()
{
  if (m_Items.IsEmpty())
    return;

  const WDocumentObject* pParent = m_Items[0].m_pObject;
  const WDocumentObject* pCurve = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
  const WColorAttribute* pColorAttr = m_pProp->GetAttributeByType<WColorAttribute>();
  const WCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<WCurveExtentsAttribute>();
  const WDefaultValueAttribute* pDefAttr = m_pProp->GetAttributeByType<WDefaultValueAttribute>();
  const WClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<WClampValueAttribute>();

  const bool bLowerFixed = pExtentsAttr ? pExtentsAttr->m_bLowerExtentFixed : false;
  const bool bUpperFixed = pExtentsAttr ? pExtentsAttr->m_bUpperExtentFixed : false;
  const double fLowerExt = pExtentsAttr ? pExtentsAttr->m_fLowerExtent : 0.0;
  const double fUpperExt = pExtentsAttr ? pExtentsAttr->m_fUpperExtent : 1.0;
  const WColorGammaUB color = pColorAttr ? pColorAttr->GetColor() : WColor::GreenYellow;
  const double fLowerRange = (pClampAttr && pClampAttr->GetMinValue().IsNumber()) ? pClampAttr->GetMinValue().ConvertTo<double>() : 0.0;
  const double fUpperRange = (pClampAttr && pClampAttr->GetMaxValue().IsNumber()) ? pClampAttr->GetMaxValue().ConvertTo<double>() : 1.0;
  const double fDefVal = (pDefAttr && pDefAttr->GetValue().IsNumber()) ? pDefAttr->GetValue().ConvertTo<double>() : 0.0;

  m_pButton->UpdatePreview(m_pObjectAccessor, pCurve, QColor(color.r, color.g, color.b), fLowerExt, bLowerFixed, fUpperExt, bUpperFixed, fDefVal, fLowerRange, fUpperRange);
}

void WQtPropertyEditorCurve1DWidget::on_Button_triggered()
{
  const WDocumentObject* pParent = m_Items[0].m_pObject;
  const WDocumentObject* pCurve = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
  const WColorAttribute* pColorAttr = m_pProp->GetAttributeByType<WColorAttribute>();
  const WCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<WCurveExtentsAttribute>();
  const WClampValueAttribute* pClampAttr = m_pProp->GetAttributeByType<WClampValueAttribute>();

  // TODO: would like to have one transaction open to finish/cancel at the end
  // but also be able to undo individual steps while editing
  // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->StartTransaction("Edit Curve");

  WStringBuilder sTitle = WTranslate(m_pProp->GetPropertyName());
  WQtCurveEditDlg* pDlg = new WQtCurveEditDlg(m_pObjectAccessor, pCurve, this, sTitle);
  pDlg->restoreGeometry(WQtCurveEditDlg::GetLastDialogGeometry());

  if (pColorAttr)
  {
    pDlg->SetCurveColor(pColorAttr->GetColor());
  }

  if (pExtentsAttr)
  {
    pDlg->SetCurveExtents(pExtentsAttr->m_fLowerExtent, pExtentsAttr->m_bLowerExtentFixed, pExtentsAttr->m_fUpperExtent, pExtentsAttr->m_bUpperExtentFixed);
  }

  if (pClampAttr)
  {
    const double fLower = pClampAttr->GetMinValue().IsNumber() ? pClampAttr->GetMinValue().ConvertTo<double>() : -WMath::HighValue<double>();
    const double fUpper = pClampAttr->GetMaxValue().IsNumber() ? pClampAttr->GetMaxValue().ConvertTo<double>() : WMath::HighValue<double>();

    pDlg->SetCurveRanges(fLower, fUpper);
  }

  if (pDlg->exec() == QDialog::Accepted)
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->FinishTransaction();

    UpdatePreview();
  }
  else
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->CancelTransaction();
  }

  delete pDlg;
}


/// *** COLOR GRADIENT ***

WQtColorGradientButtonWidget::WQtColorGradientButtonWidget(QWidget* pParent)
  : QLabel(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
  setMinimumHeight(24);
}

void WQtColorGradientButtonWidget::UpdatePreview(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pGradientObject)
{
  if (pGradientObject == nullptr)
  {
    setPixmap(QPixmap());
    return;
  }

  // Reconstruct gradient from document object
  WColorGradient gradient;

  // Read ColorCPs array
  WInt32 iNumColorCPs = 0;
  pObjectAccessor->GetCountByName(pGradientObject, "ColorCPs", iNumColorCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumColorCPs; ++i)
  {
    const WDocumentObject* pCP = pObjectAccessor->GetChildObjectByName(pGradientObject, "ColorCPs", i);
    WVariant v;

    pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();

    pObjectAccessor->GetValueByName(pCP, "Red", v).AssertSuccess();
    WUInt8 r = v.ConvertTo<WUInt8>();
    pObjectAccessor->GetValueByName(pCP, "Green", v).AssertSuccess();
    WUInt8 g = v.ConvertTo<WUInt8>();
    pObjectAccessor->GetValueByName(pCP, "Blue", v).AssertSuccess();
    WUInt8 b = v.ConvertTo<WUInt8>();

    gradient.AddColorControlPoint(WColorGradient::TickToTime(tick), WColorGammaUB(r, g, b));
  }

  // Read AlphaCPs array
  WInt32 iNumAlphaCPs = 0;
  pObjectAccessor->GetCountByName(pGradientObject, "AlphaCPs", iNumAlphaCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumAlphaCPs; ++i)
  {
    const WDocumentObject* pCP = pObjectAccessor->GetChildObjectByName(pGradientObject, "AlphaCPs", i);
    WVariant v;

    pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();
    pObjectAccessor->GetValueByName(pCP, "Alpha", v).AssertSuccess();
    WUInt8 alpha = v.ConvertTo<WUInt8>();

    gradient.AddAlphaControlPoint(WColorGradient::TickToTime(tick), alpha);
  }

  // Read IntensityCPs array
  WInt32 iNumIntensityCPs = 0;
  pObjectAccessor->GetCountByName(pGradientObject, "IntensityCPs", iNumIntensityCPs).AssertSuccess();

  for (WInt32 i = 0; i < iNumIntensityCPs; ++i)
  {
    const WDocumentObject* pCP = pObjectAccessor->GetChildObjectByName(pGradientObject, "IntensityCPs", i);
    WVariant v;

    pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    WInt64 tick = v.ConvertTo<WInt64>();
    pObjectAccessor->GetValueByName(pCP, "Intensity", v).AssertSuccess();
    float intensity = v.ConvertTo<float>();

    gradient.AddIntensityControlPoint(WColorGradient::TickToTime(tick), intensity);
  }

  // Generate preview image
  const int pW = 64;
  const int pH = 24;

  QPixmap pixmap(pW, pH);
  QPainter pt(&pixmap);

  pt.fillRect(0, 0, pW, pH, Qt::white);

  if (!gradient.IsEmpty())
  {
    // Draw gradient preview
    for (int x = 0; x < pW; ++x)
    {
      const double t = (double)x / (double)(pW - 1);

      WColorGammaUB rgba;
      float fIntensity;
      gradient.Evaluate(t, rgba, fIntensity);

      // Apply intensity to get final color
      WColor hdrColor = rgba;
      hdrColor.r *= fIntensity;
      hdrColor.g *= fIntensity;
      hdrColor.b *= fIntensity;

      // Convert to LDR for display
      WColorGammaUB ldrColor = hdrColor;

      pt.setPen(QColor(ldrColor.r, ldrColor.g, ldrColor.b, ldrColor.a));
      pt.drawLine(x, 0, x, pH);
    }
  }

  setPixmap(pixmap);
}

void WQtColorGradientButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}


WQtPropertyEditorColorGradientWidget::WQtPropertyEditorColorGradientWidget()
  : WQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new WQtColorGradientButtonWidget(this);
  m_pButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pButton);

  W_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void WQtPropertyEditorColorGradientWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtPropertyWidget::SetSelection(items);
  UpdatePreview();
}

void WQtPropertyEditorColorGradientWidget::OnInit()
{
  m_pObjectAccessor->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtPropertyEditorColorGradientWidget::PropertyEventHandler, this), m_Unsub);
  m_pObjectAccessor->GetObjectManager()->m_ObjectEvents.AddEventHandler(WMakeDelegate(&WQtPropertyEditorColorGradientWidget::ObjectEventHandler, this), m_Unsub2);
}

void WQtPropertyEditorColorGradientWidget::DoPrepareToDie()
{
  m_Unsub.Unsubscribe();
  m_Unsub2.Unsubscribe();
}

void WQtPropertyEditorColorGradientWidget::PropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  UpdatePreview();
}

void WQtPropertyEditorColorGradientWidget::ObjectEventHandler(const WDocumentObjectEvent& e)
{
  if (IsUndead())
    return;

  UpdatePreview();
}

void WQtPropertyEditorColorGradientWidget::UpdatePreview()
{
  if (m_Items.IsEmpty())
    return;

  const WDocumentObject* pParent = m_Items[0].m_pObject;
  const WDocumentObject* pGradient = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});

  m_pButton->UpdatePreview(m_pObjectAccessor, pGradient);
}

void WQtPropertyEditorColorGradientWidget::on_Button_triggered()
{
  const WDocumentObject* pParent = m_Items[0].m_pObject;
  const WDocumentObject* pGradient = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});

  WStringBuilder sTitle = WTranslate(m_pProp->GetPropertyName());
  WQtColorGradientEditDlg* pDlg = new WQtColorGradientEditDlg(m_pObjectAccessor, pGradient, this, sTitle);
  pDlg->restoreGeometry(WQtColorGradientEditDlg::GetLastDialogGeometry());

  pDlg->exec();
  delete pDlg;
}
