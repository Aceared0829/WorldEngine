#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <ProcGenPlugin/Declarations.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WProcGenBinaryOperator, 1)
  W_ENUM_CONSTANTS(WProcGenBinaryOperator::Add, WProcGenBinaryOperator::Subtract, WProcGenBinaryOperator::Multiply, WProcGenBinaryOperator::Divide)
  W_ENUM_CONSTANTS(WProcGenBinaryOperator::Max, WProcGenBinaryOperator::Min)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WProcGenBlendMode, 1)
  W_ENUM_CONSTANTS(WProcGenBlendMode::Add, WProcGenBlendMode::Subtract, WProcGenBlendMode::Multiply, WProcGenBlendMode::Divide)
  W_ENUM_CONSTANTS(WProcGenBlendMode::Max, WProcGenBlendMode::Min)
  W_ENUM_CONSTANTS(WProcGenBlendMode::Set)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WProcVertexColorChannelMapping, 1)
  W_ENUM_CONSTANTS(WProcVertexColorChannelMapping::R, WProcVertexColorChannelMapping::G, WProcVertexColorChannelMapping::B, WProcVertexColorChannelMapping::A)
  W_ENUM_CONSTANTS(WProcVertexColorChannelMapping::Black, WProcVertexColorChannelMapping::White)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WProcVertexColorMapping, WNoBase, 1, WRTTIDefaultAllocator<WProcVertexColorMapping>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("R", WProcVertexColorChannelMapping, m_R)->AddAttributes(new WDefaultValueAttribute(WProcVertexColorChannelMapping::R)),
    W_ENUM_MEMBER_PROPERTY("G", WProcVertexColorChannelMapping, m_G)->AddAttributes(new WDefaultValueAttribute(WProcVertexColorChannelMapping::G)),
    W_ENUM_MEMBER_PROPERTY("B", WProcVertexColorChannelMapping, m_B)->AddAttributes(new WDefaultValueAttribute(WProcVertexColorChannelMapping::B)),
    W_ENUM_MEMBER_PROPERTY("A", WProcVertexColorChannelMapping, m_A)->AddAttributes(new WDefaultValueAttribute(WProcVertexColorChannelMapping::A)),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WProcPlacementMode, 1)
  W_ENUM_CONSTANTS(WProcPlacementMode::Raycast, WProcPlacementMode::RaycastHighQuality, WProcPlacementMode::Fixed)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WProcPlacementPattern, 1)
  W_ENUM_CONSTANTS(WProcPlacementPattern::RegularGrid, WProcPlacementPattern::HexGrid, WProcPlacementPattern::Natural)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WProcVolumeImageMode, 1)
  W_ENUM_CONSTANTS(WProcVolumeImageMode::ReferenceColor, WProcVolumeImageMode::ChannelR, WProcVolumeImageMode::ChannelG, WProcVolumeImageMode::ChannelB, WProcVolumeImageMode::ChannelA)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

static WTypeVersion s_ProcVertexColorMappingVersion = 1;
WResult WProcVertexColorMapping::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorMappingVersion);
  inout_stream << m_R;
  inout_stream << m_G;
  inout_stream << m_B;
  inout_stream << m_A;

  return W_SUCCESS;
}

WResult WProcVertexColorMapping::Deserialize(WStreamReader& inout_stream)
{
  /*WTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorMappingVersion);
  inout_stream >> m_R;
  inout_stream >> m_G;
  inout_stream >> m_B;
  inout_stream >> m_A;

  return W_SUCCESS;
}

namespace WProcGenInternal
{
  GraphSharedDataBase::~GraphSharedDataBase() = default;
  Output::~Output() = default;

  WHashedString ExpressionInputs::s_sPosition = WMakeHashedString("position");
  WHashedString ExpressionInputs::s_sPositionX = WMakeHashedString("position.x");
  WHashedString ExpressionInputs::s_sPositionY = WMakeHashedString("position.y");
  WHashedString ExpressionInputs::s_sPositionZ = WMakeHashedString("position.z");
  WHashedString ExpressionInputs::s_sNormal = WMakeHashedString("normal");
  WHashedString ExpressionInputs::s_sNormalX = WMakeHashedString("normal.x");
  WHashedString ExpressionInputs::s_sNormalY = WMakeHashedString("normal.y");
  WHashedString ExpressionInputs::s_sNormalZ = WMakeHashedString("normal.z");
  WHashedString ExpressionInputs::s_sColor = WMakeHashedString("color");
  WHashedString ExpressionInputs::s_sColorR = WMakeHashedString("color.x");
  WHashedString ExpressionInputs::s_sColorG = WMakeHashedString("color.y");
  WHashedString ExpressionInputs::s_sColorB = WMakeHashedString("color.z");
  WHashedString ExpressionInputs::s_sColorA = WMakeHashedString("color.w");
  WHashedString ExpressionInputs::s_sPointIndex = WMakeHashedString("pointIndex");

  WHashedString ExpressionOutputs::s_sOutDensity = WMakeHashedString("outDensity");
  WHashedString ExpressionOutputs::s_sOutScale = WMakeHashedString("outScale");
  WHashedString ExpressionOutputs::s_sOutColorIndex = WMakeHashedString("outColorIndex");
  WHashedString ExpressionOutputs::s_sOutObjectIndex = WMakeHashedString("outObjectIndex");

  WHashedString ExpressionOutputs::s_sOutColor = WMakeHashedString("outColor");
  WHashedString ExpressionOutputs::s_sOutColorR = WMakeHashedString("outColor.x");
  WHashedString ExpressionOutputs::s_sOutColorG = WMakeHashedString("outColor.y");
  WHashedString ExpressionOutputs::s_sOutColorB = WMakeHashedString("outColor.z");
  WHashedString ExpressionOutputs::s_sOutColorA = WMakeHashedString("outColor.w");
} // namespace WProcGenInternal


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Declarations);
