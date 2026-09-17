#pragma once

#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>

/// Brush modes available on a 3D (volumetric) terrain brush.
struct W_TERRAINPLUGIN_DLL WTerrainModifyMode3D
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Carve = 3,
    Add = 4,
    OnlyPaint3D = 6,
    Default = Carve,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_TERRAINPLUGIN_DLL, WTerrainModifyMode3D);

using WTerrainBrush3DComponentManager = WComponentManager<class WTerrainBrush3DComponent, WBlockStorageType::Compact>;

/// 3D (volumetric) terrain brush.
///
/// The footprint is a 3D rounded box oriented by the owner object's rotation.
/// HalfSizeX / HalfSizeYBottom / HalfSizeYTop / HalfSizeZ control the box extents.
/// Setting HalfSizeYTop=0 keeps it equal to HalfSizeYBottom (symmetric).
/// Setting HalfSizeYTop=0 and HalfSizeYBottom to a positive value with a non-zero InnerRadius
/// produces an arch cross-section suitable for natural tunnels with flat floors.
///
/// If an WSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single point.
class W_TERRAINPLUGIN_DLL WTerrainBrush3DComponent : public WTerrainBrushBaseComponent
{
  W_DECLARE_COMPONENT_TYPE(WTerrainBrush3DComponent, WTerrainBrushBaseComponent, WTerrainBrush3DComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WTerrainBrush3DComponent

public:
  WTerrainBrush3DComponent();
  ~WTerrainBrush3DComponent();

  void SetModifyMode(WEnum<WTerrainModifyMode3D> mode);                      //< [ property ]
  WEnum<WTerrainModifyMode3D> GetModifyMode() const { return m_ModifyMode; } //< [ property ]

  void SetHalfSizeYBottom(float fSize);                                        //< [ property ]
  float GetHalfSizeYBottom() const { return m_fHalfSizeYBottom; }              //< [ property ]

  /// Upper Y half-size. 0 = same as HalfSizeYBottom (symmetric box).
  void SetHalfSizeYTop(float fSize);                        //< [ property ]
  float GetHalfSizeYTop() const { return m_fHalfSizeYTop; } //< [ property ]

  void SetHalfSizeZ(float fSize);                           //< [ property ]
  float GetHalfSizeZ() const { return m_fHalfSizeZ; }       //< [ property ]

protected:
  virtual void FillBrushSpecificProperties(WTerrainData_Brush& brush, float fHalfSizeX) override;

  WEnum<WTerrainModifyMode3D> m_ModifyMode;
  float m_fHalfSizeYBottom = 0.0f;
  float m_fHalfSizeYTop = 0.0f;
  float m_fHalfSizeZ = 0.0f;
};
