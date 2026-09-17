#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WDebugTextComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new WDefaultValueAttribute("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")),
    W_MEMBER_PROPERTY("Value0", m_fValue0),
    W_MEMBER_PROPERTY("Value1", m_fValue1),
    W_MEMBER_PROPERTY("Value2", m_fValue2),
    W_MEMBER_PROPERTY("Value3", m_fValue3),
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Utilities/Debug"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDebugTextComponent::WDebugTextComponent()
  : m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")
  , m_Color(WColor::White)
{
}

WDebugTextComponent::~WDebugTextComponent() = default;

void WDebugTextComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
  s << m_fMaxDistance;
}

void WDebugTextComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;

  if (uiVersion >= 2)
  {
    s >> m_fMaxDistance;
  }
}

void WDebugTextComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow)
    return;

  if (m_sText.IsEmpty())
    return;

  const float fSquaredDistance = msg.m_pView->GetCullingCamera()->GetCenterPosition().GetSquaredDistanceTo(GetOwner()->GetGlobalPosition());
  if (fSquaredDistance > m_fMaxDistance * m_fMaxDistance)
    return;

  const float fFade = WMath::Saturate(WMath::Sqrt(fSquaredDistance) / WMath::Max(m_fMaxDistance, 0.0001f) * -5.0f + 5.0f);

  WColor c = m_Color;
  c.a *= fFade;

  WStringBuilder sb;
  sb.SetFormat(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

  WDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), c);
}



W_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugTextComponent);
