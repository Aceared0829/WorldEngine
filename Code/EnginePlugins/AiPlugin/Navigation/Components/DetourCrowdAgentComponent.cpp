#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/DetourCrowdAgentComponent.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <AiPlugin/Utils/RcMath.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <DetourCrowd.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>

WCVarBool cvar_DetourCrowdVisAgents("DetourCrowd.Debug.VisAgents", false, WCVarFlags::Default, "Draws DetourCrowd agents, if any");
WCVarBool cvar_DetourCrowdVisCorners("DetourCrowd.Debug.VisCorners", false, WCVarFlags::Default, "Draws next few path corners of the DetourCrowd agents");
WCVarBool cvar_DetourCrowdVisDestination("DetourCrowd.Debug.VisDestination", false, WCVarFlags::Default, "Draws destination points of the DetourCrowd agents");

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WDetourCrowdAgentRotationMode, 1)
  W_ENUM_CONSTANTS(WDetourCrowdAgentRotationMode::LookAtNextPathCorner, WDetourCrowdAgentRotationMode::MatchVelocityDirection)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_COMPONENT_TYPE(WDetourCrowdAgentComponent, 1, WComponentMode::Dynamic)
{
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;

  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new WDynamicStringEnumAttribute("AiNavmeshConfig")),
    W_MEMBER_PROPERTY("Radius",m_fRadius)->AddAttributes(new WDefaultValueAttribute(0.3f),new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("Height",m_fHeight)->AddAttributes(new WDefaultValueAttribute(1.8f),new WClampValueAttribute(0.01f, WVariant())),
    W_MEMBER_PROPERTY("MaxSpeed",m_fMaxSpeed)->AddAttributes(new WDefaultValueAttribute(3.5f),new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MaxAcceleration",m_fMaxAcceleration)->AddAttributes(new WDefaultValueAttribute(10.0f),new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("StoppingDistance",m_fStoppingDistance)->AddAttributes(new WDefaultValueAttribute(0.3f),new WClampValueAttribute(0.001f, WVariant())),
    W_MEMBER_PROPERTY("MaxAngularSpeed",m_MaxAngularSpeed)->AddAttributes(new WDefaultValueAttribute(WAngle::MakeFromDegree(360.0f)),new WClampValueAttribute(0.0f, WVariant())),
    W_ENUM_MEMBER_PROPERTY("RotationMode",WDetourCrowdAgentRotationMode,m_RotationMode),
    W_MEMBER_PROPERTY("Pushiness",m_fPushiness)->AddAttributes(new WDefaultValueAttribute(1.0f),new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;

  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination", In, "AllowPartialPaths"),
    W_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
    W_SCRIPT_FUNCTION_PROPERTY(HasDestination),
  }
  W_END_FUNCTIONS;
}
W_END_COMPONENT_TYPE
// clang-format on

WDetourCrowdAgentComponent::WDetourCrowdAgentComponent()
{
  m_uiHasDestinationBit = 0;
  m_uiDestinationChangedBit = 0;
  m_uiSteeringFailedBit = 0;
  m_uiParamsChangedBit = 0;
  m_uiAllowPartialPathBit = 0;
}

WDetourCrowdAgentComponent::~WDetourCrowdAgentComponent() = default;

void WDetourCrowdAgentComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  WStreamWriter& s = stream.GetStream();

  s << m_sNavmeshConfig;
  s << m_fRadius;
  s << m_fHeight;
  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_fStoppingDistance;
  s << m_MaxAngularSpeed;
  s << m_RotationMode;
  s << m_fPushiness;
}

void WDetourCrowdAgentComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = stream.GetStream();

  s >> m_sNavmeshConfig;
  s >> m_fRadius;
  s >> m_fHeight;
  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_fStoppingDistance;
  s >> m_MaxAngularSpeed;
  s >> m_RotationMode;
  s >> m_fPushiness;
}

void WDetourCrowdAgentComponent::SetRadius(float fRadius)
{
  m_fRadius = WMath::Max(fRadius, 0.0f);
  m_uiParamsChangedBit = 1;
}

void WDetourCrowdAgentComponent::SetHeight(float fHeight)
{
  m_fHeight = WMath::Max(fHeight, 0.01f);
  m_uiParamsChangedBit = 1;
}

void WDetourCrowdAgentComponent::SetMaxSpeed(float fMaxSpeed)
{
  m_fMaxSpeed = WMath::Max(fMaxSpeed, 0.0f);
  m_uiParamsChangedBit = 1;
}

void WDetourCrowdAgentComponent::SetMaxAcceleration(float fMaxAcceleration)
{
  m_fMaxAcceleration = WMath::Max(fMaxAcceleration, 0.0f);
  m_uiParamsChangedBit = 1;
}

void WDetourCrowdAgentComponent::SetStoppingDistance(float fStoppingDistance)
{
  m_fStoppingDistance = WMath::Max(fStoppingDistance, 0.001f);
}

void WDetourCrowdAgentComponent::SetMaxAngularSpeed(WAngle maxAngularSpeed)
{
  if (maxAngularSpeed.GetRadian() < 0.0f)
    maxAngularSpeed.SetRadian(0.0f);

  m_MaxAngularSpeed = maxAngularSpeed;
}

void WDetourCrowdAgentComponent::SetPushiness(float fPushiness)
{
  m_fPushiness = WMath::Max(fPushiness, 0.0f);
  m_uiParamsChangedBit = 1;
}

void WDetourCrowdAgentComponent::SetDestination(const WVec3& vGlobalPos, bool bAllowPartialPath)
{
  auto* pNavMeshModule = GetWorld()->GetOrCreateModule<WAiNavMeshWorldModule>();
  auto* pNavMesh = pNavMeshModule->GetNavMesh(m_sNavmeshConfig);

  if (pNavMesh)
  {
    m_uiAllowPartialPathBit = bAllowPartialPath ? 1 : 0;
    m_uiSteeringFailedBit = 0;
    m_vDestination = vGlobalPos;
    m_uiDestinationChangedBit = 1;
    m_uiHasDestinationBit = 1;

    WRectFloat r = WRectFloat::MakeInvalid();
    r.ExpandToInclude(GetOwner()->GetGlobalPosition().GetAsVec2());
    r.ExpandToInclude(vGlobalPos.GetAsVec2());
    r.Grow(WAiNavigation::c_fPathSearchBoundary);

    pNavMesh->RequestSector(r.GetCenter(), r.GetHalfExtents());
  }
  else
  {
    WLog::Error("NavMesh '{}' does not exist (referenced by '{}')", m_sNavmeshConfig, GetOwner()->GetName());
  }
}

void WDetourCrowdAgentComponent::CancelNavigation()
{
  m_uiDestinationChangedBit = m_uiHasDestinationBit;
  m_uiHasDestinationBit = 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WDetourCrowdAgentComponentManager::WDetourCrowdAgentComponentManager(WWorld* pWorld)
  : SUPER(pWorld)
{
}
WDetourCrowdAgentComponentManager::~WDetourCrowdAgentComponentManager() = default;

void WDetourCrowdAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WDetourCrowdAgentComponentManager::AsyncUpdate, this);
    desc.m_Phase = WWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WDetourCrowdAgentComponentManager::SyncTransforms, this);
    desc.m_Phase = WWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }
}

void WDetourCrowdAgentComponentManager::FillDtCrowdAgentParams(const WDetourCrowdAgentComponent* pAgent, dtCrowdAgentParams& out_params)
{
  out_params.radius = pAgent->GetRadius();
  out_params.height = pAgent->GetHeight();
  out_params.maxAcceleration = pAgent->GetMaxAcceleration();
  out_params.maxSpeed = pAgent->GetMaxSpeed();
  out_params.collisionQueryRange = WMath::Max(1.2f, 12.0f * out_params.radius);
  out_params.pathOptimizationRange = WMath::Max(3.0f, 30.0f * out_params.radius);
  out_params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
  out_params.obstacleAvoidanceType = 3;
  out_params.separationWeight = pAgent->GetPushiness();
  out_params.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(pAgent->m_uiUniqueAgentId));
}

dtCrowdAgent* WDetourCrowdAgentComponentManager::TryGetValidDtAgent(dtCrowd* pDtCrowd, const WDetourCrowdAgentComponent* pAgent)
{
  dtCrowdAgent* pDtAgent = pAgent->m_uiAgentId ? pDtCrowd->getEditableAgent(static_cast<int>(pAgent->m_uiAgentId - 1)) : nullptr;

  if (pDtAgent && pDtAgent->active && static_cast<WUInt32>(reinterpret_cast<std::uintptr_t>(pDtAgent->params.userData)) == pAgent->m_uiUniqueAgentId)
    return pDtAgent;

  return nullptr;
}

void WDetourCrowdAgentComponentManager::AsyncUpdate(const UpdateContext& ctx)
{
  auto* pNavMeshModule = GetWorld()->GetModuleReadOnly<WAiNavMeshWorldModule>();
  if (!pNavMeshModule)
    return;

  // For each dtCrowd, check if the underlying navmesh has changed
  for (auto pair : m_CrowdPerNavMesh)
  {
    const WAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(pair.key);
    if (!pNavMesh)
      continue;

    if (pNavMesh->GetDetourNavMesh() != pair.value->getNavMeshQuery()->getAttachedNavMesh())
    {
      const WInt32 iMaxAgents = 128;
      const float fMaxAgentRadius = pNavMesh->GetConfig().m_fAgentRadius;

      pair.value->init(iMaxAgents, fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh->GetDetourNavMesh()));
    }
  }

  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    WDetourCrowdAgentComponent* pAgent = it;

    if (pAgent->IsActiveAndSimulating())
    {
      // Fetch or create a dtCrowd for agent's navmesh config
      dtCrowd* pDtCrowd = nullptr;
      if (pAgent->m_uiCrowdId)
      {
        pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      }
      else
      {
        WUInt32 uiCrowdIdx = m_CrowdPerNavMesh.Find(pAgent->m_sNavmeshConfig);
        if (uiCrowdIdx != WInvalidIndex)
        {
          pDtCrowd = m_CrowdPerNavMesh.GetValue(uiCrowdIdx);
          pAgent->m_uiCrowdId = uiCrowdIdx + 1;
        }
        else if (const WAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(pAgent->m_sNavmeshConfig))
        {
          pDtCrowd = dtAllocCrowd();

          const WInt32 iMaxAgents = 128;
          const float fMaxAgentRadius = pNavMesh->GetConfig().m_fAgentRadius;

          pDtCrowd->init(iMaxAgents, fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh->GetDetourNavMesh()));

          pAgent->m_uiCrowdId = m_CrowdPerNavMesh.Insert(pAgent->m_sNavmeshConfig, pDtCrowd) + 1;
        }
      }

      if (!pDtCrowd)
      {
        continue;
      }

      WInt32 iAgentId = static_cast<WInt32>(pAgent->m_uiAgentId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      // If an agent was created out of navmesh, which is entirely possible because we create navmesh on demand,
      // then it enters the invalid state. The only way to exit the invalid state is to recreate the agent.
      if (pDtAgent && pDtAgent->state == DT_CROWDAGENT_STATE_INVALID)
      {
        pDtCrowd->removeAgent(iAgentId);
        pDtAgent = nullptr;
        pAgent->m_uiHasDestinationBit = 0;
      }

      // If WAgent doesn't have a corresponding dtAgent, create one
      if (!pDtAgent)
      {
        pAgent->m_uiUniqueAgentId = ++m_uiNextUniqueId;

        dtCrowdAgentParams dtParams{};
        FillDtCrowdAgentParams(pAgent, dtParams);

        iAgentId = pDtCrowd->addAgent(WRcPos(pAgent->GetOwner()->GetGlobalPosition()), &dtParams);

        if (iAgentId == -1)
        {
          WLog::Warning("Couldn't create DetourCrowd agent for '{0}'. The component will be disabled.", pAgent->GetOwner()->GetName());
          pAgent->m_uiAgentId = 0;
          pAgent->SetActiveFlag(false);
          continue;
        }

        pDtAgent = pDtCrowd->getEditableAgent(iAgentId);

        pAgent->m_uiAgentId = iAgentId + 1;
        pAgent->m_uiParamsChangedBit = 0;
      }

      // Update dtAgent's parameters if any of the WAgent's properties (Height, Radius, etc) changed
      if (pAgent->m_uiParamsChangedBit)
      {
        pAgent->m_uiParamsChangedBit = 0;

        dtCrowdAgentParams dtParams{};
        FillDtCrowdAgentParams(pAgent, dtParams);

        pDtCrowd->updateAgentParameters(iAgentId, &dtParams);
      }

      if (pAgent->m_uiDestinationChangedBit)
      {
        pAgent->m_uiDestinationChangedBit = 0;

        if (pAgent->m_uiHasDestinationBit)
        {
          float vNavPos[3];
          dtPolyRef navPolyRef = 0;
          WVec3 vQueryHalfExtents = WVec3(1, 1, 2);

          // Grow search extents until a polygon is found, up to 3 attempts.
          for (int iTry = 0; iTry < 3 && navPolyRef == 0; ++iTry, vQueryHalfExtents *= 2.0f)
          {
            pDtCrowd->getNavMeshQuery()->findNearestPoly(WRcPos(pAgent->m_vDestination), WRcPos(vQueryHalfExtents), pDtCrowd->getFilter(0), &navPolyRef, vNavPos);
          }

          if (navPolyRef != 0)
          {
            pDtCrowd->requestMoveTarget(iAgentId, navPolyRef, vNavPos);
            pAgent->m_vActualDestination = WRcPos(pDtAgent->targetPos);
          }
          else
          {
            pDtCrowd->resetMoveTarget(iAgentId);
            pAgent->m_uiHasDestinationBit = 0;
            if (!pAgent->m_uiAllowPartialPathBit)
              pAgent->m_uiSteeringFailedBit = 1;
          }
        }
        else
        {
          pDtCrowd->resetMoveTarget(iAgentId);
        }
      }

      // Check if we've reached the destination
      if (pAgent->m_uiHasDestinationBit)
      {
        WVec3 vTargetPos = WRcPos(pDtAgent->targetPos);
        if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_VALID)
          vTargetPos = WRcPos(pDtAgent->corridor.getTarget());
        const float fDistSquared = vTargetPos.GetSquaredDistanceTo(WRcPos(pDtAgent->npos));

        if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
        {
          pDtCrowd->resetMoveTarget(iAgentId);
          pAgent->m_uiHasDestinationBit = 0;
          if (!pAgent->m_uiAllowPartialPathBit)
            pAgent->m_uiSteeringFailedBit = 1;
        }
        else if (fDistSquared < pAgent->m_fStoppingDistance * pAgent->m_fStoppingDistance)
        {
          pDtCrowd->resetMoveTarget(iAgentId);
          pAgent->m_uiHasDestinationBit = 0;
        }
      }
    }
    else if (pAgent->m_uiCrowdId && pAgent->m_uiAgentId)
    {
      // If WAgent is inactive, but still has a corresponding dtAgent, destroy the dtAgent

      dtCrowd* pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      if (pDtAgent)
      {
        pDtCrowd->removeAgent(static_cast<int>(pAgent->m_uiAgentId - 1));
      }

      pAgent->m_uiAgentId = 0;
    }
  }

  // Update each dtCrowd
  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  for (auto pair : m_CrowdPerNavMesh)
  {
    pair.value->update(fDeltaTime, nullptr);
  }
}

void WDetourCrowdAgentComponentManager::SyncTransforms(const UpdateContext& ctx)
{
  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // Sync each agent's position with corresponding dtAgent
  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    WDetourCrowdAgentComponent* pAgent = it;

    if (pAgent->IsActiveAndSimulating() && pAgent->m_uiCrowdId)
    {
      dtCrowd* pDtCrowd = m_CrowdPerNavMesh.GetValue(pAgent->m_uiCrowdId - 1);
      dtCrowdAgent* pDtAgent = TryGetValidDtAgent(pDtCrowd, pAgent);

      if (pDtAgent)
      {
        const WVec3 vPosition = WRcPos(pDtAgent->npos);
        const WVec3 vVelocity = WRcPos(pDtAgent->vel);

        WVec3 vLookDir = pAgent->GetOwner()->GetGlobalDirForwards();
        vLookDir.z = 0;
        vLookDir.Normalize();

        WVec3 vTargetDir = vVelocity;
        vTargetDir.z = 0;
        vTargetDir.NormalizeIfNotZero(vLookDir).IgnoreResult();

        if (pAgent->m_RotationMode == WDetourCrowdAgentRotationMode::LookAtNextPathCorner && pDtAgent->ncorners > 0)
        {
          WVec3 vNextCorner = WRcPos(pDtAgent->cornerVerts);
          WVec3 vDiff = vNextCorner - vPosition;
          vDiff.z = 0;

          if (vDiff.GetLengthSquared() > 0.001f)
          {
            vTargetDir = vDiff.GetNormalized();
          }
          else if (pDtAgent->ncorners > 1)
          {
            vNextCorner = WRcPos(pDtAgent->cornerVerts + 3);
            vDiff = vNextCorner - vPosition;
            vDiff.z = 0;
            if (vDiff.GetLengthSquared() > 0.001f)
            {
              vTargetDir = vDiff.GetNormalized();
            }
          }
        }

        const float maxTurnAngle = fDeltaTime * pAgent->m_MaxAngularSpeed.GetRadian();
        float turnAngle = vLookDir.GetAngleBetween(vTargetDir, WVec3::MakeAxisZ()).GetRadian();
        turnAngle = WMath::Sign(turnAngle) * WMath::Min(WMath::Abs(turnAngle), maxTurnAngle);

        const WQuat qRot = WQuat::MakeFromAxisAndAngle(WVec3::MakeAxisZ(), WAngle::MakeFromRadian(turnAngle));
        const WVec3 vNewLookDir = qRot * pAgent->GetOwner()->GetGlobalDirForwards();

        WTransform transform = pAgent->GetOwner()->GetGlobalTransform();
        transform.m_vPosition = vPosition;
        transform.m_qRotation = WQuat::MakeShortestRotation(WVec3::MakeAxisX(), vNewLookDir);
        pAgent->GetOwner()->SetGlobalTransform(transform);

        pAgent->m_vVelocity = vVelocity;
      }

      // Draw destination
      if (cvar_DetourCrowdVisDestination && pAgent->HasDestination())
      {
        WDebugRendererLine line{};
        line.m_start = pAgent->GetOwner()->GetGlobalPosition();
        line.m_end = pAgent->m_vActualDestination;

        WDebugRenderer::DrawLines(GetWorld(), WArrayPtr(&line, 1), WColor::DarkSalmon, WTransform::Make(WVec3(0.0f, 0.0f, 0.1f)));

        WDebugRenderer::DrawLineSphere(GetWorld(), WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), 0.2f), WColor::DarkSalmon, WTransform::Make(pAgent->m_vActualDestination));
      }
    }
  }

  // Debug visualization
  if (cvar_DetourCrowdVisAgents)
  {
    for (auto pair : m_CrowdPerNavMesh)
    {
      const WInt32 iNumAgents = pair.value->getAgentCount();
      for (int i = 0; i < iNumAgents; ++i)
      {
        const dtCrowdAgent* pDtAgent = pair.value->getAgent(i);
        if (pDtAgent->active)
        {
          const float fHeight = pDtAgent->params.height;
          const float fRadius = pDtAgent->params.radius;

          WTransform transform(WRcPos(pDtAgent->npos));
          transform.m_vPosition.z += fHeight * 0.5f;

          // Draw agent cylinder
          WDebugRenderer::DrawLineCylinderZ(GetWorld(), fHeight, fRadius, WColor::BlueViolet, transform);

          // Draw velocity arrow
          WVec3 vVelocity = WRcPos(pDtAgent->vel);
          vVelocity.z = 0;
          if (!vVelocity.IsZero())
          {
            vVelocity.Normalize();
            transform.m_qRotation = WQuat::MakeShortestRotation(WVec3(1, 0, 0), vVelocity);
            WDebugRenderer::DrawArrow(GetWorld(), 1.0f, WColor::BlueViolet, transform);
          }

          // Draw path corners
          if (cvar_DetourCrowdVisCorners.GetValue() && pDtAgent->ncorners > 0)
          {
            WDebugRendererLine lines[DT_CROWDAGENT_MAX_CORNERS];

            lines[0].m_start = WRcPos(pDtAgent->npos);
            lines[0].m_end = WRcPos(pDtAgent->cornerVerts);

            for (int i = 0; i < pDtAgent->ncorners - 1; ++i)
            {
              lines[i].m_start = WRcPos(pDtAgent->cornerVerts + 3 * i);
              lines[i].m_end = WRcPos(pDtAgent->cornerVerts + 3 * i + 3);
            }

            WDebugRenderer::DrawLines(GetWorld(), WArrayPtr(lines, pDtAgent->ncorners), WColor::Cyan, WTransform::Make(WVec3(0.0f, 0.0f, 0.1f)));
          }
        }
      }
    }
  }
}


W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Components_DetourCrowdAgentComponent);
