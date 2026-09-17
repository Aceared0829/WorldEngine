#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/CollisionCollectorImpl.h>

void FillCastResult(WPhysicsCastResult& ref_result, const WVec3& vStart, const WVec3& vDir, float fDistance, const JPH::BodyID& bodyId, const JPH::SubShapeID& subShapeId, const JPH::BodyLockInterface& lockInterface, const JPH::BodyInterface& bodyInterface, const WJoltWorldModule* pModule)
{
  JPH::BodyLockRead bodyLock(lockInterface, bodyId);
  W_ASSERT_DEBUG(bodyLock.Succeeded(), "Failed to get body lock");
  const auto& body = bodyLock.GetBody();
  ref_result.m_vNormal = WJoltConversionUtils::ToVec3(body.GetWorldSpaceSurfaceNormal(subShapeId, WJoltConversionUtils::ToVec3(ref_result.m_vPosition)));
  ref_result.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

  if (WComponent* pShapeComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(subShapeId))))
  {
    ref_result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
  }

  if (WComponent* pActorComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
  {
    ref_result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
  }

  if (const WJoltMaterial* pMaterial = static_cast<const WJoltMaterial*>(bodyInterface.GetMaterial(bodyId, subShapeId)))
  {
    if (pMaterial->m_pSurface)
    {
      ref_result.m_hSurface = pMaterial->m_pSurface->GetResourceHandle();
    }
  }

  const size_t uiBodyId = bodyId.GetIndexAndSequenceNumber();
  const size_t uiShapeId = subShapeId.GetValue();
  ref_result.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
  ref_result.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);
}

class WRayCastCollector : public JPH::CastRayCollector
{
public:
  JPH::RayCastResult m_Result;
  bool m_bAnyHit = false;
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    if (result.mFraction < m_Result.mFraction)
    {
      m_Result = result;
      m_bFoundAny = true;

      if (m_bAnyHit)
      {
        ForceEarlyOut();
      }
    }
  }
};

bool WJoltWorldModule::Raycast(WPhysicsCastResult& out_result, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection /*= WPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin = WJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = WJoltConversionUtils::ToVec3(vDir * fDistance);

  WRayCastCollector collector;
  collector.m_bAnyHit = collection == WPhysicsHitCollection::Any;

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  if (params.m_bIgnoreInitialOverlap)
  {
    JPH::RayCastSettings opt;
    opt.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces; // necessary for soft bodies to work right
    opt.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;
    opt.mTreatConvexAsSolid = false;

    query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

    if (collector.m_bFoundAny == false)
      return false;
  }
  else
  {
    if (!query.CastRay(ray, collector.m_Result, broadphaseFilter, objectFilter, bodyFilter))
      return false;
  }

  out_result.m_fDistance = collector.m_Result.mFraction * fDistance;
  out_result.m_vPosition = vStart + fDistance * collector.m_Result.mFraction * vDir;

  FillCastResult(out_result, vStart, vDir, fDistance, collector.m_Result.mBodyID, collector.m_Result.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class WRayCastCollectorAll : public JPH::CastRayCollector
{
public:
  WArrayPtr<JPH::RayCastResult> m_Results;
  WUInt32 m_uiFound = 0;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    m_Results[m_uiFound] = result;
    ++m_uiFound;
  }
};

bool WJoltWorldModule::RaycastAll(WPhysicsCastResultArray& out_results, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin = WJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = WJoltConversionUtils::ToVec3(vDir * fDistance);

  WRayCastCollectorAll collector;
  collector.m_Results = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), JPH::RayCastResult, 256);

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RayCastSettings opt;
  opt.mBackFaceModeTriangles = params.m_bIgnoreInitialOverlap ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
  opt.mBackFaceModeConvex = opt.mBackFaceModeTriangles;
  opt.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;

  query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (collector.m_uiFound == 0)
    return false;

  out_results.m_Results.SetCount(collector.m_uiFound);

  for (WUInt32 i = 0; i < collector.m_uiFound; ++i)
  {
    out_results.m_Results[i].m_fDistance = collector.m_Results[i].mFraction * fDistance;
    out_results.m_Results[i].m_vPosition = vStart + fDistance * collector.m_Results[i].mFraction * vDir;

    FillCastResult(out_results.m_Results[i], vStart, vDir, fDistance, collector.m_Results[i].mBodyID, collector.m_Results[i].mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);
  }

  return true;
}

class WJoltShapeCastCollector : public JPH::CastShapeCollector
{
public:
  JPH::ShapeCastResult m_Result;
  bool m_bFoundAny = false;
  bool m_bAnyHit = false;

  virtual void AddHit(const JPH::ShapeCastResult& result) override
  {
    if (result.mIsBackFaceHit)
      return;

    if (result.mFraction >= GetEarlyOutFraction())
      return;

    m_bFoundAny = true;
    m_Result = result;

    UpdateEarlyOutFraction(result.mFraction);

    if (m_bAnyHit)
      ForceEarlyOut();
  }
};

bool WJoltWorldModule::SweepTestSphere(WPhysicsCastResult& out_result, float fSphereRadius, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return SweepTest(out_result, shape, JPH::Mat44::sTranslation(WJoltConversionUtils::ToVec3(vStart)), vDir, fDistance, params, collection);
}

bool WJoltWorldModule::SweepTestBox(WPhysicsCastResult& out_result, const WVec3& vBoxExtents, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const
{
  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(WJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(transform.m_qRotation), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool WJoltWorldModule::SweepTestCapsule(WPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const
{
  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool WJoltWorldModule::SweepTestCylinder(WPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const
{
  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylinderHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool WJoltWorldModule::SweepTest(WPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RShapeCast cast(&shape, JPH::RVec3::sOne(), transform, WJoltConversionUtils::ToVec3(vDir * fDistance));

  WJoltShapeCastCollector collector;
  collector.m_bAnyHit = collection == WPhysicsHitCollection::Any;

  query.CastShape(cast, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  if (!collector.m_bFoundAny)
    return false;

  const auto& res = collector.m_Result;

  out_Result.m_fDistance = res.mFraction * fDistance;
  out_Result.m_vPosition = WJoltConversionUtils::ToVec3(res.mContactPointOn2);

  FillCastResult(out_Result, WJoltConversionUtils::ToVec3(transform.GetTranslation()), vDir, fDistance, res.mBodyID2, res.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class WJoltShapeCollectorAny : public JPH::CollideShapeCollector
{
public:
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_bFoundAny = true;
    ForceEarlyOut();
  }
};

class WJoltShapeCollectorAll : public JPH::CollideShapeCollector
{
public:
  WHybridArray<JPH::CollideShapeResult, 32, WAlignedAllocatorWrapper> m_Results;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_Results.PushBack(result);

    if (m_Results.GetCount() >= 256)
    {
      ForceEarlyOut();
    }
  }
};

bool WJoltWorldModule::OverlapTestSphere(float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return OverlapTest(shape, JPH::Mat44::sTranslation(WJoltConversionUtils::ToVec3(vPosition)), params);
}

bool WJoltWorldModule::OverlapTestBox(const WVec3& vBoxExtents, const WVec3& vPosition, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(WJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(transform.m_qRotation),
    WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool WJoltWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool WJoltWorldModule::OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylinderHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool WJoltWorldModule::OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const WPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  WJoltShapeCollectorAny collector;
  query.CollideShape(&shape, JPH::RVec3::sOne(), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  return collector.m_bFoundAny;
}

void WJoltWorldModule::QueryShapesInSphere(WPhysicsOverlapResultArray& out_results, float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (fSphereRadius <= 0.0f)
    return;

  const JPH::SphereShape shape(fSphereRadius);
  const JPH::Mat44 trans = JPH::Mat44::sTranslation(WJoltConversionUtils::ToVec3(vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void WJoltWorldModule::QueryShapesInBox(WPhysicsOverlapResultArray& out_results, const WVec3& vBoxExtents, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return;

  const JPH::BoxShape shape(WJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));
  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(transform.m_qRotation),
    WJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void WJoltWorldModule::QueryShapesInCapsule(WPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void WJoltWorldModule::QueryShapesInCylinder(WPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylindereHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  const WVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylindereHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * WMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  WQuat qFixRot = WQuat::MakeFromAxisAndAngle(WVec3(1, 0, 0), WAngle::MakeFromDegree(90.0f));

  WQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(WJoltConversionUtils::ToQuat(qRot), WJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void WJoltWorldModule::QueryShapes(WPhysicsOverlapResultArray& out_results, const JPH::Shape& shape, const JPH::Mat44& transform, const WPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  WJoltShapeCollectorAll collector;

  query.CollideShape(&shape, JPH::RVec3::sOne(), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  out_results.m_Results.SetCount(collector.m_Results.GetCount());

  auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();

  for (WUInt32 i = 0; i < collector.m_Results.GetCount(); ++i)
  {
    auto& overlapResult = out_results.m_Results[i];
    auto& overlapHit = collector.m_Results[i];

    JPH::BodyLockRead bodyLock(lockInterface, overlapHit.mBodyID2);
    W_ASSERT_DEBUG(bodyLock.Succeeded(), "Failed to get body lock");
    const auto& body = bodyLock.GetBody();

    overlapResult.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();
    overlapResult.m_vCenterPosition = WJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());

    const size_t uiBodyId = body.GetID().GetIndexAndSequenceNumber();
    const size_t uiShapeId = overlapHit.mSubShapeID2.GetValue();
    overlapResult.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
    overlapResult.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);

    if (WComponent* pShapeComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(overlapHit.mSubShapeID2))))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
    }

    if (WComponent* pActorComponent = WJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}

void WJoltWorldModule::QueryGeometryInBox(const WPhysicsQueryParameters& params, WBoundingBox box, WDynamicArray<WNavmeshTriangle>& out_triangles) const
{
  JPH::AABox aabb;
  aabb.mMin = WJoltConversionUtils::ToVec3(box.m_vMin);
  aabb.mMax = WJoltConversionUtils::ToVec3(box.m_vMax);

  JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;

  WJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  WJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
  WJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  m_pSystem->GetNarrowPhaseQuery().CollectTransformedShapes(aabb, collector, broadphaseFilter, objectFilter, bodyFilter);

  const int cMaxTriangles = 128;

  WStaticArray<WVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  WStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  for (const JPH::TransformedShape& ts : collector.mHits)
  {
    JPH::Shape::GetTrianglesContext ctx;
    ts.GetTrianglesStart(ctx, aabb, JPH::Vec3::sZero());

    while (true)
    {
      const int triCount = ts.GetTrianglesNext(ctx, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

      if (triCount == 0)
        break;

      out_triangles.Reserve(out_triangles.GetCount() + triCount);

      for (int i = 0; i < triCount; ++i)
      {
        const WJoltMaterial* pMat = static_cast<const WJoltMaterial*>(materialsTmp[i]);

        auto& tri = out_triangles.ExpandAndGetRef();
        tri.m_pSurface = pMat ? pMat->m_pSurface : nullptr;
        tri.m_Vertices[0] = positionsTmp[i * 3 + 0];
        tri.m_Vertices[1] = positionsTmp[i * 3 + 1];
        tri.m_Vertices[2] = positionsTmp[i * 3 + 2];
      }
    }
  }
}
