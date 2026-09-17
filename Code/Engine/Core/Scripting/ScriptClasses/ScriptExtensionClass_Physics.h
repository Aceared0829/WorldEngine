#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class WWorld;
class WGameObject;

/// Script extension class providing physics world queries and utilities for scripts.
///
/// Exposes physics system functionality to scripts including collision detection,
/// raycasting, and shape overlap testing. All functions require a valid world
/// and may return no results if no physics world module is active.
class W_CORE_DLL WScriptExtensionClass_Physics
{
public:
  /// Gets the current gravity vector for the physics world.
  static WVec3 GetGravity(WWorld* pWorld);

  /// Finds collision layer index by name, returns invalid index if not found.
  static WUInt8 GetCollisionLayerByName(WWorld* pWorld, WStringView sLayerName);

  /// Finds weight category index by name, returns invalid key if not found.
  static WUInt8 GetWeightCategoryByName(WWorld* pWorld, WStringView sCategoryName);

  /// Finds impulse type index by name, returns invalid key if not found.
  static WUInt8 GetImpulseTypeByName(WWorld* pWorld, WStringView sImpulseTypeName);

  /// Performs raycast and returns hit information if collision is found.
  static bool Raycast(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, const WVec3& vStart, const WVec3& vDirection, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic, WUInt32 uiIgnoreObjectID = WInvalidIndex);

  /// Tests if a line segment intersects with any physics shapes.
  static bool OverlapTestLine(WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic, WUInt32 uiIgnoreObjectID = WInvalidIndex);

  /// Tests if a sphere at the given position overlaps with any physics shapes.
  static bool OverlapTestSphere(WWorld* pWorld, float fRadius, const WVec3& vPosition, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  /// Tests if a capsule with the given transform overlaps with any physics shapes.
  static bool OverlapTestCapsule(WWorld* pWorld, float fRadius, float fHeight, const WTransform& transform, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  /// Sweeps a sphere along a direction and returns hit information if collision is found.
  static bool SweepTestSphere(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, float fRadius, const WVec3& vStart, const WVec3& vDirection, float fDistance, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  /// Sweeps a capsule along a direction and returns hit information if collision is found.
  static bool SweepTestCapsule(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, float fRadius, float fHeight, const WTransform& start, const WVec3& vDirection, float fDistance, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes = WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic);

  /// Performs raycast and triggers surface interaction at hit point if collision is found.
  static bool RaycastSurfaceInteraction(WWorld* pWorld, const WVec3& vRayStart, const WVec3& vRayDirection, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes, WStringView sFallbackSurface, const WTempHashedString& sInteraction, float fInteractionImpulse, WUInt32 uiIgnoreObjectID = WInvalidIndex);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptExtensionClass_Physics);
