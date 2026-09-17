#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

#include <../../Data/Base/Shaders/Common/LightData.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WBoxReflectionProbeComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new WClampValueAttribute(WVec3(0.0f), {}), new WDefaultValueAttribute(WVec3(5.0f))),
    W_ACCESSOR_PROPERTY("InfluenceScale", GetInfluenceScale, SetInfluenceScale)->AddAttributes(new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f)), new WDefaultValueAttribute(WVec3(1.0f))),
    W_ACCESSOR_PROPERTY("InfluenceShift", GetInfluenceShift, SetInfluenceShift)->AddAttributes(new WClampValueAttribute(WVec3(-1.0f), WVec3(1.0f)), new WDefaultValueAttribute(WVec3(0.0f))),
    W_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f)), new WDefaultValueAttribute(WVec3(0.1f, 0.1f, 0.0f))),
    W_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new WClampValueAttribute(WVec3(0.0f), WVec3(1.0f)), new WDefaultValueAttribute(WVec3(0.1f, 0.1f, 0.0f))),
    W_ACCESSOR_PROPERTY("BoxProjection", GetBoxProjection, SetBoxProjection)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(OnObjectCreated),
  }
  W_END_FUNCTIONS;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
    W_MESSAGE_HANDLER(WMsgTransformChanged, OnTransformChanged),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Rendering/Reflections"),
    new WBoxVisualizerAttribute("Extents", 1.0f, WColorScheme::LightUI(WColorScheme::Blue)),
    new WBoxManipulatorAttribute("Extents", 1.0f, true),
    new WBoxReflectionProbeVisualizerAttribute("Extents", "InfluenceScale", "InfluenceShift"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoxReflectionProbeVisualizerAttribute, 1, WRTTIDefaultAllocator<WBoxReflectionProbeVisualizerAttribute>)
{
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoxReflectionProbeComponentManager::WBoxReflectionProbeComponentManager(WWorld* pWorld)
  : WComponentManager<WBoxReflectionProbeComponent, WBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

WBoxReflectionProbeComponent::WBoxReflectionProbeComponent() = default;
WBoxReflectionProbeComponent::~WBoxReflectionProbeComponent() = default;

void WBoxReflectionProbeComponent::SetExtents(const WVec3& vExtents)
{
  m_vExtents = vExtents;
}

const WVec3& WBoxReflectionProbeComponent::GetInfluenceScale() const
{
  return m_vInfluenceScale;
}

void WBoxReflectionProbeComponent::SetInfluenceScale(const WVec3& vInfluenceScale)
{
  m_vInfluenceScale = vInfluenceScale;
}

const WVec3& WBoxReflectionProbeComponent::GetInfluenceShift() const
{
  return m_vInfluenceShift;
}

void WBoxReflectionProbeComponent::SetInfluenceShift(const WVec3& vInfluenceShift)
{
  m_vInfluenceShift = vInfluenceShift;
}

void WBoxReflectionProbeComponent::SetPositiveFalloff(const WVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vPositiveFalloff = vFalloff.CompClamp(WVec3(WMath::DefaultEpsilon<float>()), WVec3(1.0f));
}

void WBoxReflectionProbeComponent::SetNegativeFalloff(const WVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vNegativeFalloff = vFalloff.CompClamp(WVec3(WMath::DefaultEpsilon<float>()), WVec3(1.0f));
}

void WBoxReflectionProbeComponent::SetBoxProjection(bool bBoxProjection)
{
  m_bBoxProjection = bBoxProjection;
}

const WVec3& WBoxReflectionProbeComponent::GetExtents() const
{
  return m_vExtents;
}

void WBoxReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = WReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void WBoxReflectionProbeComponent::OnDeactivated()
{
  WReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void WBoxReflectionProbeComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void WBoxReflectionProbeComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(WDefaultSpatialDataCategories::RenderDynamic);
}

void WBoxReflectionProbeComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == WCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    WReflectionPool::UpdateReflectionProbe(GetWorld(), m_Id, m_Desc, this);
  }

  auto globalTransform = GetOwner()->GetGlobalTransform();

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<WReflectionProbeRenderData>(GetOwner());
  pRenderData->m_vGlobalPosition = globalTransform * m_Desc.m_vCaptureOffset;
  pRenderData->m_GlobalTransform = globalTransform;
  pRenderData->m_vHalfExtents = m_vExtents / 2.0f;
  pRenderData->m_vInfluenceScale = m_vInfluenceScale;
  pRenderData->m_vInfluenceShift = m_vInfluenceShift;
  pRenderData->m_vPositiveFalloff = m_vPositiveFalloff;
  pRenderData->m_vNegativeFalloff = m_vNegativeFalloff;
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = 0;
  if (m_bBoxProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const WVec3 vScale = pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents);
  const float fVolume = WMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fVolume, vScale);
  WReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void WBoxReflectionProbeComponent::OnTransformChanged(WMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void WBoxReflectionProbeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vInfluenceScale;
  s << m_vInfluenceShift;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
  s << m_bBoxProjection;
}

void WBoxReflectionProbeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vInfluenceScale;
  s >> m_vInfluenceShift;
  s >> m_vPositiveFalloff;
  s >> m_vNegativeFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bBoxProjection;
  }
}

//////////////////////////////////////////////////////////////////////////

WBoxReflectionProbeVisualizerAttribute::WBoxReflectionProbeVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WBoxReflectionProbeVisualizerAttribute::WBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty)
  : WVisualizerAttribute(szExtentsProperty, szInfluenceScaleProperty, szInfluenceShiftProperty)
{
}

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
