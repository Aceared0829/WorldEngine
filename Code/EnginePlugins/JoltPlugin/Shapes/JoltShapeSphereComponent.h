#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using WJoltShapeSphereComponentManager = WComponentManager<class WJoltShapeSphereComponent, WBlockStorageType::FreeList>;

/// Adds a Jolt sphere shape to a Jolt actor.
class W_JOLTPLUGIN_DLL WJoltShapeSphereComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltShapeSphereComponent, WJoltShapeComponent, WJoltShapeSphereComponentManager);

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
  // WJoltShapeSphereComponent

public:
  WJoltShapeSphereComponent();
  ~WJoltShapeSphereComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
};
