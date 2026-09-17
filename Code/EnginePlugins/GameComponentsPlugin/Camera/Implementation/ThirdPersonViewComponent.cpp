#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameComponentsPlugin/Camera/ThirdPersonViewComponent.h>

WThirdPersonViewComponentManager::WThirdPersonViewComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

WThirdPersonViewComponentManager::~WThirdPersonViewComponentManager() = default;

void WThirdPersonViewComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WThirdPersonViewComponentManager::Update, this);
    desc.m_Phase = WWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void WThirdPersonViewComponentManager::Update(const WWorldModule::UpdateContext& context)
{
  // Iterate through all components managed by this manager and update them
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}


// clang-format off
W_BEGIN_COMPONENT_TYPE(WThirdPersonViewComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("TargetObject", GetTargetObject, SetTargetObject)->AddAttributes(new WDefaultValueAttribute(WStringView(".."))),
    W_MEMBER_PROPERTY("TargetOffsetHigh", m_vTargetOffsetHigh),
    W_MEMBER_PROPERTY("TargetOffsetLow", m_vTargetOffsetLow),
    W_MEMBER_PROPERTY("MinDistance", m_fMinDistance)->AddAttributes(new WDefaultValueAttribute(0.25f), new WClampValueAttribute(0.05f, 10.0f)),
    W_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new WDefaultValueAttribute(3.0f), new WClampValueAttribute(0.5f, 100.0f)),
    W_MEMBER_PROPERTY("MaxDistanceUp", m_fMaxDistanceUp)->AddAttributes(new WDefaultValueAttribute(3.0f), new WClampValueAttribute(0.5f, 100.0f)),
    W_MEMBER_PROPERTY("MaxDistanceDown", m_fMaxDistanceDown)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.5f, 100.0f)),
    W_MEMBER_PROPERTY("MinUpRotation", m_MinUpRotation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(-70)), new WClampValueAttribute(WAngle::MakeFromDegree(-85), WAngle::MakeFromDegree(70))),
    W_MEMBER_PROPERTY("MaxUpRotation", m_MaxUpRotation)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(+80)), new WClampValueAttribute(WAngle::MakeFromDegree(-70), WAngle::MakeFromDegree(85))),
    W_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new WDynamicEnumAttribute("PhysicsCollisionLayer")),
    W_MEMBER_PROPERTY("SweepWidth", m_fSweepWidth)->AddAttributes(new WDefaultValueAttribute(0.4f), new WClampValueAttribute(0.05f, 2.0f)),
    W_MEMBER_PROPERTY("ZoomInSpeed", m_fZoomInSpeed)->AddAttributes(new WDefaultValueAttribute(10.0f), new WClampValueAttribute(0.01f, 1000.0f)),
    W_MEMBER_PROPERTY("ZoomOutSpeed", m_fZoomOutSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.01f, 1000.0f)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Gameplay"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(RotateUp, In, "angle"),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE;
// clang-format on

WThirdPersonViewComponent::WThirdPersonViewComponent() = default;
WThirdPersonViewComponent::~WThirdPersonViewComponent() = default;

void WThirdPersonViewComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // version 1
  s << m_sTargetObject;
  s << m_vTargetOffsetHigh;
  s << m_vTargetOffsetLow;
  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_MinUpRotation;
  s << m_MaxUpRotation;
  s << m_fMaxDistanceUp;
  s << m_fMaxDistanceDown;
  s << m_uiCollisionLayer;
  s << m_fSweepWidth;
  s << m_fZoomInSpeed;
  s << m_fZoomOutSpeed;
}

void WThirdPersonViewComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // version 1
  s >> m_sTargetObject;
  s >> m_vTargetOffsetHigh;
  s >> m_vTargetOffsetLow;
  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_MinUpRotation;
  s >> m_MaxUpRotation;
  s >> m_fMaxDistanceUp;
  s >> m_fMaxDistanceDown;
  s >> m_uiCollisionLayer;
  s >> m_fSweepWidth;
  s >> m_fZoomInSpeed;
  s >> m_fZoomOutSpeed;
}

void WThirdPersonViewComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_CurUpRotation = WMath::Lerp(m_MinUpRotation, m_MaxUpRotation, 0.5f);

  SetTargetObject(m_sTargetObject);
}

void WThirdPersonViewComponent::SetTargetObject(const char* szTargetObject)
{
  m_sTargetObject = szTargetObject;
  m_hTargetObject.Invalidate();

  if (IsActiveAndSimulating() && !m_sTargetObject.IsEmpty())
  {
    if (WGameObject* pTargetObject = GetWorld()->SearchForObject(m_sTargetObject, GetOwner()))
    {
      m_hTargetObject = pTargetObject->GetHandle();
    }
  }
}

const char* WThirdPersonViewComponent::GetTargetObject() const
{
  return m_sTargetObject;
}

void WThirdPersonViewComponent::RotateUp(WAngle angle)
{
  m_RotateUp += angle;
}

void WThirdPersonViewComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  WGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    return;

  const WTransform tTarget = pTarget->GetGlobalTransform();

  m_CurUpRotation += m_RotateUp;
  m_CurUpRotation = WMath::Clamp(m_CurUpRotation, m_MinUpRotation, m_MaxUpRotation);
  m_RotateUp = {};

  float fCurMaxDistance = m_fMaxDistance;

  const WVec3 vOffsetCenter = WMath::Lerp(m_vTargetOffsetLow, m_vTargetOffsetHigh, 0.5f);
  WVec3 vOffset = vOffsetCenter;
  if (m_CurUpRotation.GetRadian() > 0)
  {
    const float fLerp = WMath::Unlerp(0.0f, m_MaxUpRotation.GetRadian(), m_CurUpRotation.GetRadian());
    vOffset = WMath::Lerp(vOffsetCenter, m_vTargetOffsetHigh, fLerp);
    fCurMaxDistance = WMath::Lerp(m_fMaxDistance, m_fMaxDistanceUp, fLerp);
  }
  else
  {
    const float fLerp = WMath::Unlerp(0.0f, m_MinUpRotation.GetRadian(), m_CurUpRotation.GetRadian());
    vOffset = WMath::Lerp(vOffsetCenter, m_vTargetOffsetLow, fLerp);
    fCurMaxDistance = WMath::Lerp(m_fMaxDistance, m_fMaxDistanceDown, fLerp);
  }

  const WVec3 vLookAtPos = tTarget.m_vPosition + tTarget.m_qRotation * vOffset;

  const WQuat qRotUp = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisY(), m_CurUpRotation);
  const WQuat qRotTotal = tTarget.m_qRotation * qRotUp;

  const WVec3 vSweepDir = qRotTotal * WVec3(-1, 0, 0);
  float fNewDistance = fCurMaxDistance;

  if (WPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<WPhysicsWorldModuleInterface>())
  {
    const WTime tDiff = GetWorld()->GetClock().GetTimeDiff();

    WPhysicsCastResult res1;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = WPhysicsShapeType::Static;
    params.m_uiCollisionLayer = m_uiCollisionLayer;
    if (pPhysics->SweepTestSphere(res1, m_fSweepWidth * 0.5f, vLookAtPos, vSweepDir, fNewDistance, params))
    {
      fNewDistance = WMath::Max(m_fMinDistance, res1.m_fDistance);
    }

    if (m_fCurDistance > fNewDistance)
    {
      m_fCurDistance = WMath::Lerp(m_fCurDistance, fNewDistance, WMath::Min(1.0f, tDiff.AsFloatInSeconds() * m_fZoomInSpeed));
    }
    else
    {
      m_fCurDistance = WMath::Lerp(m_fCurDistance, fNewDistance, WMath::Min(1.0f, tDiff.AsFloatInSeconds() * m_fZoomOutSpeed));
    }
  }

  const WVec3 vLookFromPos = vLookAtPos + m_fCurDistance * vSweepDir;
  GetOwner()->SetGlobalTransformToLookAt(vLookFromPos, vLookAtPos);
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Camera_Implementation_ThirdPersonViewComponent);
