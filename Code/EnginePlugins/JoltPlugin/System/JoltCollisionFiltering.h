#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Basics.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/JoltPluginDLL.h>

class WCollisionFilterConfig;

namespace JPH
{
  using ObjectLayer = WUInt16;
} // namespace JPH

enum class WJoltBroadphaseLayer : WUInt8
{
  Static,
  Dynamic,
  Query,
  Trigger,
  Character,
  Ragdoll,
  Rope,
  Cloth,
  Debris,

  ENUM_COUNT
};

namespace WJoltCollisionFiltering
{
  /// Constructs the JPH::ObjectLayer value from the desired collision group index and the broadphase into which the object shall be sorted
  W_JOLTPLUGIN_DLL JPH::ObjectLayer ConstructObjectLayer(WUInt8 uiCollisionGroup, WJoltBroadphaseLayer broadphase);

  /// Returns the (hard-coded) collision mask that determines which other broad-phases to collide with.
  W_JOLTPLUGIN_DLL WUInt32 GetBroadphaseCollisionMask(WJoltBroadphaseLayer broadphase);

}; // namespace WJoltCollisionFiltering


class W_JOLTPLUGIN_DLL WJoltObjectToBroadphaseLayer final : public JPH::BroadPhaseLayerInterface
{
public:
  virtual WUInt32 GetNumBroadPhaseLayers() const override;

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
};

class W_JOLTPLUGIN_DLL WJoltBroadPhaseLayerFilter final : public JPH::BroadPhaseLayerFilter
{
public:
  WJoltBroadPhaseLayerFilter(WBitflags<WPhysicsShapeType> shapeTypes)
  {
    m_uiCollisionMask = shapeTypes.GetValue();
  }

  WUInt32 m_uiCollisionMask = 0;

  virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
  {
    return (W_BIT(static_cast<WUInt8>(inLayer)) & m_uiCollisionMask) != 0;
  }
};

class W_JOLTPLUGIN_DLL WJoltObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
  WUInt32 m_uiCollisionLayer = 0;

  WJoltObjectLayerFilter(WUInt32 uiCollisionLayer)
    : m_uiCollisionLayer(uiCollisionLayer)
  {
  }

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override;
};

class W_JOLTPLUGIN_DLL WJoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  WJoltObjectVsBroadPhaseLayerFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

class W_JOLTPLUGIN_DLL WJoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
  WJoltObjectLayerPairFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
};

class W_JOLTPLUGIN_DLL WJoltBodyFilter final : public JPH::BodyFilter
{
public:
  WUInt32 m_uiObjectFilterIDToIgnore = WInvalidIndex - 1;

  WJoltBodyFilter(WUInt32 uiBodyFilterIdToIgnore = WInvalidIndex - 1)
    : m_uiObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
  {
  }

  void ClearFilter()
  {
    m_uiObjectFilterIDToIgnore = WInvalidIndex - 1;
  }

  virtual bool ShouldCollideLocked(const JPH::Body& body) const override
  {
    return body.GetCollisionGroup().GetGroupID() != m_uiObjectFilterIDToIgnore;
  }
};
