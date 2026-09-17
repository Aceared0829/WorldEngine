#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using WJoltShapeBoxComponentManager = WComponentManager<class WJoltShapeBoxComponent, WBlockStorageType::FreeList>;

/// Adds a Jolt box shape to a Jolt actor.
class W_JOLTPLUGIN_DLL WJoltShapeBoxComponent : public WJoltShapeComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltShapeBoxComponent, WJoltShapeComponent, WJoltShapeBoxComponentManager);

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
  // WJoltShapeBoxComponent

public:
  WJoltShapeBoxComponent();
  ~WJoltShapeBoxComponent();

  void SetHalfExtents(const WVec3& value);                       // [ property ]
  const WVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  virtual void ExtractGeometry(WMsgExtractGeometry& ref_msg) const override;

protected:
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;

  WVec3 m_vHalfExtents = WVec3(0.5f);
};
