#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

WSpatialData::Category WWindVolumeComponent::SpatialDataCategory = WSpatialData::RegisterCategory("WindVolumes", WSpatialData::Flags::None);

// clang-format off
W_BEGIN_ABSTRACT_COMPONENT_TYPE(WWindVolumeComponent, 3)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Strength", WWindStrength, m_Strength),
    W_MEMBER_PROPERTY("StrengthFactor", m_fStrengthFactor)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(-10, 10)),
    W_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    W_ENUM_MEMBER_PROPERTY("OnFinishedAction", WOnComponentFinishedAction, m_OnFinishedAction),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnTriggered),
    W_MESSAGE_HANDLER(WMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Effects/Wind"),
  }
  W_END_ATTRIBUTES;
}
W_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

WWindVolumeComponent::WWindVolumeComponent() = default;
WWindVolumeComponent::~WWindVolumeComponent() = default;

void WWindVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void WWindVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void WWindVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    WMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void WWindVolumeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
  s << m_fStrengthFactor;
}

void WWindVolumeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;

  if (uiVersion == 2)
  {
    bool bReverse = false;
    s >> bReverse;
    m_fStrengthFactor = bReverse ? -1.0f : 1.0f;
  }

  if (uiVersion >= 3)
  {
    s >> m_fStrengthFactor;
  }
}

WSimdVec4f WWindVolumeComponent::ComputeForceAtGlobalPosition(const WSimdVec4f& vGlobalPos) const
{
  const WSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const WSimdTransform tInv = t.GetInverse();
  const WSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

  const WSimdVec4f force = ComputeForceAtLocalPosition(localPos);

  return t.TransformDirection(force);
}

void WWindVolumeComponent::OnTriggered(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != WTempHashedString("Suicide"))
    return;

  WOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void WWindVolumeComponent::OnMsgDeleteGameObject(WMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    WOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

float WWindVolumeComponent::GetWindInMetersPerSecond() const
{
  return WWindStrength::GetInMetersPerSecond(m_Strength) * m_fStrengthFactor;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WWindVolumeComponentPatch_2_3 : public WGraphPatch
{
public:
  WWindVolumeComponentPatch_2_3()
    : WGraphPatch("WWindVolumeComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto pReverseDirection = pNode->FindProperty("ReverseDirection");
    if (pReverseDirection && pReverseDirection->m_Value.IsA<bool>())
    {
      float fFactor = pReverseDirection->m_Value.Get<bool>() ? -1.0f : 1.0f;
      pNode->AddProperty("StrengthFactor", fFactor);
    }
  }
};

WWindVolumeComponentPatch_2_3 g_WWindVolumeComponentPatch_2_3;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WWindVolumeSphereComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WSphereVisualizerAttribute("Radius", WColor::CornflowerBlue),
    new WSphereManipulatorAttribute("Radius"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WWindVolumeSphereComponent::WWindVolumeSphereComponent() = default;
WWindVolumeSphereComponent::~WWindVolumeSphereComponent() = default;

void WWindVolumeSphereComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void WWindVolumeSphereComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

WSimdVec4f WWindVolumeSphereComponent::ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const
{
  // TODO: could do this computation in global space

  WSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  WSimdFloat forceFactor = WSimdFloat(1.0f) - (lenScaled * lenScaled);

  const WSimdFloat force = GetWindInMetersPerSecond() * forceFactor.Max(0.0f);

  WSimdVec4f dir = vLocalPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void WWindVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = WMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WWindVolumeSphereComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), m_fRadius), WWindVolumeComponent::SpatialDataCategory);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WWindVolumeCylinderMode, 1)
  W_ENUM_CONSTANTS(WWindVolumeCylinderMode::Directional, WWindVolumeCylinderMode::Vortex)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WWindVolumeCylinderComponent, 2, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, WVariant())),
    W_ACCESSOR_PROPERTY("RadiusFalloff", GetRadiusFalloff, SetRadiusFalloff)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.1f, WVariant())),
    W_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new WClampValueAttribute(0.0f, 1.0f)),
    W_ENUM_MEMBER_PROPERTY("Mode", WWindVolumeCylinderMode, m_Mode),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WCylinderVisualizerAttribute(WBasisAxis::PositiveX, "Length", "Radius", WColor::CornflowerBlue),
    new WDirectionVisualizerAttribute(WBasisAxis::PositiveX, 1.0f, WColor::DeepSkyBlue),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WWindVolumeCylinderComponent::WWindVolumeCylinderComponent() = default;
WWindVolumeCylinderComponent::~WWindVolumeCylinderComponent() = default;

void WWindVolumeCylinderComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fRadiusFalloff;
  s << m_fLength;
  s << m_fPositiveFalloff;
  s << m_fNegativeFalloff;
  s << m_Mode;
}

void WWindVolumeCylinderComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  if (uiVersion >= 2)
  {
    s >> m_fRadiusFalloff;
  }

  s >> m_fLength;
  if (uiVersion >= 2)
  {
    s >> m_fPositiveFalloff;
    s >> m_fNegativeFalloff;
  }

  s >> m_Mode;

  ComputeScaleBiasValues();
}

WSimdVec4f WWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const
{
  WSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(WSimdFloat::MakeZero());
  const WSimdVec4f radius = WSimdVec4f(orthoDir.GetLength<3>());

  const WSimdVec4f ddr = vLocalPos.GetCombined<WSwizzle::XXXX>(radius); // dist, dist, radius
  WSimdVec4f fadeValues = WSimdVec4f::MulAdd(ddr, m_vScaleValues, m_vBiasValues);
  fadeValues = fadeValues.CompMin(WSimdVec4f(1.0f)).CompMax(WSimdVec4f::MakeZero());
  const WSimdFloat finalStrength = fadeValues.HorizontalMin<2>() * fadeValues.z() * WSimdFloat(GetWindInMetersPerSecond());

  WSimdVec4f dir = WSimdVec4f(1, 0, 0, 0);
  WSimdVec4f vortexDir = dir.CrossRH(orthoDir);
  vortexDir.NormalizeIfNotZero<3>();

  WSimdVec4b isVortex(m_Mode == WWindVolumeCylinderMode::Vortex);
  dir = WSimdVec4f::Select(isVortex, vortexDir, dir);

  return dir * finalStrength;
}

void WWindVolumeCylinderComponent::SetRadius(float fVal)
{
  m_fRadius = WMath::Max(fVal, 0.1f);

  ComputeScaleBiasValues();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WWindVolumeCylinderComponent::SetRadiusFalloff(float fVal)
{
  m_fRadiusFalloff = WMath::Saturate(fVal);

  ComputeScaleBiasValues();
}

void WWindVolumeCylinderComponent::SetLength(float fVal)
{
  m_fLength = WMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WWindVolumeCylinderComponent::SetPositiveFalloff(float fVal)
{
  m_fPositiveFalloff = WMath::Saturate(fVal);

  ComputeScaleBiasValues();
}

void WWindVolumeCylinderComponent::SetNegativeFalloff(float fVal)
{
  m_fNegativeFalloff = WMath::Saturate(fVal);

  ComputeScaleBiasValues();
}

void WWindVolumeCylinderComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  const WVec3 halfExtents(m_fLength * 0.5f, m_fRadius, m_fRadius);
  const float sphereRadius = halfExtents.GetAsVec2().GetLength();

  msg.AddBounds(WBoundingBoxSphere::MakeFromCenterExtents(WVec3::MakeZero(), halfExtents, sphereRadius), WWindVolumeComponent::SpatialDataCategory);
}

void WWindVolumeCylinderComponent::ComputeScaleBiasValues()
{
  const float fPositiveScale = -1.0f / WMath::Max(m_fLength * m_fPositiveFalloff, 0.0001f);
  const float fPositiveBias = -fPositiveScale * m_fLength * 0.5f;

  const float fNegativeFalloff = WMath::Min(m_fNegativeFalloff, 1.0f - m_fPositiveFalloff);
  const float fNegativeScale = 1.0f / WMath::Max(m_fLength * fNegativeFalloff, 0.0001f);
  const float fNegativeBias = fNegativeScale * m_fLength * 0.5f;

  const float fRadiusScale = -1.0f / WMath::Max(m_fRadius * m_fRadiusFalloff, 0.0001f);
  const float fRadiusBias = -fRadiusScale * m_fRadius;

  m_vScaleValues.Set(fPositiveScale, fNegativeScale, fRadiusScale, 0.0f);
  m_vBiasValues.Set(fPositiveBias, fNegativeBias, fRadiusBias, 0.0f);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_COMPONENT_TYPE(WWindVolumeConeComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(45)), new WClampValueAttribute(WAngle::MakeFromDegree(1), WAngle::MakeFromDegree(179))),
    W_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, WVariant())),
  }
  W_END_PROPERTIES;
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WConeVisualizerAttribute(WBasisAxis::PositiveX, "Angle", 1.0f, "Length", WColor::CornflowerBlue),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WWindVolumeConeComponent::WWindVolumeConeComponent() = default;
WWindVolumeConeComponent::~WWindVolumeConeComponent() = default;

void WWindVolumeConeComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void WWindVolumeConeComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

WSimdVec4f WWindVolumeConeComponent::ComputeForceAtLocalPosition(const WSimdVec4f& vLocalPos) const
{
  const WSimdFloat fConeDist = vLocalPos.x();

  if (fConeDist <= WSimdFloat::MakeZero() || fConeDist >= m_fLength)
    return WSimdVec4f::MakeZero();

  // TODO: precompute base radius
  const float fBaseRadius = WMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const WSimdFloat fConeRadius = (fConeDist / WSimdFloat(m_fLength)) * WSimdFloat(fBaseRadius);

  WSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return WSimdVec4f::MakeZero();

  return vLocalPos.GetNormalized<3>() * GetWindInMetersPerSecond();
}

void WWindVolumeConeComponent::SetLength(float fVal)
{
  m_fLength = WMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WWindVolumeConeComponent::SetAngle(WAngle val)
{
  m_Angle = WMath::Max(val, WAngle::MakeFromDegree(1.0f));

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void WWindVolumeConeComponent::OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg)
{
  WVec3 c0, c1;
  c0.x = 0;
  c0.y = -WMath::Tan(m_Angle * 0.5f) * m_fLength;
  c0.z = c0.y;

  c1.x = m_fLength;
  c1.y = WMath::Tan(m_Angle * 0.5f) * m_fLength;
  c1.z = c1.y;

  msg.AddBounds(WBoundingBoxSphere::MakeFromBox(WBoundingBox::MakeFromMinMax(c0, c1)), WWindVolumeComponent::SpatialDataCategory);
}


W_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_WindVolumeComponent);
