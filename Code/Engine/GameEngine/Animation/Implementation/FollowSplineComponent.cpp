#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Animation/FollowSplineComponent.h>
#include <RendererCore/Components/SplineComponent.h>

#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WFollowSplineMode, 1)
  W_ENUM_CONSTANTS(WFollowSplineMode::OnlyPosition, WFollowSplineMode::AlignUpZ, WFollowSplineMode::FullRotation)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WFollowSplineComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Spline", DummyGetter, SetSplineObject)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_ACCESSOR_PROPERTY("StartDistance", GetStartDistance, SetStartDistance)->AddAttributes(new WClampValueAttribute(0.0f, {})),
    W_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new WDefaultValueAttribute(true)), // Whether the animation should start right away.
    W_ENUM_MEMBER_PROPERTY("Mode", WPropertyAnimMode, m_Mode),
    W_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("LookAhead", m_fLookAhead)->AddAttributes(new WClampValueAttribute(0.0f, 10.0f)),
    W_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ENUM_MEMBER_PROPERTY("FollowMode", WFollowSplineMode, m_FollowMode),
    W_MEMBER_PROPERTY("TiltAmount", m_fTiltAmount)->AddAttributes(new WDefaultValueAttribute(5.0f)),
    W_MEMBER_PROPERTY("MaxTilt", m_MaxTilt)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(30.0f)), new WClampValueAttribute(WAngle::MakeFromDegree(0.0f), WAngle::MakeFromDegree(90.0f))),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetCurrentDistance, In, "Distance"),
    W_SCRIPT_FUNCTION_PROPERTY(GetCurrentDistance),
    W_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    W_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    W_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WFollowSplineComponent::WFollowSplineComponent() = default;
WFollowSplineComponent::~WFollowSplineComponent() = default;

void WFollowSplineComponent::SerializeComponent(WWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  ref_stream.WriteGameObjectHandle(m_hSplineObject);

  s << m_fStartDistance;
  s << m_fSpeed;
  s << m_fLookAhead;
  s << m_Mode;
  s << m_fSmoothing;
  s << m_bIsRunning;
  s << m_bIsRunningForwards;
  s << m_FollowMode;
  s << m_fTiltAmount;
  s << m_MaxTilt;
}

void WFollowSplineComponent::DeserializeComponent(WWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  m_hSplineObject = ref_stream.ReadGameObjectHandle();

  s >> m_fStartDistance;
  s >> m_fSpeed;
  s >> m_fLookAhead;
  s >> m_Mode;
  s >> m_fSmoothing;
  s >> m_bIsRunning;
  s >> m_bIsRunningForwards;
  s >> m_FollowMode;
  s >> m_fTiltAmount;
  s >> m_MaxTilt;
}

void WFollowSplineComponent::OnActivated()
{
  SUPER::OnActivated();

  SetCurrentDistance(m_fStartDistance);
}

void WFollowSplineComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // if no spline reference was set, search the parent objects for a spline
  if (m_hSplineObject.IsInvalidated())
  {
    WGameObject* pParent = GetOwner()->GetParent();
    while (pParent != nullptr)
    {
      WSplineComponent* pSpline = nullptr;
      if (pParent->TryGetComponentOfBaseType(pSpline))
      {
        m_hSplineObject = pSpline->GetOwner()->GetHandle();
        break;
      }

      pParent = pParent->GetParent();
    }
  }

  SetCurrentDistance(m_fStartDistance);
}

void WFollowSplineComponent::SetSplineObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hSplineObject = resolver(szReference, GetHandle(), "Spline");
}

void WFollowSplineComponent::SetStartDistance(float fDistance)
{
  m_bLastStateValid = false;
  m_fStartDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    SetCurrentDistance(m_fStartDistance);
  }
}

void WFollowSplineComponent::SetCurrentDistance(float fDistance)
{
  m_fCurrentDistance = WMath::Max(fDistance, 0.0f);

  if (IsActiveAndInitialized())
  {
    WGameObject* pSplineObject = nullptr;
    if (!GetWorld()->TryGetObject(m_hSplineObject, pSplineObject))
      return;

    WSplineComponent* pSplineComponent;
    if (!pSplineObject->TryGetComponentOfBaseType(pSplineComponent))
      return;

    m_fCurrentDistance = WMath::Min(m_fCurrentDistance, pSplineComponent->GetTotalLength());

    Update(true);
  }
}

void WFollowSplineComponent::SetRunning(bool b)
{
  m_bIsRunning = b;
}

void WFollowSplineComponent::SetDirectionForwards(bool bForwards)
{
  m_bIsRunningForwards = bForwards;
}

void WFollowSplineComponent::ToggleDirection()
{
  m_bIsRunningForwards = !m_bIsRunningForwards;
}

void WFollowSplineComponent::Update(bool bForce)
{
  if (!bForce && (!m_bIsRunning || m_fSpeed == 0.0f))
    return;

  if (m_hSplineObject.IsInvalidated())
    return;

  WWorld* pWorld = GetWorld();

  WGameObject* pSplineObject = nullptr;
  if (!pWorld->TryGetObject(m_hSplineObject, pSplineObject))
  {
    // no need to retry this again
    m_hSplineObject.Invalidate();
    return;
  }

  WSplineComponent* pSplineComponent;
  if (!pSplineObject->TryGetComponentOfBaseType(pSplineComponent))
    return;

  auto& clock = pWorld->GetClock();

  float fToAdvance = m_fSpeed * clock.GetTimeDiff().AsFloatInSeconds();

  if (!m_bIsRunningForwards)
  {
    fToAdvance = -fToAdvance;
  }

  if (fToAdvance != 0.0f)
  {
    const float fTotalLength = pSplineComponent->GetTotalLength();

    bool bReachedEnd = false;
    const float fNewDistance = m_fCurrentDistance + fToAdvance;
    if (fToAdvance > 0.0f && fNewDistance >= fTotalLength)
    {
      bReachedEnd = true;
      m_fCurrentDistance = fTotalLength;
      fToAdvance = fNewDistance - fTotalLength;
    }
    else if (fToAdvance < 0.0f && fNewDistance <= 0.0f)
    {
      bReachedEnd = true;
      m_fCurrentDistance = 0.0f;
      fToAdvance = fNewDistance;
    }
    else
    {
      m_fCurrentDistance = fNewDistance;
    }

    if (bReachedEnd)
    {
      WMsgAnimationReachedEnd msg;
      m_ReachedEndEvent.SendEventMessage(msg, this, GetOwner());

      if (m_Mode == WPropertyAnimMode::Loop)
      {
        m_fCurrentDistance = fToAdvance;
      }
      else if (m_Mode == WPropertyAnimMode::BackAndForth)
      {
        m_bIsRunningForwards = !m_bIsRunningForwards;
        fToAdvance = -fToAdvance;
        m_fCurrentDistance += fToAdvance;
      }
      else
      {
        m_bIsRunning = false;
      }
    }
  }

  const float fKey = pSplineComponent->GetKeyAtDistance(m_fCurrentDistance);
  WVec3 vPosition = pSplineComponent->GetPositionAtKey(fKey);
  WVec3 vUpDir = pSplineComponent->GetUpDirAtKey(fKey);

  WVec3 vForwardDir;
  if (m_fLookAhead > 0.0f)
  {
    float fLookAhead = WMath::Max(m_fLookAhead, 0.02f);
    float fLookAheadDistance = m_fCurrentDistance + fLookAhead;
    if (fLookAheadDistance > pSplineComponent->GetTotalLength() && m_Mode == WPropertyAnimMode::Loop)
    {
      fLookAheadDistance -= pSplineComponent->GetTotalLength();
    }

    const WVec3 vLookAheadPosition = pSplineComponent->GetPositionAtDistance(fLookAheadDistance);
    vForwardDir = vLookAheadPosition - vPosition;
  }
  else
  {
    vForwardDir = pSplineComponent->GetForwardDirAtKey(fKey);
  }

  if (m_bLastStateValid)
  {
    const float fSmoothing = WMath::Clamp(m_fSmoothing, 0.0f, 0.99f);

    vPosition = WMath::Lerp(vPosition, m_vLastPosition, fSmoothing);
    vUpDir = WMath::Lerp(vUpDir, m_vLastUpDir, fSmoothing);
    vForwardDir = WMath::Lerp(vForwardDir, m_vLastForwardDir, fSmoothing);
  }

  if (m_FollowMode == WFollowSplineMode::AlignUpZ)
  {
    const WPlane plane = WPlane::MakeFromNormalAndPoint(WVec3::MakeAxisZ(), vPosition);
    vForwardDir = plane.GetCoplanarDirection(vForwardDir);
  }
  vForwardDir.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();

  vUpDir = (m_FollowMode == WFollowSplineMode::FullRotation) ? vUpDir : WVec3::MakeAxisZ();
  WVec3 vRightDir = vUpDir.CrossRH(vForwardDir);
  vRightDir.NormalizeIfNotZero(WVec3::MakeAxisY()).IgnoreResult();

  vUpDir = vForwardDir.CrossRH(vRightDir);
  vUpDir.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

  // check if we want to tilt the platform when turning
  WAngle deltaAngle = WAngle::MakeFromDegree(0.0f);
  if (m_FollowMode == WFollowSplineMode::AlignUpZ && !WMath::IsZero(m_fTiltAmount, 0.0001f) && !WMath::IsZero(m_MaxTilt.GetDegree(), 0.0001f))
  {
    if (m_bLastStateValid)
    {
      WVec3 vLastForwardDir = m_vLastForwardDir;
      {
        const WPlane plane = WPlane::MakeFromNormalAndPoint(WVec3::MakeAxisZ(), vPosition);
        vLastForwardDir = plane.GetCoplanarDirection(vLastForwardDir);
        vLastForwardDir.NormalizeIfNotZero(WVec3::MakeAxisX()).IgnoreResult();
      }

      const float fTiltStrength = WMath::Sign((vLastForwardDir - vForwardDir).Dot(vRightDir)) * WMath::Sign(m_fTiltAmount);
      WAngle tiltAngle = WMath::Min(vLastForwardDir.GetAngleBetween(vForwardDir) * WMath::Abs(m_fTiltAmount), m_MaxTilt);
      deltaAngle = WMath::Lerp(tiltAngle * fTiltStrength, m_LastTiltAngle, 0.85f); // this smooths out the tilting from being jittery

      WQuat rot = WQuat::MakeFromAxisAndAngle(vForwardDir, deltaAngle);
      vUpDir = rot * vUpDir;
      vRightDir = rot * vRightDir;
    }
  }

  {
    m_bLastStateValid = true;
    m_vLastPosition = vPosition;
    m_vLastForwardDir = vForwardDir;
    m_vLastUpDir = vUpDir;
    m_LastTiltAngle = deltaAngle;
  }

  WMat3 mRot = WMat3::MakeIdentity();
  if (m_FollowMode != WFollowSplineMode::OnlyPosition)
  {
    mRot.SetColumn(0, vForwardDir);
    mRot.SetColumn(1, vRightDir);
    mRot.SetColumn(2, vUpDir);
  }

  WTransform tFinal;
  tFinal.m_vPosition = vPosition;
  tFinal.m_vScale = GetOwner()->GetLocalScaling() * GetOwner()->GetLocalUniformScaling();
  tFinal.m_qRotation = WQuat::MakeFromMat3(mRot);

  GetOwner()->SetGlobalTransform(pSplineObject->GetGlobalTransform() * tFinal);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WFollowPathComponentPatch_1_2 : public WGraphPatch
{
public:
  WFollowPathComponentPatch_1_2()
    : WGraphPatch("WFollowPathComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WFollowSplineComponent");

    pNode->RenameProperty("Path", "Spline");

    auto* pFollowMode = pNode->FindProperty("FollowMode");
    if (pFollowMode && pFollowMode->m_Value.IsA<WString>())
    {
      WStringBuilder sFollowMode = pFollowMode->m_Value.Get<WString>();
      sFollowMode.ReplaceAll("Path", "Spline");
      pNode->ChangeProperty("FollowMode", sFollowMode.GetView());
    }
  }
};

WFollowPathComponentPatch_1_2 g_WFollowPathComponentPatch_1_2;


W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_FollowSplineComponent);
