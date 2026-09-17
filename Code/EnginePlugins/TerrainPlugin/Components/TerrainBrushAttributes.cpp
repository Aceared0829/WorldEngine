#include <TerrainPlugin/TerrainPluginPCH.h>

#include <TerrainPlugin/Components/TerrainBrushAttributes.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainBrush2DVisualizerAttribute, 1, WRTTIDefaultAllocator<WTerrainBrush2DVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InnerColor", m_InnerColor),
    W_MEMBER_PROPERTY("OuterColor", m_OuterColor),
    W_MEMBER_PROPERTY("Offset", m_vOffset),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const WColor&, const char*, const WColor&, WVec3),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTerrainBrush2DVisualizerAttribute::WTerrainBrush2DVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WTerrainBrush2DVisualizerAttribute::WTerrainBrush2DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYProp, const char* szInnerRadiusProp, const WColor& innerColor, const char* szOuterRadiusProp, const WColor& outerColor, WVec3 vLocalOffset)
  : WVisualizerAttribute(szHalfSizeXProp, szHalfSizeYProp, szInnerRadiusProp, szOuterRadiusProp)
  , m_InnerColor(innerColor)
  , m_OuterColor(outerColor)
  , m_vOffset(vLocalOffset)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTerrainBrush3DVisualizerAttribute, 1, WRTTIDefaultAllocator<WTerrainBrush3DVisualizerAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("InnerColor", m_InnerColor),
    W_MEMBER_PROPERTY("OuterColor", m_OuterColor),
    W_MEMBER_PROPERTY("Offset", m_vOffset),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const WColor&, const char*, const WColor&, WVec3),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WTerrainBrush3DVisualizerAttribute::WTerrainBrush3DVisualizerAttribute()
  : WVisualizerAttribute(nullptr)
{
}

WTerrainBrush3DVisualizerAttribute::WTerrainBrush3DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYBottomProp, const char* szHalfSizeYTopProp, const char* szHalfSizeZProp, const char* szInnerRadiusProp, const WColor& innerColor, const char* szOuterRadiusProp, const WColor& outerColor, WVec3 vLocalOffset)
  : WVisualizerAttribute(szHalfSizeXProp, szHalfSizeYBottomProp, szHalfSizeYTopProp, szHalfSizeZProp, szInnerRadiusProp, szOuterRadiusProp)
  , m_InnerColor(innerColor)
  , m_OuterColor(outerColor)
  , m_vOffset(vLocalOffset)
{
}
