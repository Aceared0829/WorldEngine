#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>


using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

/// Classifies the facing of an individual raycast hit
enum class WPhysicsHitType : int8_t
{
  Undefined = -1,        ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0, ///< The raycast hit the front face of a triangle
  TriangleBackFace = 1,  ///< The raycast hit the back face of a triangle
};

/// Used for raycast and sweep tests
struct WPhysicsCastResult
{
  WVec3 m_vPosition;
  WVec3 m_vNormal;
  float m_fDistance;

  WGameObjectHandle m_hShapeObject;                        ///< The game object to which the hit physics shape is attached.
  WGameObjectHandle m_hActorObject;                        ///< The game object to which the parent actor of the hit physics shape is attached.
  WSurfaceResourceHandle m_hSurface;                       ///< The type of surface that was hit (if available)
  WUInt32 m_uiObjectFilterID = WInvalidIndex;             ///< An ID either per object (rigid-body / ragdoll) or per shape (implementation specific) that can be used to ignore this object during raycasts and shape queries.
  WPhysicsHitType m_hitType = WPhysicsHitType::Undefined; ///< Classification of the triangle face, see WPhysicsHitType

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct WPhysicsCastResultArray
{
  WHybridArray<WPhysicsCastResult, 16> m_Results;
};

/// Used to report overlap query results
struct WPhysicsOverlapResult
{
  W_DECLARE_POD_TYPE();

  WGameObjectHandle m_hShapeObject;            ///< The game object to which the hit physics shape is attached.
  WGameObjectHandle m_hActorObject;            ///< The game object to which the parent actor of the hit physics shape is attached.
  WUInt32 m_uiObjectFilterID = WInvalidIndex; ///< The shape id of the hit physics shape
  WVec3 m_vCenterPosition;                     ///< The center position of the reported object in world space.

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct WPhysicsOverlapResultArray
{
  WHybridArray<WPhysicsOverlapResult, 16> m_Results;
};

/// Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
///
/// This is mainly for optimization purposes. It is up to the physics integration to support some or all of these flags.
///
/// Note: If this is modified, 'Physics.ts' also has to be updated.
W_DECLARE_FLAGS_WITH_DEFAULT(WUInt32, WPhysicsShapeType, 0xFFFFFFFF,
  Static,    ///< Static geometry
  Dynamic,   ///< Dynamic and kinematic objects
  Query,     ///< Query shapes are kinematic bodies that don't participate in the simulation and are only used for raycasts and other queries.
  Trigger,   ///< Trigger shapes
  Character, ///< Shapes associated with character controllers.
  Ragdoll,   ///< All shapes belonging to ragdolls.
  Rope,      ///< All shapes belonging to ropes.
  Cloth,     ///< Soft-body shapes. Mainly for decorative purposes.
  Debris     ///< Small stuff for visuals, but shouldn't affect the game. This will only have one-way interactions, ie get pushed, but won't push others.
);

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WPhysicsShapeType);

struct WPhysicsQueryParameters
{
  WPhysicsQueryParameters() = default;
  explicit WPhysicsQueryParameters(WUInt32 uiCollisionLayer,
    WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Default, WUInt32 uiIgnoreObjectFilterID = WInvalidIndex)
    : m_uiCollisionLayer(uiCollisionLayer)
    , m_ShapeTypes(shapeTypes)
    , m_uiIgnoreObjectFilterID(uiIgnoreObjectFilterID)
  {
  }

  WUInt32 m_uiCollisionLayer = 0;
  WBitflags<WPhysicsShapeType> m_ShapeTypes = WPhysicsShapeType::Default;
  WUInt32 m_uiIgnoreObjectFilterID = WInvalidIndex;
  bool m_bIgnoreInitialOverlap = false;
};

enum class WPhysicsHitCollection
{
  Closest,
  Any
};
