#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/BlackboardHelper.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFogRenderData, 1, WRTTIDefaultAllocator<WFogRenderData>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WFogComponent, 3, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new WDefaultValueAttribute(WColorGammaUB(WColor(0.2f, 0.2f, 0.3f)))),
    W_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f)),
    W_ACCESSOR_PROPERTY("StartDistance", GetStartDistance, SetStartDistance),
    W_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(10.0f)),
    W_ACCESSOR_PROPERTY("ModulateWithSkyColor", GetModulateWithSkyColor, SetModulateWithSkyColor),
    W_ACCESSOR_PROPERTY("SkyDistance", GetSkyDistance, SetSkyDistance)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1000.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WFogComponent::WFogComponent() = default;
WFogComponent::~WFogComponent() = default;

void WFogComponent::Deinitialize()
{
  WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void WFogComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void WFogComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void WFogComponent::SetColor(WColor color)
{
  m_Color = color;

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

WColor WFogComponent::GetColor() const
{
  return m_Color;
}

void WFogComponent::SetDensity(float fDensity)
{
  m_fDensity = WMath::Max(fDensity, 0.0f);

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float WFogComponent::GetDensity() const
{
  return m_fDensity;
}

void WFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = WMath::Max(fHeightFalloff, 0.0f);

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float WFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void WFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

bool WFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void WFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float WFogComponent::GetSkyDistance() const
{
  return m_fSkyDistance;
}

void WFogComponent::SetStartDistance(float fDistance)
{
  m_fStartDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    WRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float WFogComponent::GetStartDistance() const
{
  return m_fStartDistance;
}

void WFogComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? WDefaultSpatialDataCategories::RenderDynamic : WDefaultSpatialDataCategories::RenderStatic);
}

void WFogComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  const WBlackboard& blackboard = *GetWorld()->GetBlackboard().Borrow();
  const WColor color = W_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(m_Color, blackboard, Fog.Color);
  const float fDensity = W_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(m_fDensity, blackboard, Fog.Density);
  const float fHeightFalloff = W_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(m_fHeightFalloff, blackboard, Fog.HeightFalloff);
  const float fStartDistance = W_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(m_fStartDistance, blackboard, Fog.StartDistance);

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WFogRenderData>(GetOwner());

  pRenderData->m_Color = color;
  pRenderData->m_fDensity = fDensity / 100.0f;
  pRenderData->m_fBaseHeight = GetOwner()->GetGlobalTransform().m_vPosition.z;
  pRenderData->m_fHeightFalloff = fHeightFalloff;
  pRenderData->m_fInvSkyDistance = m_bModulateWithSkyColor ? 1.0f / m_fSkyDistance : 0.0f;
  pRenderData->m_fFogStartDistance = fStartDistance;
  pRenderData->m_uiSortingKey = WInvalidIndex;

  msg.AddRenderData(pRenderData, WDefaultRenderDataCategories::Light, WRenderData::Caching::Never);
}

void WFogComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
  s << m_fStartDistance;
}

void WFogComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }

  if (uiVersion >= 3)
  {
    s >> m_fStartDistance;
  }
}

W_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);
