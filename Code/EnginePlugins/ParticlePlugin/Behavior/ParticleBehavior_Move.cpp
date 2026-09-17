#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Move.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WMovementMode, 1)
  W_ENUM_CONSTANT(WMovementMode::Constant),
  W_ENUM_CONSTANT(WMovementMode::CustomCurve),
  W_ENUM_CONSTANT(WMovementMode::SharedCurve),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehaviorFactory_Move, 1, WRTTIDefaultAllocator<WParticleBehaviorFactory_Move>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("MoveX_Mode", WMovementMode, m_MoveX_Mode)->AddAttributes(new WGroupAttribute("X Axis")),
    W_MEMBER_PROPERTY("MoveX_Speed", m_fMoveX_Speed),
    W_MEMBER_PROPERTY("MoveX_Curve", m_MoveX_Curve),
    W_RESOURCE_MEMBER_PROPERTY("MoveX_SharedCurve", m_hMoveX_SharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("MoveX_CurveOffset", m_fMoveX_CurveOffset)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MoveX_CurveScale", m_fMoveX_CurveScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),

    W_ENUM_MEMBER_PROPERTY("MoveY_Mode", WMovementMode, m_MoveY_Mode)->AddAttributes(new WGroupAttribute("Y Axis")),
    W_MEMBER_PROPERTY("MoveY_Speed", m_fMoveY_Speed),
    W_MEMBER_PROPERTY("MoveY_Curve", m_MoveY_Curve),
    W_RESOURCE_MEMBER_PROPERTY("MoveY_SharedCurve", m_hMoveY_SharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("MoveY_CurveOffset", m_fMoveY_CurveOffset)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MoveY_CurveScale", m_fMoveY_CurveScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),

    W_ENUM_MEMBER_PROPERTY("MoveZ_Mode", WMovementMode, m_MoveZ_Mode)->AddAttributes(new WGroupAttribute("Z Axis")),
    W_MEMBER_PROPERTY("MoveZ_Speed", m_fMoveZ_Speed),
    W_MEMBER_PROPERTY("MoveZ_Curve", m_MoveZ_Curve),
    W_RESOURCE_MEMBER_PROPERTY("MoveZ_SharedCurve", m_hMoveZ_SharedCurve)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    W_MEMBER_PROPERTY("MoveZ_CurveOffset", m_fMoveZ_CurveOffset)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_MEMBER_PROPERTY("MoveZ_CurveScale", m_fMoveZ_CurveScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleBehavior_Move, 1, WRTTIDefaultAllocator<WParticleBehavior_Move>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleBehaviorFactory_Move::WParticleBehaviorFactory_Move() = default;
WParticleBehaviorFactory_Move::~WParticleBehaviorFactory_Move() = default;

const WRTTI* WParticleBehaviorFactory_Move::GetBehaviorType() const
{
  return WGetStaticRTTI<WParticleBehavior_Move>();
}

void WParticleBehaviorFactory_Move::CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const
{
  WParticleBehavior_Move* pBehavior = static_cast<WParticleBehavior_Move*>(pObject);

  pBehavior->m_MoveX_Mode = m_MoveX_Mode;
  pBehavior->m_fMoveX_Speed = m_fMoveX_Speed;
  pBehavior->m_fMoveX_CurveOffset = m_fMoveX_CurveOffset;
  pBehavior->m_fMoveX_CurveScale = m_fMoveX_CurveScale;
  pBehavior->m_pMoveX_Curve = &m_RuntimeMoveX_Curve;

  pBehavior->m_MoveY_Mode = m_MoveY_Mode;
  pBehavior->m_fMoveY_Speed = m_fMoveY_Speed;
  pBehavior->m_fMoveY_CurveOffset = m_fMoveY_CurveOffset;
  pBehavior->m_fMoveY_CurveScale = m_fMoveY_CurveScale;
  pBehavior->m_pMoveY_Curve = &m_RuntimeMoveY_Curve;

  pBehavior->m_MoveZ_Mode = m_MoveZ_Mode;
  pBehavior->m_fMoveZ_Speed = m_fMoveZ_Speed;
  pBehavior->m_fMoveZ_CurveOffset = m_fMoveZ_CurveOffset;
  pBehavior->m_fMoveZ_CurveScale = m_fMoveZ_CurveScale;
  pBehavior->m_pMoveZ_Curve = &m_RuntimeMoveZ_Curve;

  pBehavior->m_pPhysicsModule = (WPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(WGetStaticRTTI<WPhysicsWorldModuleInterface>());
}

enum class BehaviorMoveVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void WParticleBehaviorFactory_Move::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)BehaviorMoveVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_MoveX_Mode;
  inout_stream << m_fMoveX_Speed;
  inout_stream << m_hMoveX_SharedCurve;
  inout_stream << m_fMoveX_CurveOffset;
  inout_stream << m_fMoveX_CurveScale;

  m_MoveX_Curve.ConvertToRuntimeData(m_RuntimeMoveX_Curve);
  m_RuntimeMoveX_Curve.SortControlPoints();
  m_RuntimeMoveX_Curve.Save(inout_stream);

  inout_stream << m_MoveY_Mode;
  inout_stream << m_fMoveY_Speed;
  inout_stream << m_hMoveY_SharedCurve;
  inout_stream << m_fMoveY_CurveOffset;
  inout_stream << m_fMoveY_CurveScale;

  m_MoveY_Curve.ConvertToRuntimeData(m_RuntimeMoveY_Curve);
  m_RuntimeMoveY_Curve.SortControlPoints();
  m_RuntimeMoveY_Curve.Save(inout_stream);

  inout_stream << m_MoveZ_Mode;
  inout_stream << m_fMoveZ_Speed;
  inout_stream << m_hMoveZ_SharedCurve;
  inout_stream << m_fMoveZ_CurveOffset;
  inout_stream << m_fMoveZ_CurveScale;

  m_MoveZ_Curve.ConvertToRuntimeData(m_RuntimeMoveZ_Curve);
  m_RuntimeMoveZ_Curve.SortControlPoints();
  m_RuntimeMoveZ_Curve.Save(inout_stream);
}

void WParticleBehaviorFactory_Move::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor)
{
  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  W_ASSERT_DEV(uiVersion <= (int)BehaviorMoveVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_MoveX_Mode;
  inout_stream >> m_fMoveX_Speed;
  inout_stream >> m_hMoveX_SharedCurve;
  inout_stream >> m_fMoveX_CurveOffset;
  inout_stream >> m_fMoveX_CurveScale;

  m_RuntimeMoveX_Curve.Load(inout_stream);
  m_RuntimeMoveX_Curve.SortControlPoints(); // also updates the aabb
  m_RuntimeMoveX_Curve.CreateLinearApproximation();

  if (m_MoveX_Mode == WMovementMode::SharedCurve && m_hMoveX_SharedCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurveResource(m_hMoveX_SharedCurve, WResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeMoveX_Curve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }

  inout_stream >> m_MoveY_Mode;
  inout_stream >> m_fMoveY_Speed;
  inout_stream >> m_hMoveY_SharedCurve;
  inout_stream >> m_fMoveY_CurveOffset;
  inout_stream >> m_fMoveY_CurveScale;

  m_RuntimeMoveY_Curve.Load(inout_stream);
  m_RuntimeMoveY_Curve.SortControlPoints(); // also updates the aabb
  m_RuntimeMoveY_Curve.CreateLinearApproximation();

  if (m_MoveY_Mode == WMovementMode::SharedCurve && m_hMoveY_SharedCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurveResource(m_hMoveY_SharedCurve, WResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeMoveY_Curve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }

  inout_stream >> m_MoveZ_Mode;
  inout_stream >> m_fMoveZ_Speed;
  inout_stream >> m_hMoveZ_SharedCurve;
  inout_stream >> m_fMoveZ_CurveOffset;
  inout_stream >> m_fMoveZ_CurveScale;

  m_RuntimeMoveZ_Curve.Load(inout_stream);
  m_RuntimeMoveZ_Curve.SortControlPoints(); // also updates the aabb
  m_RuntimeMoveZ_Curve.CreateLinearApproximation();

  if (m_MoveZ_Mode == WMovementMode::SharedCurve && m_hMoveZ_SharedCurve.IsValid())
  {
    WResourceLock<WCurve1DResource> pCurveResource(m_hMoveZ_SharedCurve, WResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == WResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeMoveZ_Curve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }
}

void WParticleBehavior_Move::CreateRequiredStreams()
{
  CreateStream("Position", WProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  // Need lifetime stream if any axis uses curves
  const bool bNeedsLifeTime =
    ((m_MoveX_Mode == WMovementMode::CustomCurve || m_MoveX_Mode == WMovementMode::SharedCurve) && m_pMoveX_Curve && !m_pMoveX_Curve->IsEmpty()) ||
    ((m_MoveY_Mode == WMovementMode::CustomCurve || m_MoveY_Mode == WMovementMode::SharedCurve) && m_pMoveY_Curve && !m_pMoveY_Curve->IsEmpty()) ||
    ((m_MoveZ_Mode == WMovementMode::CustomCurve || m_MoveZ_Mode == WMovementMode::SharedCurve) && m_pMoveZ_Curve && !m_pMoveZ_Curve->IsEmpty());

  if (bNeedsLifeTime)
  {
    CreateStream("LifeTime", WProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
}

void WParticleBehavior_Move::Process(WUInt64 uiNumElements)
{
  W_PROFILE_SCOPE("PFX: Move");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  // Early exit if no movement on any axis
  const bool bHasXMovement = (m_MoveX_Mode == WMovementMode::Constant && m_fMoveX_Speed != 0.0f) ||
                             ((m_MoveX_Mode == WMovementMode::CustomCurve || m_MoveX_Mode == WMovementMode::SharedCurve) && m_pMoveX_Curve && !m_pMoveX_Curve->IsEmpty());
  const bool bHasYMovement = (m_MoveY_Mode == WMovementMode::Constant && m_fMoveY_Speed != 0.0f) ||
                             ((m_MoveY_Mode == WMovementMode::CustomCurve || m_MoveY_Mode == WMovementMode::SharedCurve) && m_pMoveY_Curve && !m_pMoveY_Curve->IsEmpty());
  const bool bHasZMovement = (m_MoveZ_Mode == WMovementMode::Constant && m_fMoveZ_Speed != 0.0f) ||
                             ((m_MoveZ_Mode == WMovementMode::CustomCurve || m_MoveZ_Mode == WMovementMode::SharedCurve) && m_pMoveZ_Curve && !m_pMoveZ_Curve->IsEmpty());

  if (!bHasXMovement && !bHasYMovement && !bHasZMovement)
    return;

  WProcessingStreamIterator<WSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  // Check if any axis uses curves
  const bool bNeedsCurves =
    (m_MoveX_Mode == WMovementMode::CustomCurve || m_MoveX_Mode == WMovementMode::SharedCurve) ||
    (m_MoveY_Mode == WMovementMode::CustomCurve || m_MoveY_Mode == WMovementMode::SharedCurve) ||
    (m_MoveZ_Mode == WMovementMode::CustomCurve || m_MoveZ_Mode == WMovementMode::SharedCurve);

  if (bNeedsCurves && m_pStreamLifeTime)
  {
    // Prepare curve extents for each axis
    double fMinX_X = 0.0, fMaxX_X = 1.0;
    double fMinX_Y = 0.0, fMaxX_Y = 1.0;
    double fMinX_Z = 0.0, fMaxX_Z = 1.0;

    if (bHasXMovement && (m_MoveX_Mode == WMovementMode::CustomCurve || m_MoveX_Mode == WMovementMode::SharedCurve))
    {
      m_pMoveX_Curve->QueryExtents(fMinX_X, fMaxX_X);
      fMinX_X = WMath::Min(fMinX_X, 0.0);
      fMaxX_X = WMath::Max(fMaxX_X, 1.0);
    }

    if (bHasYMovement && (m_MoveY_Mode == WMovementMode::CustomCurve || m_MoveY_Mode == WMovementMode::SharedCurve))
    {
      m_pMoveY_Curve->QueryExtents(fMinX_Y, fMaxX_Y);
      fMinX_Y = WMath::Min(fMinX_Y, 0.0);
      fMaxX_Y = WMath::Max(fMaxX_Y, 1.0);
    }

    if (bHasZMovement && (m_MoveZ_Mode == WMovementMode::CustomCurve || m_MoveZ_Mode == WMovementMode::SharedCurve))
    {
      m_pMoveZ_Curve->QueryExtents(fMinX_Z, fMaxX_Z);
      fMinX_Z = WMath::Min(fMinX_Z, 0.0);
      fMaxX_Z = WMath::Max(fMaxX_Z, 1.0);
    }

    WProcessingStreamIterator<WFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    while (!itPosition.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      WVec3 vMove(0.0f);

      // X-axis movement
      if (bHasXMovement)
      {
        if (m_MoveX_Mode == WMovementMode::CustomCurve || m_MoveX_Mode == WMovementMode::SharedCurve)
        {
          const double evalPos = WMath::Lerp(fMaxX_X, fMinX_X, fLifeTimeFraction);
          const float val = (float)m_pMoveX_Curve->Evaluate(evalPos);
          vMove.x = (m_fMoveX_CurveOffset + val * m_fMoveX_CurveScale) * tDiff;
        }
        else
        {
          vMove.x = m_fMoveX_Speed * tDiff;
        }
      }

      // Y-axis movement
      if (bHasYMovement)
      {
        if (m_MoveY_Mode == WMovementMode::CustomCurve || m_MoveY_Mode == WMovementMode::SharedCurve)
        {
          const double evalPos = WMath::Lerp(fMaxX_Y, fMinX_Y, fLifeTimeFraction);
          const float val = (float)m_pMoveY_Curve->Evaluate(evalPos);
          vMove.y = (m_fMoveY_CurveOffset + val * m_fMoveY_CurveScale) * tDiff;
        }
        else
        {
          vMove.y = m_fMoveY_Speed * tDiff;
        }
      }

      // Z-axis movement (along inverse gravity direction)
      if (bHasZMovement)
      {
        float fZSpeed = 0.0f;
        if (m_MoveZ_Mode == WMovementMode::CustomCurve || m_MoveZ_Mode == WMovementMode::SharedCurve)
        {
          const double evalPos = WMath::Lerp(fMaxX_Z, fMinX_Z, fLifeTimeFraction);
          const float val = (float)m_pMoveZ_Curve->Evaluate(evalPos);
          vMove.z = (m_fMoveZ_CurveOffset + val * m_fMoveZ_CurveScale) * tDiff;
        }
        else
        {
          vMove.z = m_fMoveZ_Speed * tDiff;
        }
      }

      itPosition.Current() += WSimdConversion::ToVec3(vMove);

      itPosition.Advance();
      itLifeTime.Advance();
    }
  }
  else
  {
    // Constant movement on all axes
    WVec3 vMove(0.0f);

    if (bHasXMovement)
      vMove.x = m_fMoveX_Speed * tDiff;

    if (bHasYMovement)
      vMove.y = m_fMoveY_Speed * tDiff;

    if (bHasZMovement)
      vMove.z = m_fMoveZ_Speed * tDiff;

    const WSimdVec4f vMoveSimd = WSimdConversion::ToVec3(vMove);

    while (!itPosition.HasReachedEnd())
    {
      itPosition.Current() += vMoveSimd;
      itPosition.Advance();
    }
  }
}

void WParticleBehavior_Move::RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<WPhysicsWorldModuleInterface>();
}


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Move);
