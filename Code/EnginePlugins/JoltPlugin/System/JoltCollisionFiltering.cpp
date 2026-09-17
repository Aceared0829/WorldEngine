#include <JoltPlugin/JoltPluginPCH.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>

namespace WJoltCollisionFiltering
{
  JPH::ObjectLayer ConstructObjectLayer(WUInt8 uiCollisionGroup, WJoltBroadphaseLayer broadphase)
  {
    return static_cast<JPH::ObjectLayer>(static_cast<WUInt16>(broadphase) << 8 | static_cast<WUInt16>(uiCollisionGroup));
  }

  WUInt32 GetBroadphaseCollisionMask(WJoltBroadphaseLayer broadphase)
  {
    // this mapping defines which types of objects can generally collide with each other
    // if a flag is not included here, those types will never collide, no matter what their collision group is and other filter settings are
    // note that this is only used for the simulation, raycasts and shape queries can use their own mapping

    switch (broadphase)
    {
      case WJoltBroadphaseLayer::Static:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll) | W_BIT((WUInt32)WJoltBroadphaseLayer::Rope) | W_BIT((WUInt32)WJoltBroadphaseLayer::Cloth) | W_BIT((WUInt32)WJoltBroadphaseLayer::Debris);

      case WJoltBroadphaseLayer::Dynamic:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Trigger) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll) | W_BIT((WUInt32)WJoltBroadphaseLayer::Rope) | W_BIT((WUInt32)WJoltBroadphaseLayer::Cloth) | W_BIT((WUInt32)WJoltBroadphaseLayer::Debris);

      case WJoltBroadphaseLayer::Query:
        // query shapes never interact with anything in the simulation
        return 0;

      case WJoltBroadphaseLayer::Trigger:
        // triggers specifically exclude detail objects such as ropes, ragdolls and queries (also used for hitboxes) for performance reasons
        // if necessary, these shapes can still be found with overlap queries
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character);

      case WJoltBroadphaseLayer::Character:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Trigger) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character) | W_BIT((WUInt32)WJoltBroadphaseLayer::Cloth) | W_BIT((WUInt32)WJoltBroadphaseLayer::Debris);

      case WJoltBroadphaseLayer::Ragdoll:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll) | W_BIT((WUInt32)WJoltBroadphaseLayer::Rope) | W_BIT((WUInt32)WJoltBroadphaseLayer::Cloth) | W_BIT((WUInt32)WJoltBroadphaseLayer::Debris);

      case WJoltBroadphaseLayer::Rope:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll) | W_BIT((WUInt32)WJoltBroadphaseLayer::Rope);

      case WJoltBroadphaseLayer::Cloth:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll);

      case WJoltBroadphaseLayer::Debris:
        return W_BIT((WUInt32)WJoltBroadphaseLayer::Static) | W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic) | W_BIT((WUInt32)WJoltBroadphaseLayer::Character) | W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll);

        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0;
  };

} // namespace WJoltCollisionFiltering

WUInt32 WJoltObjectToBroadphaseLayer::GetNumBroadPhaseLayers() const
{
  return (WUInt32)WJoltBroadphaseLayer::ENUM_COUNT;
}

JPH::BroadPhaseLayer WJoltObjectToBroadphaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
  return JPH::BroadPhaseLayer(inLayer >> 8);
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* WJoltObjectToBroadphaseLayer::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
  switch (inLayer)
  {
    case Static:
      return "Static";

    case Dynamic:
      return "Dynamic";

    case Query:
      return "QueryShapes";

    case Trigger:
      return "Trigger";

    case Character:
      return "Character";

    case Ragdoll:
      return "Ragdoll";

    case Rope:
      return "Rope";

    case Cloth:
      return "Cloth";

    case Debris:
      return "Debris";

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
#endif

// if any of these asserts fails, WPhysicsShapeType and WJoltBroadphaseLayer are out of sync
static_assert(WPhysicsShapeType::Static == W_BIT((WUInt32)WJoltBroadphaseLayer::Static));
static_assert(WPhysicsShapeType::Dynamic == W_BIT((WUInt32)WJoltBroadphaseLayer::Dynamic));
static_assert(WPhysicsShapeType::Query == W_BIT((WUInt32)WJoltBroadphaseLayer::Query));
static_assert(WPhysicsShapeType::Trigger == W_BIT((WUInt32)WJoltBroadphaseLayer::Trigger));
static_assert(WPhysicsShapeType::Character == W_BIT((WUInt32)WJoltBroadphaseLayer::Character));
static_assert(WPhysicsShapeType::Ragdoll == W_BIT((WUInt32)WJoltBroadphaseLayer::Ragdoll));
static_assert(WPhysicsShapeType::Rope == W_BIT((WUInt32)WJoltBroadphaseLayer::Rope));
static_assert(WPhysicsShapeType::Cloth == W_BIT((WUInt32)WJoltBroadphaseLayer::Cloth));
static_assert(WPhysicsShapeType::Debris == W_BIT((WUInt32)WJoltBroadphaseLayer::Debris));
static_assert(WPhysicsShapeType::Count == (WUInt32)WJoltBroadphaseLayer::ENUM_COUNT);

bool WJoltObjectLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer) const
{
  return WJoltCore::GetCollisionFilterConfig().IsCollisionEnabled(m_uiCollisionLayer, static_cast<WUInt32>(inLayer) & 0xFF);
}

bool WJoltObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
  const WUInt32 uiMask1 = static_cast<WUInt32>(W_BIT(inLayer1 >> 8));
  const WUInt32 uiMask2 = WJoltCollisionFiltering::GetBroadphaseCollisionMask(static_cast<WJoltBroadphaseLayer>((WUInt8)inLayer2));

  return (uiMask1 & uiMask2) != 0;
}

bool WJoltObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
  return WJoltCore::GetCollisionFilterConfig().IsCollisionEnabled(static_cast<WUInt32>(inObject1) & 0xFF, static_cast<WUInt32>(inObject2) & 0xFF);
}
