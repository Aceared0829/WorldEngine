#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/CreatureCrawlComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WCreatureLeg, WNoBase, 1, WRTTIDefaultAllocator<WCreatureLeg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LegName", m_sLegObject),
    W_MEMBER_PROPERTY("StepGroup", m_uiStepGroup),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WCreatureCrawlComponent, 1, WComponentMode::Dynamic);
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Body", DummyGetter, SetBodyReference)->AddAttributes(new WGameObjectReferenceAttribute()),
    W_MEMBER_PROPERTY("CastUp", m_fCastUp)->AddAttributes(new WDefaultValueAttribute(0.3f)),
    W_MEMBER_PROPERTY("CastDown", m_fCastDown)->AddAttributes(new WDefaultValueAttribute(1.0f)),
    W_MEMBER_PROPERTY("StepDistance", m_fStepDistance)->AddAttributes(new WDefaultValueAttribute(0.4f)),
    W_MEMBER_PROPERTY("MinLegDistance", m_fMinLegDistance)->AddAttributes(new WDefaultValueAttribute(0.5f), new WClampValueAttribute(0.1f, 1.0f)),
    W_ARRAY_MEMBER_PROPERTY("Legs", m_Legs),
  }
  W_END_PROPERTIES;

  W_BEGIN_ATTRIBUTES
  {
      new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WCreatureCrawlComponent::WCreatureCrawlComponent() = default;
WCreatureCrawlComponent::~WCreatureCrawlComponent() = default;

void WCreatureCrawlComponent::SetBodyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hBody = resolver(szReference, GetHandle(), "Body");
}

void WCreatureCrawlComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fCastUp;
  s << m_fCastDown;
  s << m_fStepDistance;
  s << m_fMinLegDistance;

  inout_stream.WriteGameObjectHandle(m_hBody);
  s << m_Legs.GetCount();
  for (WUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s << m_Legs[i].m_sLegObject;
    s << m_Legs[i].m_uiStepGroup;
  }
}

void WCreatureCrawlComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fCastUp;
  s >> m_fCastDown;
  s >> m_fStepDistance;
  s >> m_fMinLegDistance;

  m_hBody = inout_stream.ReadGameObjectHandle();
  WUInt32 numLegs = 0;
  s >> numLegs;
  m_Legs.SetCount(numLegs);
  for (WUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s >> m_Legs[i].m_sLegObject;
    s >> m_Legs[i].m_uiStepGroup;
  }
}

void WCreatureCrawlComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  const WTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

  for (auto& leg : m_Legs)
  {
    leg.m_vCurTargetPosAbs.SetZero();
    leg.m_fMoveLegFactor = 0.99f;
    leg.m_vRestPositionRelative.SetZero();
    leg.m_hLegObject.Invalidate();

    if (WGameObject* pLeg = GetOwner()->FindChildByName(leg.m_sLegObject))
    {
      leg.m_hLegObject = pLeg->GetHandle();
      leg.m_vRestPositionRelative = invTrans * pLeg->GetGlobalPosition();
    }
  }
}

void WCreatureCrawlComponent::Update()
{
  if (m_pPhysicsInterface == nullptr)
  {
    m_pPhysicsInterface = GetWorld()->GetModule<WPhysicsWorldModuleInterface>();
    return;
  }

  const WUInt32 uiNumLegs = m_Legs.GetCount();

  const WVec3 vCenterPos = GetOwner()->GetGlobalPosition();
  const WQuat qCenterRot = GetOwner()->GetGlobalRotation();

  WTempHybridArray<WVec3, 8> vNewTargetPos;
  vNewTargetPos.SetCount(uiNumLegs);

  WWorld* pWorld = GetWorld();

  // TODO: make step duration configurable
  // TODO: make step height configurable
  const WTime tStepDuration = WTime::MakeFromMilliseconds(150);
  const float fStepHeight = 0.3f;
  const float fMoveAdd = WMath::Min<float>(1.0f, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds() / tStepDuration.AsFloatInSeconds());

  WTempHybridArray<bool, 8> bLegMoving;
  bLegMoving.SetCount(uiNumLegs);
  bool bAnyLegMoving = false;

  for (WUInt32 i = 0; i < uiNumLegs; ++i)
  {
    vNewTargetPos[i] = qCenterRot * (m_Legs[i].m_vRestPositionRelative + WVec3(0, 0, m_fCastUp));
    bLegMoving[i] = m_Legs[i].m_fMoveLegFactor < 1.0f;
    bAnyLegMoving = bAnyLegMoving || bLegMoving[i];
  }

  bool bComputeBodyTilt = true;

  for (WUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (m_Legs[i].m_hLegObject.IsInvalidated())
      continue;

    // find a place where the leg can stand
    // if none found at the rest position, try pulling the leg closer to the body in two steps (unless forbidden, see m_fMinLegDistance)

    WPhysicsCastResult res;
    if (m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i], WVec3(0, 0, -1.0f), m_fCastDown, WPhysicsQueryParameters(0, WPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * WMath::Lerp(m_fMinLegDistance, 1.0f, 0.5f), WVec3(0, 0, -1.0f), m_fCastDown, WPhysicsQueryParameters(0, WPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * m_fMinLegDistance, WVec3(0, 0, -1.0f), m_fCastDown, WPhysicsQueryParameters(0, WPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else
    {
      bComputeBodyTilt = false;

      vNewTargetPos[i] += vCenterPos;
      m_Legs[i].m_vCurTargetPosAbs = WMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], fMoveAdd);
    }
  }

  WGameObject* pBody;
  if (!m_hBody.IsInvalidated() && pWorld->TryGetObject(m_hBody, pBody) && m_Legs.GetCount() == 4 /* TODO: remove safety check*/)
  {
    const WQuat qOwnerRot = GetOwner()->GetGlobalRotation();

    WQuat qNewBodyTilt;

    if (bComputeBodyTilt)
    {
      // TODO: compute body tilt from N points:
      // const WTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

      // WVec3 vAvgDir(0);

      // for (WUInt32 i = 0; i < uiNumLegs; ++i)
      //{
      //   WVec3 vDir = invTrans * vNewTargetPos[i];
      //   vAvgDir += vDir;
      // }

      WVec3 vAvgLegLeft = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[2]) * 0.5f);
      WVec3 vAvgLegRight = qOwnerRot.GetInverse() * ((vNewTargetPos[1] + vNewTargetPos[3]) * 0.5f);
      vAvgLegLeft.x = 0.0f;
      vAvgLegRight.x = 0.0f;

      WVec3 vAvgLegFwd = qOwnerRot.GetInverse() * ((vNewTargetPos[2] + vNewTargetPos[3]) * 0.5f);
      WVec3 vAvgLegBack = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[1]) * 0.5f);
      vAvgLegFwd.y = 0.0f;
      vAvgLegBack.y = 0.0f;

      const WVec3 vSideTilt = (vAvgLegRight - vAvgLegLeft).GetNormalized();
      const WQuat qSideTilt = WQuat::MakeShortestRotation(WVec3(0, 1, 0), vSideTilt);

      const WVec3 vFwdTilt = (vAvgLegFwd - vAvgLegBack).GetNormalized();
      const WQuat qFwdTilt = WQuat::MakeShortestRotation(WVec3(1, 0, 0), vFwdTilt);

      qNewBodyTilt = qSideTilt * qFwdTilt;
    }
    else
    {
      qNewBodyTilt = WQuat::MakeIdentity();
    }

    m_qBodyTilt = WQuat::MakeSlerp(m_qBodyTilt, qNewBodyTilt, fMoveAdd);
    pBody->SetGlobalRotation(qOwnerRot * m_qBodyTilt);

    pBody->UpdateGlobalTransform();
  }

  if (!bAnyLegMoving)
  {
    float fMaxDistSqr = 0;
    WUInt32 uiStepGroup = WInvalidIndex;

    // check whether to move a leg
    for (WUInt32 i = 0; i < uiNumLegs; ++i)
    {
      const float ds = (m_Legs[i].m_vCurTargetPosAbs - vNewTargetPos[i]).GetLengthSquared();

      if (ds > fMaxDistSqr)
      {
        fMaxDistSqr = ds;
        uiStepGroup = m_Legs[i].m_uiStepGroup;
      }
    }

    const WTime tNow = pWorld->GetClock().GetAccumulatedTime();

    // TODO: make step delay configurable ?

    if (fMaxDistSqr >= WMath::Square(m_fStepDistance) ||
        (fMaxDistSqr >= WMath::Square(m_fStepDistance * 0.25) && (tNow - m_LastMove > WTime::MakeFromSeconds(0.6f))))
    {
      m_LastMove = tNow;

      for (WUInt32 i = 0; i < uiNumLegs; ++i)
      {
        if (m_Legs[i].m_uiStepGroup == uiStepGroup)
        {
          // move all legs from the same group simultaneously
          m_Legs[i].m_fMoveLegFactor = 0.0f;
        }
      }
    }
  }

  for (WUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (bLegMoving[i])
    {
      // TODO: nicer step curve (sine instead of linear)

      m_Legs[i].m_fMoveLegFactor += fMoveAdd;

      if (m_Legs[i].m_fMoveLegFactor >= 1.0f)
      {
        m_Legs[i].m_fMoveLegFactor = 1.0f;
        m_Legs[i].m_vCurTargetPosAbs = vNewTargetPos[i];
      }
      else
      {
        float fSrcHeight = m_Legs[i].m_vCurTargetPosAbs.z;
        float fDstHeight = vNewTargetPos[i].z;
        float fMidHeight = WMath::Max(fSrcHeight, fDstHeight) + fStepHeight;

        if (m_Legs[i].m_fMoveLegFactor < 0.5f)
        {
          vNewTargetPos[i].z = WMath::Lerp(m_Legs[i].m_vCurTargetPosAbs.z, fMidHeight, m_Legs[i].m_fMoveLegFactor * 2.0f);
        }
        else
        {
          vNewTargetPos[i].z = WMath::Lerp(fMidHeight, vNewTargetPos[i].z, (m_Legs[i].m_fMoveLegFactor - 0.5f) * 2.0f);
        }
      }

      vNewTargetPos[i] = WMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], m_Legs[i].m_fMoveLegFactor);
    }
    else
    {
      vNewTargetPos[i] = m_Legs[i].m_vCurTargetPosAbs;
    }

    WGameObject* pLegTarget;
    if (pWorld->TryGetObject(m_Legs[i].m_hLegObject, pLegTarget))
    {
      pLegTarget->SetGlobalPosition(vNewTargetPos[i]);
    }
  }
}


W_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Animation_Implementation_CreatureCrawlComponent);
