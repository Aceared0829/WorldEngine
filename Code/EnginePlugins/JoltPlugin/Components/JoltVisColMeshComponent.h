#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class WJoltVisColMeshComponentManager : public WComponentManager<class WJoltVisColMeshComponent, WBlockStorageType::Compact>
{
public:
  using SUPER = WComponentManager<WJoltVisColMeshComponent, WBlockStorageType::Compact>;

  WJoltVisColMeshComponentManager(WWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const WWorldModule::UpdateContext& context);
  void EnqueueUpdate(WComponentHandle hComponent);

private:
  void ResourceEventHandler(const WResourceEvent& e);

  mutable WMutex m_Mutex;
  WDeque<WComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

/// Visualizes a Jolt collision mesh that is attached to the same game object.
///
/// When attached to a game object where a WJoltStaticActorComponent or a WJoltShapeConvexHullComponent is attached as well,
/// this component will retrieve the triangle mesh and turn it into a render mesh.
///
/// This is used for displaying the collision mesh of a single object.
/// It doesn't work for non-mesh shape types (sphere, box, capsule).
class W_JOLTPLUGIN_DLL WJoltVisColMeshComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltVisColMeshComponent, WRenderComponent, WJoltVisColMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltVisColMeshComponent

public:
  WJoltVisColMeshComponent();
  ~WJoltVisColMeshComponent();

  /// If this is set directly, the mesh is not taken from the sibling components.
  void SetMesh(const WJoltMeshResourceHandle& hMesh);                                          // [ property ]
  W_ALWAYS_INLINE const WJoltMeshResourceHandle& GetMesh() const { return m_hCollisionMesh; } // [ property ]

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;
  void CreateCollisionRenderMesh();

  WJoltMeshResourceHandle m_hCollisionMesh;
  WMeshResourceHandle m_hMesh;

  mutable WInstanceDataOffset m_InstanceDataOffset;
};
