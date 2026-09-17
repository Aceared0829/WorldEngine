#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <TerrainPlugin/Components/TerrainBrush3DComponent.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTerrainModifyMode3D, 1)
  W_ENUM_CONSTANT(WTerrainModifyMode3D::OnlyPaint3D),
  W_ENUM_CONSTANT(WTerrainModifyMode3D::Carve),
  W_ENUM_CONSTANT(WTerrainModifyMode3D::Add),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WTerrainBrush3DComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("ModifyMode", WTerrainModifyMode3D, GetModifyMode, SetModifyMode),
    W_ACCESSOR_PROPERTY("HalfSizeX", GetHalfSizeX, SetHalfSizeX)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("HalfSizeYTop", GetHalfSizeYTop, SetHalfSizeYTop)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("HalfSizeYBottom", GetHalfSizeYBottom, SetHalfSizeYBottom)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("HalfSizeZ", GetHalfSizeZ, SetHalfSizeZ)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("InnerRadius", GetInnerRadius, SetInnerRadius)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("OuterRadius", GetOuterRadius, SetOuterRadius)->AddAttributes(new WDefaultValueAttribute(5.0f), new WClampValueAttribute(0.0f, WVariant())),
    W_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.1f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoiseStrength", GetNoiseStrength, SetNoiseStrength)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 10.0f)),
    W_ACCESSOR_PROPERTY("NoiseFrequency", GetNoiseFrequency, SetNoiseFrequency)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(1.0f, 100.0f)),
    W_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new WClampValueAttribute(0, 15)),
    W_ACCESSOR_PROPERTY("MaterialStrength", GetMaterialStrength, SetMaterialStrength)->AddAttributes(new WDefaultValueAttribute(0.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ACCESSOR_PROPERTY("AffectPatches", GetAffectPatches, SetAffectPatches)->AddAttributes(new WDefaultValueAttribute(false)),
    W_ACCESSOR_PROPERTY("AffectVolumes", GetAffectVolumes, SetAffectVolumes)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new WDefaultValueAttribute((WInt8)0), new WClampValueAttribute((WInt8)-8, (WInt8)8)),
    W_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new WTagSetWidgetAttribute("Terrain")),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Terrain"),
    new WShapeIconAlwaysVisibleAttribute(),
    new WTerrainBrush3DVisualizerAttribute("HalfSizeX", "HalfSizeYBottom", "HalfSizeYTop", "HalfSizeZ", "InnerRadius", WColor::Yellow, "OuterRadius", WColor::GreenYellow, WVec3(0, 0, 0)),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WTerrainBrush3DComponent::WTerrainBrush3DComponent() = default;
WTerrainBrush3DComponent::~WTerrainBrush3DComponent() = default;

void WTerrainBrush3DComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_ModifyMode;
  s << m_fHalfSizeYBottom;
  s << m_fHalfSizeYTop;
  s << m_fHalfSizeZ;
}

void WTerrainBrush3DComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_ModifyMode;
  s >> m_fHalfSizeYBottom;
  s >> m_fHalfSizeYTop;
  s >> m_fHalfSizeZ;
}

void WTerrainBrush3DComponent::SetModifyMode(WEnum<WTerrainModifyMode3D> mode)
{
  if (m_ModifyMode == mode)
    return;
  m_ModifyMode = mode;
  RefreshBrushes();
}

void WTerrainBrush3DComponent::SetHalfSizeYBottom(float fSize)
{
  if (m_fHalfSizeYBottom == fSize)
    return;
  m_fHalfSizeYBottom = fSize;
  RefreshBrushes();
}

void WTerrainBrush3DComponent::SetHalfSizeYTop(float fSize)
{
  if (m_fHalfSizeYTop == fSize)
    return;
  m_fHalfSizeYTop = fSize;
  RefreshBrushes();
}

void WTerrainBrush3DComponent::SetHalfSizeZ(float fSize)
{
  if (m_fHalfSizeZ == fSize)
    return;
  m_fHalfSizeZ = fSize;
  RefreshBrushes();
}

void WTerrainBrush3DComponent::FillBrushSpecificProperties(WTerrainData_Brush& brush, float /*fHalfSizeX*/)
{
  brush.m_ModifyMode = static_cast<WTerrainModifyMode::Enum>(m_ModifyMode.GetValue());
  brush.m_vHalfExtents.y = m_fHalfSizeYBottom;
  brush.m_fHalfExtentYTop = m_fHalfSizeYTop;
  brush.m_fHalfExtentZ = m_fHalfSizeZ;
}
