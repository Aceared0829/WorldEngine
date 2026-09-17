#pragma once

#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using WJoltShapeConvexHullComponentManager = WComponentManager<class WJoltShapeConvexHullComponent, WBlockStorageType::FreeList>;

/// Adds a Jolt convex hull shape to a Jolt actor.
///
/// A convex hull is a simple convex shape. It can be used for simulating dynamic rigid bodies.
/// Often the convex hull of a complex mesh is used to approximate the mesh and make it possible to use it as a dynamic actor.
class W_JOLTPLUGIN_DLL WJoltShapeConvexHullComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltShapeConvexHullComponent, WJoltShapeComponent, WJoltShapeConvexHullComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WJoltShapeComponent

protected:
  virtual void CreateShapes(WDynamicArray<WJoltSubShape>& out_Shapes, const WTransform& rootTransform, float fDensity, const WJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // WConvexShapeConvexComponent

public:
  WJoltShapeConvexHullComponent();
  ~WJoltShapeConvexHullComponent();

  virtual void ExtractGeometry(WMsgExtractGeometry& ref_msg) const override;

  WJoltMeshResourceHandle GetMesh() const { return m_hCollisionMesh; }

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;

  WJoltMeshResourceHandle m_hCollisionMesh;
};
