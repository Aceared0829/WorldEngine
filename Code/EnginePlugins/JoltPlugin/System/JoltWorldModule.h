#pragma once

#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

class WJoltCharacterControllerComponent;
class WJoltContactListener;
class WJoltMaterial;
class WJoltSoftBodyContactListener;
class WJoltRagdollComponent;
class WJoltRopeComponent;
class WJoltBreakableSlabComponent;
class WView;

namespace JPH
{
  class Body;
  class TempAllocator;
  class PhysicsSystem;
  class GroupFilter;
  class Shape;

  class BodyInterface;
} // namespace JPH

struct WJoltImpulse
{
  WUInt32 m_uiBodyID;
  WVec3 m_vImpulse;
  WVec3 m_vGlobalPosition;

  enum class Type : WUInt8
  {
    Center,
    AtGlobalPos,
    Angular
  };

  Type m_Type;
};

struct WJoltForce
{
  WUInt32 m_uiBodyID;
  WTime m_tDisable;
  WVec3 m_vForce;
};

using WJoltForceId = WGenericId<24, 8>;

class W_JOLTPLUGIN_DLL WJoltWorldModule : public WPhysicsWorldModuleInterface
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WJoltWorldModule, WPhysicsWorldModuleInterface);

public:
  WJoltWorldModule(WWorld* pWorld);
  ~WJoltWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  JPH::PhysicsSystem* GetJoltSystem() { return m_pSystem.get(); }
  const JPH::PhysicsSystem* GetJoltSystem() const { return m_pSystem.get(); }

  JPH::BodyInterface& GetBodyInterface() { return m_pSystem->GetBodyInterface(); }

  WUInt32 CreateObjectFilterID();
  void DeleteObjectFilterID(WUInt32& ref_uiObjectFilterID);

  WUInt32 AllocateUserData(WJoltUserData*& out_pUserData);
  void DeallocateUserData(WUInt32& ref_uiUserDataId);
  const WJoltUserData& GetUserData(WUInt32 uiUserDataId) const;

  void SetGravity(const WVec3& vObjectGravity, const WVec3& vCharacterGravity);
  virtual WVec3 GetGravity() const override { return m_Settings.m_vObjectGravity; }
  WVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }

  /// Queues an impulse to be applied on the given body as soon as that body is added to the Jolt scene.
  void AddImpulse(WUInt32 uiBodyID, const WVec3& vImpulse, const WVec3& vGlobalPosition);

  /// Queues an impulse to be applied on the given body as soon as that body is added to the Jolt scene.
  void AddImpulse(WUInt32 uiBodyID, const WVec3& vImpulse);

  /// Queues an angular impulse to be applied on the given body as soon as that body is added to the Jolt scene.
  void AddTorque(WUInt32 uiBodyID, const WVec3& vImpulse);

  /// Creates a force that acts upon the given Jolt body for a limited time.
  ///
  /// The force is applied every frame. It can be updated by calling this function again with a previously returned force ID.
  /// Once the duration is elapsed without an update, the force is removed.
  ///
  /// If an invalid ID is passed in, a new force is created and a valid ID is returned.
  /// If a valid ID is passed in, the existing force gets updated, and the same ID is returned.
  WJoltForceId AddOrUpdateForce(WJoltForceId forceId, WUInt32 uiBodyID, WTime duration, const WVec3& vForce);

  /// Removes a previously added force. See AddOrUpdateForce().
  void ClearForce(WJoltForceId id);

  //////////////////////////////////////////////////////////////////////////
  // WPhysicsWorldModuleInterface
  //

  virtual WUInt32 GetCollisionLayerByName(WStringView sName) const override;
  virtual WUInt8 GetWeightCategoryByName(WStringView sName) const override;
  virtual WUInt8 GetImpulseTypeByName(WStringView sName) const override;

  virtual bool Raycast(WPhysicsCastResult& out_result, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(WPhysicsCastResultArray& out_results, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(WPhysicsCastResult& out_result, float fSphereRadius, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(WPhysicsCastResult& out_result, const WVec3& vBoxExtents, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(WPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCylinder(WPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestBox(const WVec3& vBoxExtents, const WVec3& vPosition, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(WPhysicsOverlapResultArray& out_results, float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInBox(WPhysicsOverlapResultArray& out_results, const WVec3& vBoxExtents, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInCapsule(WPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInCylinder(WPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(WGameObject* pObject, WVec3 vBoxSize) override;

  virtual void AddFixedJointComponent(WGameObject* pOwner, const WPhysicsWorldModuleInterface::FixedJointConfig& cfg) override;

  virtual WBoundingBoxSphere GetWorldSpaceBounds(WGameObject* pOwner, WUInt32 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const override;

  virtual WResult TrySetHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier) override;

  virtual void CreateHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier, const HeightfieldColliderData& data) override;

  virtual void RemoveHeightfieldCollider(WGameObject* pOwner) override;

  WDeque<WComponentHandle> m_RequireUpdate;

  const WSet<WJoltDynamicActorComponent*>& GetActiveActors() const { return m_ActiveActors; }
  const WMap<WJoltRopeComponent*, WInt32>& GetActiveRopes() const { return m_ActiveRopes; }
  const WMap<WJoltRagdollComponent*, WInt32>& GetActiveRagdolls() const { return m_ActiveRagdolls; }
  WArrayPtr<WJoltRagdollComponent*> GetRagdollsPutToSleep() { return m_RagdollsPutToSleep.GetArrayPtr(); }
  const WMap<WJoltBreakableSlabComponent*, WInt32>& GetActiveSlabs() const { return m_ActiveSlabs; }
  WArrayPtr<WJoltBreakableSlabComponent*> GetSlabsPutToSleep() { return m_SlabsPutToSleep.GetArrayPtr(); }

  void QueueBodyToAdd(JPH::Body* pBody, bool bAwake);
  void RemoveBodyFromQueue(JPH::BodyID bodyId);

  JPH::GroupFilter* GetGroupFilter() const { return m_pGroupFilter; }
  JPH::GroupFilter* GetGroupFilterIgnoreSame() const { return m_pGroupFilterIgnoreSame; }

  void EnableJoinedBodiesCollisions(WUInt32 uiObjectFilterID1, WUInt32 uiObjectFilterID2, bool bEnable);

  JPH::TempAllocator* GetTempAllocator() const { return m_pTempAllocator.get(); }

  void ActivateCharacterController(WJoltCharacterControllerComponent* pCharacter, bool bActivate);

  WJoltContactListener* GetContactListener()
  {
    return reinterpret_cast<WJoltContactListener*>(m_pContactListener);
  }

  WJoltSoftBodyContactListener* GetSoftBodyContactListener()
  {
    return reinterpret_cast<WJoltSoftBodyContactListener*>(m_pSoftBodyContactListener);
  }

  void CheckBreakableConstraints();

  WSet<WComponentHandle> m_BreakableConstraints;

  void QueryGeometryInBox(const WPhysicsQueryParameters& params, WBoundingBox box, WDynamicArray<WNavmeshTriangle>& out_triangles) const;

  /// Returns the counter of the last Jolt update.
  /// Can be used to detect when no physics update was done (at high frame rates) to skip duplicate physics modifications.
  WUInt64 GetJoltUpdateCounter() const { return m_uiJoltUpdateCounter; }

private:
  bool SweepTest(WPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection) const;

  static void AttachHeightfieldBody(WJoltWorldModule* pModule, WGameObject* pOwner, const WJoltHeightfieldResourceHandle& hRes);
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const WPhysicsQueryParameters& params) const;
  void QueryShapes(WPhysicsOverlapResultArray& out_results, const JPH::Shape& shape, const JPH::Mat44& transform, const WPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const WWorldModule::UpdateContext& context);
  void FetchResults(const WWorldModule::UpdateContext& context);

  void Simulate();

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void ApplyImpulses();
  void UpdateForces();

  void UpdateConstraints();

  WTime CalculateUpdateSteps();

  void DebugDrawGeometry();
  void DebugDrawGeometry(const WVec3& vCenter, float fRadius, WPhysicsShapeType::Enum shapeType, const WTag& tag, bool bSurfaceColors);

  struct DebugGeo
  {
    WGameObjectHandle m_hObject;
    WUInt32 m_uiLastSeenCounter = 0;
    bool m_bMutableGeometry = false;
  };

  /// The triangles of a shape that use one surface. A mesh can only be drawn with a single color,
  /// so shapes whose triangles use different surfaces are split into several of these.
  struct DebugGeoShapePart
  {
    WDynamicMeshBufferResourceHandle m_hMesh;
    WColorGammaUB m_SurfaceColor = WColor::White;
  };

  struct DebugGeoShape
  {
    WSmallArray<DebugGeoShapePart, 1> m_Parts;
    WBoundingBox m_Bounds;
    WUInt32 m_uiLastSeenCounter = 0;
  };

  struct DebugBodyShapeKey
  {
    WUInt32 m_uiBodyID;
    /// Identifies the sub-shape within the body. Necessary because the same shape instance can be
    /// referenced multiple times by a compound shape (e.g. several colliders using the same mesh resource),
    /// in which case the shape pointer alone is not unique.
    WUInt32 m_uiSubShapeID;
    const void* m_pShapePtr;

    bool operator<(const DebugBodyShapeKey& rhs) const
    {
      if (m_uiBodyID != rhs.m_uiBodyID)
        return m_uiBodyID < rhs.m_uiBodyID;

      if (m_uiSubShapeID != rhs.m_uiSubShapeID)
        return m_uiSubShapeID < rhs.m_uiSubShapeID;

      return m_pShapePtr < rhs.m_pShapePtr;
    }

    bool operator==(const DebugBodyShapeKey& rhs) const
    {
      return (m_uiBodyID == rhs.m_uiBodyID) && (m_uiSubShapeID == rhs.m_uiSubShapeID) && (m_pShapePtr == rhs.m_pShapePtr);
    }
  };

  WUInt64 m_uiJoltUpdateCounter = 0;

  WUInt32 m_uiDebugGeoLastSeenCounter = 0;
  bool m_bDebugGeoSurfaceColors = false; ///< which of the two visualizations the cached debug geometry was built for
  WMap<DebugBodyShapeKey, DebugGeo> m_DebugDrawComponents;
  WMap<const void*, DebugGeoShape> m_DebugDrawShapeGeo;

  WUInt32 m_uiNextObjectFilterID = 1;
  WDynamicArray<WUInt32> m_FreeObjectFilterIDs;

  WDeque<WJoltUserData> m_AllocatedUserData;
  WDynamicArray<WUInt32> m_FreeUserData;
  WDynamicArray<WUInt32> m_FreeUserDataAfterSimulationStep;

  WTime m_AccumulatedTimeSinceUpdate;

  WJoltSettings m_Settings;

  WSharedPtr<WTask> m_pSimulateTask;
  WTaskGroupID m_SimulateTaskGroupId;
  WTime m_SimulatedTimeStep;

  std::unique_ptr<JPH::PhysicsSystem> m_pSystem;
  std::unique_ptr<JPH::TempAllocator> m_pTempAllocator;

  WJoltObjectToBroadphaseLayer m_ObjectToBroadphase;
  WJoltObjectVsBroadPhaseLayerFilter m_ObjectVsBroadphaseFilter;
  WJoltObjectLayerPairFilter m_ObjectLayerPairFilter;

  void* m_pContactListener = nullptr;
  void* m_pSoftBodyContactListener = nullptr;
  void* m_pActivationListener = nullptr;
  WSet<WJoltDynamicActorComponent*> m_ActiveActors;
  WMap<WJoltRopeComponent*, WInt32> m_ActiveRopes;
  WMap<WJoltRagdollComponent*, WInt32> m_ActiveRagdolls;
  WDynamicArray<WJoltRagdollComponent*> m_RagdollsPutToSleep;
  WMap<WJoltBreakableSlabComponent*, WInt32> m_ActiveSlabs;
  WDynamicArray<WJoltBreakableSlabComponent*> m_SlabsPutToSleep;

  JPH::GroupFilter* m_pGroupFilter = nullptr;
  JPH::GroupFilter* m_pGroupFilterIgnoreSame = nullptr;

  WUInt32 m_uiBodiesAddedSinceOptimize = 100;
  WDeque<WUInt32> m_BodiesToAdd;
  WDeque<WUInt32> m_BodiesToAddAndActivate;

  WHybridArray<WTime, 4> m_UpdateSteps;
  WHybridArray<WJoltCharacterControllerComponent*, 4> m_ActiveCharacters;

  // Tracks identifiers of runtime-created heightfield resources for TrySetHeightfieldCollider existence checks.
  WSet<WString> m_RuntimeHeightfieldIDs;

  WMutex m_ImpulsesMutex;
  WDeque<WJoltImpulse> m_Impulses;

  WMutex m_ForcesMutex;
  WIdTable<WJoltForceId, WJoltForce> m_Forces;
};

/// Implementation of the WNavmeshGeoWorldModuleInterface that uses Jolt physics to retrieve the geometry
/// from which to generate a navmesh.
class W_JOLTPLUGIN_DLL WJoltNavmeshGeoWorldModule : public WNavmeshGeoWorldModuleInterface
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WJoltNavmeshGeoWorldModule, WNavmeshGeoWorldModuleInterface);

public:
  WJoltNavmeshGeoWorldModule(WWorld* pWorld);

  virtual void RetrieveGeometryInArea(WUInt32 uiCollisionLayer, const WBoundingBox& box, WDynamicArray<WNavmeshTriangle>& out_triangles) const override;

private:
  WJoltWorldModule* m_pJoltModule = nullptr;
};
