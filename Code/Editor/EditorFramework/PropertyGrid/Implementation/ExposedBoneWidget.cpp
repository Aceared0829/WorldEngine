#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WQtExposedBoneWidget::WQtExposedBoneWidget()
{
  m_pRotWidget[0] = nullptr;
  m_pRotWidget[1] = nullptr;
  m_pRotWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (WInt32 c = 0; c < 3; ++c)
  {
    m_pRotWidget[c] = new WQtDoubleSpinBox(this);
    m_pRotWidget[c]->setMinimum(-WMath::Infinity<double>());
    m_pRotWidget[c]->setMaximum(WMath::Infinity<double>());
    m_pRotWidget[c]->setSingleStep(1.0);
    m_pRotWidget[c]->setAccelerated(true);
    m_pRotWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pRotWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pRotWidget[c]);

    connect(m_pRotWidget[c], SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
    connect(m_pRotWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void WQtExposedBoneWidget::SetSelection(const WArrayPtr<WPropertySelection>& items)
{
  WQtStandardPropertyWidget::SetSelection(items);
  W_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<WExposedBone>(), "Selection does not match WExposedBone.");
}

void WQtExposedBoneWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
  {
    Broadcast(WPropertyEvent::Type::BeginTemporary);
    m_bTemporaryCommand = true;
  }
}

void WQtExposedBoneWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(WPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void WQtExposedBoneWidget::SlotValueChanged()
{
  onBeginTemporary();

  auto obj = m_OldValue.Get<WTypedObject>();
  WExposedBone* pCopy = reinterpret_cast<WExposedBone*>(WReflectionSerializer::Clone(obj.m_pObject, obj.m_pType));

  {
    WAngle x = WAngle::MakeFromDegree(m_pRotWidget[0]->value());
    WAngle y = WAngle::MakeFromDegree(m_pRotWidget[1]->value());
    WAngle z = WAngle::MakeFromDegree(m_pRotWidget[2]->value());

    pCopy->m_Transform.m_qRotation = WQuat::MakeFromEulerAngles(x, y, z);
  }

  WVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}

void WQtExposedBoneWidget::OnInit()
{
}

void WQtExposedBoneWidget::InternalSetValue(const WVariant& value)
{
  if (value.GetReflectedType() != WGetStaticRTTI<WExposedBone>())
    return;

  const WExposedBone* pBone = reinterpret_cast<const WExposedBone*>(value.GetData());

  WQtScopedBlockSignals b0(m_pRotWidget[0]);
  WQtScopedBlockSignals b1(m_pRotWidget[1]);
  WQtScopedBlockSignals b2(m_pRotWidget[2]);

  if (value.IsValid())
  {
    WAngle x, y, z;
    pBone->m_Transform.m_qRotation.GetAsEulerAngles(x, y, z);

    m_pRotWidget[0]->setValue(x.GetDegree());
    m_pRotWidget[1]->setValue(y.GetDegree());
    m_pRotWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pRotWidget[0]->setValueInvalid();
    m_pRotWidget[1]->setValueInvalid();
    m_pRotWidget[2]->setValueInvalid();
  }
}
