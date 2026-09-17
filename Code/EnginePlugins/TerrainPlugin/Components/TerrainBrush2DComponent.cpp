#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <TerrainPlugin/Components/TerrainBrush2DComponent.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTerrainModifyMode2D, 1)
  W_ENUM_CONSTANT(WTerrainModifyMode2D::OnlyPaint2D),
  W_ENUM_CONSTANT(WTerrainModifyMode2D::Max),
  W_ENUM_CONSTANT(WTerrainModifyMode2D::Min),
  W_ENUM_CONSTANT(WTerrainModifyMode2D::Set),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WTerrainBrush2DComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("ModifyMode", WTerrainModifyMode2D, GetModifyMode, SetModifyMode),
    W_ACCESSOR_PROPERTY("HalfSizeX", GetHalfSizeX, SetHalfSizeX)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("HalfSizeY", GetHalfSizeY, SetHalfSizeY)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("InnerRadius", GetInnerRadius, SetInnerRadius)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("OuterRadius", GetOuterRadius, SetOuterRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoiseStrength", GetNoiseStrength, SetNoiseStrength)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoiseFrequency", GetNoiseFrequency, SetNoiseFrequency)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(1.0f, 100.0f)),
    W_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new WClampValueAttribute(0, 15)),
    W_ACCESSOR_PROPERTY("MaterialStrength", GetMaterialStrength, SetMaterialStrength)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("AffectPatches", GetAffectPatches, SetAffectPatches)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("AffectVolumes", GetAffectVolumes, SetAffectVolumes)->AddAttributes(new WDefaultValueAttribute(false)),
    W_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new WDefaultValueAttribute((WInt8)0), new WClampValueAttribute((WInt8)-8, (WInt8)8)),
    W_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new WTagSetWidgetAttribute("Terrain")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Terrain"),
    new WShapeIconAlwaysVisibleAttribute(),
    new WTerrainBrush2DVisualizerAttribute("HalfSizeX", "HalfSizeY", "InnerRadius", WColor::Yellow, "OuterRadius", WColor::GreenYellow, WVec3(0, 0, 0)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WTerrainBrush2DComponent::WTerrainBrush2DComponent() = default;
WTerrainBrush2DComponent::~WTerrainBrush2DComponent() = default;

void WTerrainBrush2DComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_ModifyMode;
  s << m_fHalfSizeY;
}

void WTerrainBrush2DComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_ModifyMode;
  s >> m_fHalfSizeY;
}

void WTerrainBrush2DComponent::SetModifyMode(WEnum<WTerrainModifyMode2D> mode)
{
  if (m_ModifyMode == mode)
    return;
  m_ModifyMode = mode;
  RefreshBrushes();
}

void WTerrainBrush2DComponent::SetHalfSizeY(float fSize)
{
  if (m_fHalfSizeY == fSize)
    return;
  m_fHalfSizeY = fSize;
  RefreshBrushes();
}

void WTerrainBrush2DComponent::FillBrushSpecificProperties(WTerrainData_Brush& brush, float /*fHalfSizeX*/)
{
  brush.m_ModifyMode = static_cast<WTerrainModifyMode::Enum>(m_ModifyMode.GetValue());
  brush.m_vHalfExtents.y = m_fHalfSizeY;
  brush.m_fHalfExtentYTop = 0.0f;
  brush.m_fHalfExtentZ = 0.0f;
}
