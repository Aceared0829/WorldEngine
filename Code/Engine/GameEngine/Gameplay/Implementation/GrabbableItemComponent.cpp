#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

struct GICFlags
{
  enum Enum
  {
    DebugShowPoints = 0,
  };
};

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WGrabbableItemGrabPoint, WNoBase, 1, WRTTIDefaultAllocator<WGrabbableItemGrabPoint>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    W_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
    new WTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE


W_BEGIN_COMPONENT_TYPE(WGrabbableItemComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("DebugShowPoints", GetDebugShowPoints, SetDebugShowPoints),
    W_ARRAY_MEMBER_PROPERTY("GrabPoints", m_GrabPoints),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds),
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WGrabbableItemComponent::WGrabbableItemComponent() = default;
WGrabbableItemComponent::~WGrabbableItemComponent() = default;

void WGrabbableItemComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  const WUInt8 uiNumGrabPoints = static_cast<WUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void WGrabbableItemComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  WUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}

void WGrabbableItemComponent::SetDebugShowPoints(bool bShow)
{
  SetUserFlag(GICFlags::DebugShowPoints, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool WGrabbableItemComponent::GetDebugShowPoints() const
{
  return GetUserFlag(GICFlags::DebugShowPoints);
}

void WGrabbableItemComponent::DebugDrawGrabPoint(const WWorld& world, const WTransform& globalGrabPointTransform)
{
  WDebugRenderer::DrawArrow(&world, 0.75f, WColorScheme::LightUI(WColorScheme::Red), globalGrabPointTransform, WVec3::MakeAxisX());
  WDebugRenderer::DrawArrow(&world, 0.3f, WColorScheme::LightUI(WColorScheme::Green), globalGrabPointTransform, WVec3::MakeAxisY());
  WDebugRenderer::DrawArrow(&world, 0.3f, WColorScheme::LightUI(WColorScheme::Blue), globalGrabPointTransform, WVec3::MakeAxisZ());
}

void WGrabbableItemComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const
{
  if (GetDebugShowPoints())
  {
    msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 1.0f), WDefaultSpatialDataCategories::RenderDynamic);
  }
}

void WGrabbableItemComponent::OnExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!GetDebugShowPoints() || m_GrabPoints.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != WCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    return;

  const WTransform globalTransform = GetOwner()->GetGlobalTransform();

  for (auto& grabPoint : m_GrabPoints)
  {
    WTransform grabPointTransform = WTransform::MakeGlobalTransform(globalTransform, WTransform(grabPoint.m_vLocalPosition, grabPoint.m_qLocalRotation));
    DebugDrawGrabPoint(*GetWorld(), grabPointTransform);
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
