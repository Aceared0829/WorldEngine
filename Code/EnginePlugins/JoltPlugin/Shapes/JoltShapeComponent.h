#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Component.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct WMsgExtractGeometry;
struct WMsgUpdateLocalBounds;
class WJoltUserData;
class WJoltMaterial;

namespace JPH
{
  class Shape;
}

struct WJoltSubShape
{
  JPH::Shape* m_pShape = nullptr;
  WTransform m_Transform = WTransform::MakeIdentity();
};

/// Base class for all Jolt physics shapes.
///
/// A physics shape is used to represent (part of) a physical actor, such as a box or sphere,
/// which is used for the rigid body simulation.
///
/// When an actor is created, it searches for WJoltShapeComponent on its own object and all child objects.
/// It then adds all these shapes, with their respective transforms, to the Jolt actor.
class W_JOLTPLUGIN_DLL WJoltShapeComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WJoltShapeComponent, WComponent);


  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WJoltShapeComponent

public:
  WJoltShapeComponent();
  ~WJoltShapeComponent();

  /// If overridden, a triangular representation of the physics shape is added to the geometry object.
  ///
  /// This may be used for debug visualization or navmesh generation (though there are also other ways to do those).
  virtual void ExtractGeometry(WMsgExtractGeometry& ref_msg) const {}

protected:
  friend class WJoltActorComponent;
  virtual void CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial) = 0;

  const WJoltUserData* GetUserData();
  WUInt32 GetUserDataIndex();

  WUInt32 m_uiUserDataIndex = WInvalidIndex;
};
