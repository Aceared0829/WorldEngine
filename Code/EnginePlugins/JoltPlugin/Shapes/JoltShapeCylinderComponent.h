#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using WJoltShapeCylinderComponentManager = WComponentManager<class WJoltShapeCylinderComponent, WBlockStorageType::FreeList>;

/// Adds a Jolt cylinder shape to a Jolt actor.
///
/// Be aware that the cylinder shape is not as stable in simulation as other shapes.
/// If possible use capsule shapes instead. In some cases even using a convex hull shape may provide better results,
/// but this has to be tried out case by case.
class W_JOLTPLUGIN_DLL WJoltShapeCylinderComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltShapeCylinderComponent, WJoltShapeComponent, WJoltShapeCylinderComponentManager);

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
  // WJoltShapeCylinderComponent

public:
  WJoltShapeCylinderComponent();
  ~WJoltShapeCylinderComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
