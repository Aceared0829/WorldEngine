#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/SphereReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WSphereReflectionProbeComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WClampValueAttribute(0.0f, {}), new WDefaultValueAttribute(5.0f)),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f), new WDefaultValueAttribute(0.1f)),
    W_ACCESSOR_PROPERTY("SphereProjection", GetSphereProjection, SetSphereProjection)->AddAttributes(new WDefaultValueAttribute(true)),
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
    new WSphereVisualizerAttribute("Radius", WColorScheme::LightUI(WColorScheme::Blue)),
    new WSphereManipulatorAttribute("Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WSphereReflectionProbeComponentManager::WSphereReflectionProbeComponentManager(WWorld* pWorld)
  : WComponentManager<WSphereReflectionProbeComponent, WBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

WSphereReflectionProbeComponent::WSphereReflectionProbeComponent() = default;
WSphereReflectionProbeComponent::~WSphereReflectionProbeComponent() = default;

void WSphereReflectionProbeComponent::SetRadius(float fRadius)
{
  m_fRadius = WMath::Max(fRadius, 0.0f);
  m_bStatesDirty = true;
}

float WSphereReflectionProbeComponent::GetRadius() const
{
  return m_fRadius;
}

void WSphereReflectionProbeComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = WMath::Clamp(fFalloff, WMath::DefaultEpsilon<float>(), 1.0f);
}

void WSphereReflectionProbeComponent::SetSphereProjection(bool bSphereProjection)
{
  m_bSphereProjection = bSphereProjection;
}

void WSphereReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = WReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void WSphereReflectionProbeComponent::OnDeactivated()
{
  WReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void WSphereReflectionProbeComponent::OnObjectCreated(const WAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void WSphereReflectionProbeComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(WDefaultSpatialDataCategories::RenderDynamic);
}

void WSphereReflectionProbeComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
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
  pRenderData->m_vHalfExtents = WVec3(m_fRadius);
  pRenderData->m_vInfluenceScale = WVec3(1.0f);
  pRenderData->m_vInfluenceShift = WVec3(0.0f);
  pRenderData->m_vPositiveFalloff = WVec3(m_fFalloff);
  pRenderData->m_vNegativeFalloff = WVec3(m_fFalloff);
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = REFLECTION_PROBE_IS_SPHERE;
  if (m_bSphereProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const WVec3 vScale = globalTransform.m_vScale * m_fRadius;
  constexpr float fSphereConstant = (4.0f / 3.0f) * WMath::Pi<float>();
  const float fEllipsoidVolume = fSphereConstant * WMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fEllipsoidVolume, vScale);
  WReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void WSphereReflectionProbeComponent::OnTransformChanged(WMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void WSphereReflectionProbeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  WStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
  s << m_bSphereProjection;
}

void WSphereReflectionProbeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bSphereProjection;
  }
  else
  {
    m_bSphereProjection = false;
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WSphereReflectionProbeComponent_1_2 : public WGraphPatch
{
public:
  WSphereReflectionProbeComponent_1_2()
    : WGraphPatch("WSphereReflectionProbeComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("SphereProjection", false);
  }
};

WSphereReflectionProbeComponent_1_2 g_WSphereReflectionProbeComponent_1_2;

W_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
