#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/Bitflags.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

namespace JPH
{
  class Shape;
  class BodyCreationSettings;
} // namespace JPH

/// Base class for all Jolt actors.
///
/// An actor is an object that participates in the physical simulation.
/// It is often also called a (rigid) body.
/// An actor is made out of one or multiple shapes that define its geometry.
/// Different types of actors differ in how they participate in the simulation.
class W_JOLTPLUGIN_DLL WJoltActorComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WJoltActorComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltActorComponent

public:
  WJoltActorComponent();
  ~WJoltActorComponent();

  /// The collision layer determines with which other actors this actor collides.
  ///
  /// Which collision layers collide with each other is configured through the WCollisionFilterConfig.
  /// \see WJoltCollisionFiltering::GetCollisionFilterConfig()
  WUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// Sets the object filter ID to use. This can only be set right after creation, before the component gets activated.
  void SetInitialObjectFilterID(WUInt32 uiObjectFilterID);

  /// The object filter ID can be used to ignore collisions specifically with this one object.
  WUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

  /// Returns the internal ID used by Jolt to identify this actor/body.
  WUInt32 GetJoltBodyID() const { return m_uiJoltBodyID; }

protected:
  const WJoltUserData* GetUserData() const;

  void ExtractSubShapeGeometry(const WGameObject* pObject, WMsgExtractGeometry& msg) const;

  static void GatherShapes(WDynamicArray<WJoltSubShape>& shapes, WGameObject* pObject, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial);
  WResult CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const WJoltMaterial* pMaterial);

  virtual void CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial) {}

  WUInt32 m_uiUserDataIndex = WInvalidIndex;
  WUInt32 m_uiJoltBodyID = WInvalidIndex;
  WUInt32 m_uiObjectFilterID = WInvalidIndex;
};
