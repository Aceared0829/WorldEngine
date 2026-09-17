#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

struct WMsgExtractGeometry;

struct WMsgPhysicsMakeTemporarilyDynamic;

/// Manager for WJoltStaticActorComponent.
///
/// Beyond the default component management it keeps track of the static actors that were temporarily turned into
/// dynamic ones, and copies their simulated transform back onto their owner objects.
class W_JOLTPLUGIN_DLL WJoltStaticActorComponentManager : public WComponentManager<class WJoltStaticActorComponent, WBlockStorageType::FreeList>
{
public:
  WJoltStaticActorComponentManager(WWorld* pWorld);
  ~WJoltStaticActorComponentManager();

private:
  friend class WJoltWorldModule;
  friend class WJoltStaticActorComponent;

  void UpdateTemporarilyDynamicActors();

  WDynamicArray<WComponentHandle> m_TemporarilyDynamicActors;
};

/// Turns an object into an immovable obstacle in the physics simulation.
///
/// Dynamic actors collide with static actors. Static actors cannot be moved, not even programmatically.
/// If that is desired, use a dynamic actor instead and set it to be "kinematic".
///
/// Static actors are the only ones that can use concave collision meshes.
class W_JOLTPLUGIN_DLL WJoltStaticActorComponent : public WJoltActorComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltStaticActorComponent, WJoltActorComponent, WJoltStaticActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  void OnMsgPhysicsMakeTemporarilyDynamic(WMsgPhysicsMakeTemporarilyDynamic& msg);

  //////////////////////////////////////////////////////////////////////////
  // WJoltActorComponent
protected:
  virtual void CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltStaticActorComponent

public:
  WJoltStaticActorComponent();
  ~WJoltStaticActorComponent();

  /// Searches for a sibling WMeshComponent and attempts to retrieve information about the WJoltMaterial to use for each submesh from its WMaterial information.
  void PullSurfacesFromGraphicsMesh(WDynamicArray<const WJoltMaterial*>& ref_materials);

  void SetMesh(const WJoltMeshResourceHandle& hMesh);
  W_ALWAYS_INLINE const WJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; }

  void SetSurfaceFile(WStringView sFile);      // [ property ]
  WStringView GetSurfaceFile() const;          // [ property ]

  bool m_bPullSurfacesFromGraphicsMesh = false; // [ property ]
  WSurfaceResourceHandle m_hSurface;           // [ property ]

protected:
  void OnMsgExtractGeometry(WMsgExtractGeometry& msg) const;
  const WJoltMaterial* GetJoltMaterial() const;

  /// Whether the shapes of this actor could be used for a dynamic body. Triangle meshes can't.
  bool CanBeMadeDynamic();

  WJoltMeshResourceHandle m_hCollisionMesh;

  // array to keep surfaces alive, in case they are pulled from the materials of the render mesh
  WDynamicArray<WSurfaceResourceHandle> m_UsedSurfaces;
};
