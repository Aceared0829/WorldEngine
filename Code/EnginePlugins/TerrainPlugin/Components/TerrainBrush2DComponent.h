#pragma once

#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>

/// Brush modes available on a 2D (heightfield-style) terrain brush.
struct W_TERRAINPLUGIN_DLL WTerrainModifyMode2D
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Max = 0,         ///< Only raises terrain toward the brush height.
    Min = 1,         ///< Only lowers terrain toward the brush height.
    Set = 2,         ///< Forces terrain to the brush height.
    OnlyPaint2D = 5, ///< Does not modify height values; paints material using 2D SDF projection.
    Default = Max,
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_TERRAINPLUGIN_DLL, WTerrainModifyMode2D);

using WTerrainBrush2DComponentManager = WComponentManager<class WTerrainBrush2DComponent, WBlockStorageType::Compact>;

/// 2D terrain brush: raises, lowers, or sets the heightfield surface.
///
/// The footprint is a rounded rectangle oriented by the owner object's rotation.
/// HalfSizeX/HalfSizeY control the straight-edge lengths; OuterRadius is the corner rounding radius.
/// Setting HalfSizeX=HalfSizeY=0 produces a circle; OuterRadius=0 gives a plain rectangle.
///
/// If an WSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single point.
class W_TERRAINPLUGIN_DLL WTerrainBrush2DComponent : public WTerrainBrushBaseComponent
{
  W_DECLARE_COMPONENT_TYPE(WTerrainBrush2DComponent, WTerrainBrushBaseComponent, WTerrainBrush2DComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WTerrainBrush2DComponent

public:
  WTerrainBrush2DComponent();
  ~WTerrainBrush2DComponent();

  void SetModifyMode(WEnum<WTerrainModifyMode2D> mode);                      //< [ property ]
  WEnum<WTerrainModifyMode2D> GetModifyMode() const { return m_ModifyMode; } //< [ property ]

  void SetHalfSizeY(float fSize);                                              //< [ property ]
  float GetHalfSizeY() const { return m_fHalfSizeY; }                          //< [ property ]

protected:
  virtual void FillBrushSpecificProperties(WTerrainData_Brush& brush, float fHalfSizeX) override;

  WEnum<WTerrainModifyMode2D> m_ModifyMode;
  float m_fHalfSizeY = 0.0f;
};
