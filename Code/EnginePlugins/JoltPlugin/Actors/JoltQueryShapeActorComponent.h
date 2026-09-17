#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class W_JOLTPLUGIN_DLL WJoltQueryShapeActorComponentManager : public WComponentManager<class WJoltQueryShapeActorComponent, WBlockStorageType::FreeList>
{
public:
  WJoltQueryShapeActorComponentManager(WWorld* pWorld);
  ~WJoltQueryShapeActorComponentManager();

private:
  friend class WJoltWorldModule;
  friend class WJoltQueryShapeActorComponent;

  void UpdateMovingQueryShapes();

  WDynamicArray<WJoltQueryShapeActorComponent*> m_MovingQueryShapes;
};

//////////////////////////////////////////////////////////////////////////

/// A physics actor that can be moved procedurally (like a kinematic actor) but that doesn't affect rigid bodies.
///
/// It passes right through dynamic actors. However, you can detect it via raycasts or shape casts.
/// This is useful to represent detail shapes (like the collision shapes of animated meshes) that should be pickable,
/// but that shouldn't interact with the world otherwise.
/// They are more lightweight at runtime than full kinematic dynamic actors.
class W_JOLTPLUGIN_DLL WJoltQueryShapeActorComponent : public WJoltActorComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltQueryShapeActorComponent, WJoltActorComponent, WJoltQueryShapeActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltQueryShapeActorComponent
public:
  WJoltQueryShapeActorComponent();
  ~WJoltQueryShapeActorComponent();

  void SetSurfaceFile(WStringView sFile); // [ property ]
  WStringView GetSurfaceFile() const;     // [ property ]

  WSurfaceResourceHandle m_hSurface;      // [ property ]

protected:
  const WJoltMaterial* GetJoltMaterial() const;
};
