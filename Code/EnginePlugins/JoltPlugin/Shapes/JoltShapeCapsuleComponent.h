#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using WJoltShapeCapsuleComponentManager = WComponentManager<class WJoltShapeCapsuleComponent, WBlockStorageType::FreeList>;

/// Adds a Jolt capsule shape to a Jolt actor.
class W_JOLTPLUGIN_DLL WJoltShapeCapsuleComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltShapeCapsuleComponent, WJoltShapeComponent, WJoltShapeCapsuleComponentManager);

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
  // WJoltShapeCapsuleComponent

public:
  WJoltShapeCapsuleComponent();
  ~WJoltShapeCapsuleComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
