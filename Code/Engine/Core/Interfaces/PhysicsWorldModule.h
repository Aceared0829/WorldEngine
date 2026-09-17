#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>

struct WGameObjectHandle;
struct WSkeletonResourceDescriptor;

/// Interface for physics world modules that provide physics simulation and queries.
///
/// Physics world modules implement physics functionality for a world, including
/// collision detection, raycasting, and shape queries. Different physics engines
/// can provide their own implementations of this interface.
class W_CORE_DLL WPhysicsWorldModuleInterface : public WWorldModule
{
  W_ADD_DYNAMIC_REFLECTION(WPhysicsWorldModuleInterface, WWorldModule);

protected:
  WPhysicsWorldModuleInterface(WWorld* pWorld)
    : WWorldModule(pWorld)
  {
  }

public:
  /// Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns WInvalidIndex if no such collision layer exists.
  virtual WUInt32 GetCollisionLayerByName(WStringView sName) const = 0;

  /// Searches for a weight category with the given name and returns its key.
  ///
  /// Returns WWeightCategoryConfig::InvalidKey if no such category exists.
  virtual WUInt8 GetWeightCategoryByName(WStringView sName) const = 0;

  /// Searches for an impulse type with the given name and returns its key.
  ///
  /// Returns WImpulseTypeConfig::InvalidKey if no such category exists.
  virtual WUInt8 GetImpulseTypeByName(WStringView sName) const = 0;

  virtual bool Raycast(WPhysicsCastResult& out_result, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(WPhysicsCastResultArray& out_results, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(WPhysicsCastResult& out_result, float fSphereRadius, const WVec3& vStart, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(WPhysicsCastResult& out_result, const WVec3& vBoxExtents, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(WPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCylinder(WPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WVec3& vDir, float fDistance, const WPhysicsQueryParameters& params, WPhysicsHitCollection collection = WPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestBox(const WVec3& vBoxExtents, const WVec3& vPosition, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(WPhysicsOverlapResultArray& out_results, float fSphereRadius, const WVec3& vPosition, const WPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInBox(WPhysicsOverlapResultArray& out_results, const WVec3& vBoxExtents, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCapsule(WPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCylinder(WPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const WTransform& transform, const WPhysicsQueryParameters& params) const = 0;

  virtual WVec3 GetGravity() const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(WGameObject* pOwner, WVec3 vBoxSize)
  {
    W_IGNORE_UNUSED(pOwner);
    W_IGNORE_UNUSED(vBoxSize);
  }

  /// Data for creating a heightfield collider.
  struct HeightfieldColliderData
  {
    WVec2 m_vHalfExtents;
    WUInt32 m_uiResolution = 64;

    /// Height samples in row-major order, m_uiResolution * m_uiResolution entries.
    WDynamicArray<float> m_Heights;

    /// Per-cell material indices (one per quad, (m_uiResolution-1)^2 entries). Indexes into m_Surfaces.
    WDynamicArray<WUInt8> m_MaterialIndices;
    WDynamicArray<WSurfaceResourceHandle> m_Surfaces;

    WUInt8 m_uiCollisionLayer = 0;
  };

  /// Tries to create a heightfield collider on pOwner by reusing a shape previously cached under sIdentifier.
  ///
  /// Removes any existing heightfield collider component from pOwner first.
  /// Returns W_FAILURE if no shape with that identifier is cached; the caller should then
  /// call CreateHeightfieldCollider() with the full data.
  virtual WResult TrySetHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier)
  {
    W_IGNORE_UNUSED(pOwner);
    W_IGNORE_UNUSED(sIdentifier);
    return W_FAILURE;
  }

  /// Creates a heightfield collider on pOwner from the given height data and caches the resulting shape under sIdentifier.
  ///
  /// Removes any existing heightfield collider component from pOwner first.
  /// If a non-empty sIdentifier is supplied, the shape is stored so that future calls to
  /// TrySetHeightfieldCollider() with the same identifier can skip the data rebuild.
  virtual void CreateHeightfieldCollider(WGameObject* pOwner, WStringView sIdentifier, const HeightfieldColliderData& data)
  {
    W_IGNORE_UNUSED(pOwner);
    W_IGNORE_UNUSED(sIdentifier);
    W_IGNORE_UNUSED(data);
  }

  /// Removes the heightfield collider component from pOwner, if present.
  ///
  /// Decrements the reference count of the cached shape. If the count reaches zero, the cached shape is evicted.
  virtual void RemoveHeightfieldCollider(WGameObject* pOwner) { W_IGNORE_UNUSED(pOwner); }

  struct JointConfig
  {
    WGameObjectHandle m_hActorA;
    WGameObjectHandle m_hActorB;
    WTransform m_LocalFrameA = WTransform::MakeIdentity();
    WTransform m_LocalFrameB = WTransform::MakeIdentity();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(WGameObject* pOwner, const WPhysicsWorldModuleInterface::FixedJointConfig& cfg)
  {
    W_IGNORE_UNUSED(pOwner);
    W_IGNORE_UNUSED(cfg);
  }

  /// Gets world space bounds of a physics object if its shape type is included in shapeTypes and its collision layer interacts with uiCollisionLayer.
  virtual WBoundingBoxSphere GetWorldSpaceBounds(WGameObject* pOwner, WUInt32 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
  {
    W_IGNORE_UNUSED(pOwner);
    W_IGNORE_UNUSED(uiCollisionLayer);
    W_IGNORE_UNUSED(shapeTypes);
    W_IGNORE_UNUSED(bIncludeChildObjects);
    return WBoundingBoxSphere::MakeInvalid();
  }
};

/// Used to apply a physical impulse on the object
struct W_CORE_DLL WMsgPhysicsAddImpulse : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgPhysicsAddImpulse, WMessage);

  WVec3 m_vGlobalPosition;
  WVec3 m_vImpulse;
  WUInt8 m_uiImpulseType = 0;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct W_CORE_DLL WMsgPhysicsJointBroke : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgPhysicsJointBroke, WMessage);

  WGameObjectHandle m_hJointObject;
};

/// Sent by components such as WJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct W_CORE_DLL WMsgObjectGrabbed : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgObjectGrabbed, WMessage);

  WGameObjectHandle m_hGrabbedBy;
  bool m_bGotGrabbed = true;
};

/// Asks physics actors to temporarily become fully simulated, so that they fall down and settle realistically.
///
/// This is an editor-only tool: it is sent when the user asks the editor to record object transforms during a
/// simulation, so that even objects that are normally not simulated can be placed physically.
/// The change lasts only for the running simulation, nothing needs to be restored, because the runtime world is
/// discarded when the simulation ends.
///
/// Actors that can't be simulated ignore this message. In particular a static actor that only has a concave
/// (triangle mesh) collider can't become dynamic and stays as it is.
struct W_CORE_DLL WMsgPhysicsMakeTemporarilyDynamic : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgPhysicsMakeTemporarilyDynamic, WMessage);
};

/// Send this to components such as WJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct W_CORE_DLL WMsgReleaseObjectGrab : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgReleaseObjectGrab, WMessage);

  WGameObjectHandle m_hGrabbedObjectToRelease;
};

/// Can be sent by character controllers to inform objects when a CC pushes into them.
///
/// Whether this message is sent, depends on the character controller implementation.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct W_CORE_DLL WMsgPhysicCharacterContact : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgPhysicCharacterContact, WMessage);

  WComponentHandle m_hCharacter;
  WVec3 m_vGlobalPosition;
  WVec3 m_vNormal;
  WVec3 m_vCharacterVelocity;
  float m_fImpact;
};

/// Sent to physics components that have contact reporting enabled (see WOnJoltContact::SendContactMsg).
///
/// Only sent for certain physics object combinations, e.g. debris doesn't trigger this.
/// The reported contact position and normal is an average of the contact manifold.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct W_CORE_DLL WMsgPhysicContact : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgPhysicContact, WMessage);

  WGameObjectHandle m_hOtherObject;
  WVec3 m_vGlobalPosition;
  WVec3 m_vNormal;
  float m_fImpactSqr;
};


//////////////////////////////////////////////////////////////////////////

struct W_CORE_DLL WSmcTriangle
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiVertexIndices[3];
};

struct W_CORE_DLL WSmcSubMesh
{
  W_DECLARE_POD_TYPE();

  WUInt32 m_uiFirstTriangle = 0;
  WUInt32 m_uiNumTriangles = 0;
  WUInt16 m_uiSurfaceIndex = 0;
};

struct W_CORE_DLL WSmcDescription
{
  WDeque<WVec3> m_Vertices;
  WDeque<WSmcTriangle> m_Triangles;
  WDeque<WSmcSubMesh> m_SubMeshes;
  WDeque<WString> m_Surfaces;
};

struct W_CORE_DLL WMsgBuildStaticMesh : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgBuildStaticMesh, WMessage);

  /// Append data to this description to add meshes to the automatic static mesh generation
  WSmcDescription* m_pStaticMeshDescription = nullptr;
};
