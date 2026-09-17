#pragma once

#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

/// Visualizer attribute for a 2D rounded rectangle brush shape in the XY plane.
///
/// Used on brush components to render an inner and outer rounded-rectangle outline around the brush area.
class W_TERRAINPLUGIN_DLL WTerrainBrush2DVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTerrainBrush2DVisualizerAttribute, WVisualizerAttribute);

public:
  WTerrainBrush2DVisualizerAttribute();
  WTerrainBrush2DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYProp, const char* szInnerRadiusProp, const WColor& innerColor, const char* szOuterRadiusProp, const WColor& outerColor, WVec3 vLocalOffset);

  const WUntrackedString& GetPropHalfSizeX() const { return m_sProperty1; }
  const WUntrackedString& GetPropHalfSizeY() const { return m_sProperty2; }
  const WUntrackedString& GetPropInnerRadius() const { return m_sProperty3; }
  const WUntrackedString& GetPropOuterRadius() const { return m_sProperty4; }

  WColor m_InnerColor = WColor::White;
  WColor m_OuterColor = WColor::White;
  WVec3 m_vOffset = WVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////

/// Visualizer attribute for a 3D rounded box brush shape.
///
/// The box spans [-HalfSizeX, +HalfSizeX] in X, [-HalfSizeYBottom, +HalfSizeYTop] in Y, and [-HalfSizeZ, +HalfSizeZ] in Z.
/// Top and bottom faces are drawn as rounded rectangles; four vertical edges connect them.
/// When OuterRadius > 0, a second outline is drawn using (InnerRadius + OuterRadius) as the corner radius.
class W_TERRAINPLUGIN_DLL WTerrainBrush3DVisualizerAttribute : public WVisualizerAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WTerrainBrush3DVisualizerAttribute, WVisualizerAttribute);

public:
  WTerrainBrush3DVisualizerAttribute();
  WTerrainBrush3DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYBottomProp, const char* szHalfSizeYTopProp, const char* szHalfSizeZProp, const char* szInnerRadiusProp, const WColor& innerColor, const char* szOuterRadiusProp, const WColor& outerColor, WVec3 vLocalOffset);

  const WUntrackedString& GetPropHalfSizeX() const { return m_sProperty1; }
  const WUntrackedString& GetPropHalfSizeYBottom() const { return m_sProperty2; }
  const WUntrackedString& GetPropHalfSizeYTop() const { return m_sProperty3; }
  const WUntrackedString& GetPropHalfSizeZ() const { return m_sProperty4; }
  const WUntrackedString& GetPropInnerRadius() const { return m_sProperty5; }
  const WUntrackedString& GetPropOuterRadius() const { return m_sProperty6; }

  WColor m_InnerColor = WColor::White;
  WColor m_OuterColor = WColor::White;
  WVec3 m_vOffset = WVec3::MakeZero();
};
