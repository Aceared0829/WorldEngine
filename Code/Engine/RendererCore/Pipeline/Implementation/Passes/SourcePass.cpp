#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSourcePass, 4, WRTTIDefaultAllocator<WSourcePass>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Output", m_PinOutput),
    W_ENUM_MEMBER_PROPERTY("Type", WRequiredTextureType, m_Type),
    W_ENUM_MEMBER_PROPERTY("Precision", WRequiredTexturePrecision, m_MinPrecision),
    W_ENUM_MEMBER_PROPERTY("Channels", WRequiredTextureChannels, m_MinChannels),
    W_ENUM_MEMBER_PROPERTY("MSAA_Mode", WGALMSAASampleCount, m_MsaaMode),
    W_MEMBER_PROPERTY("UAV", m_bUAV),
    W_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new WExposeColorAlphaAttribute()),
    W_MEMBER_PROPERTY("ClearDepth", m_fClearDepth)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_MEMBER_PROPERTY("Clear", m_bClear),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Input")
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WRequiredTexturePrecision, 1)
  W_ENUM_CONSTANTS(
    WRequiredTexturePrecision::Bits_5,
    WRequiredTexturePrecision::Bits_8,
    WRequiredTexturePrecision::Bits_10,
    WRequiredTexturePrecision::Bits_16,
    WRequiredTexturePrecision::Bits_24,
    WRequiredTexturePrecision::Bits_32)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WRequiredTextureType, 1)
  W_ENUM_CONSTANTS(
    WRequiredTextureType::UNorm,
    WRequiredTextureType::SNorm,
    WRequiredTextureType::SRGB,
    WRequiredTextureType::UInt,
    WRequiredTextureType::SInt,
    WRequiredTextureType::Float,
    WRequiredTextureType::Depth)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WRequiredTextureChannels, 1)
  W_ENUM_CONSTANTS(
    WRequiredTextureChannels::Channels_1,
    WRequiredTextureChannels::Channels_2,
    WRequiredTextureChannels::Channels_3,
    WRequiredTextureChannels::Channels_4)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

WSourcePass::WSourcePass(const char* szName)
  : WRenderPipelinePass(szName, true)
{
}

WSourcePass::~WSourcePass() = default;

namespace
{
  /// The removed WSourceFormat enum. Only kept around to be able to read data that was written before texture requirements replaced it.
  enum class LegacySourceFormat : WUInt32
  {
    Color4Channel8BitNormalized_sRGB = 0,
    Color4Channel8BitNormalized = 1,
    Color4Channel16BitFloat = 2,
    Color4Channel32BitFloat = 3,
    Color3Channel11_11_10BitFloat = 4,
    Depth16Bit = 5,
    Depth24BitStencil8Bit = 6,
    Depth32BitFloat = 7,
  };

  struct TextureFormat
  {
    WRequiredTextureChannels::Enum m_Channels;
    WRequiredTexturePrecision::Enum m_Precision;
    WGALResourceFormat::Enum m_Format;
  };

  static WBitflags<WGALResourceFormatSupport> GetRequiredFormatSupport(WEnum<WGALMSAASampleCount> msaaMode, bool bUAV)
  {
    WBitflags<WGALResourceFormatSupport> support = WGALResourceFormatSupport::RenderTarget | WGALResourceFormatSupport::Texture;
    if (bUAV)
      support.Add(WGALResourceFormatSupport::TextureRW);

    switch (msaaMode)
    {
      case WGALMSAASampleCount::TwoSamples:
        support.Add(WGALResourceFormatSupport::MSAA2x);
        break;
      case WGALMSAASampleCount::FourSamples:
        support.Add(WGALResourceFormatSupport::MSAA4x);
        break;
      case WGALMSAASampleCount::EightSamples:
        support.Add(WGALResourceFormatSupport::MSAA8x);
        break;
      default:
        break;
    }
    return support;
  }

  static void GetLegacyFormatRequirements(LegacySourceFormat format, WEnum<WRequiredTextureType>& out_type, WEnum<WRequiredTexturePrecision>& out_precision, WEnum<WRequiredTextureChannels>& out_channels)
  {
    out_channels = WRequiredTextureChannels::Channels_4;
    switch (format)
    {
      case LegacySourceFormat::Color4Channel8BitNormalized_sRGB:
        out_type = WRequiredTextureType::SRGB;
        out_precision = WRequiredTexturePrecision::Bits_8;
        break;
      case LegacySourceFormat::Color4Channel8BitNormalized:
        out_type = WRequiredTextureType::UNorm;
        out_precision = WRequiredTexturePrecision::Bits_8;
        break;
      case LegacySourceFormat::Color4Channel16BitFloat:
        out_type = WRequiredTextureType::Float;
        out_precision = WRequiredTexturePrecision::Bits_16;
        break;
      case LegacySourceFormat::Color4Channel32BitFloat:
        out_type = WRequiredTextureType::Float;
        out_precision = WRequiredTexturePrecision::Bits_32;
        break;
      case LegacySourceFormat::Color3Channel11_11_10BitFloat:
        out_type = WRequiredTextureType::Float;
        out_precision = WRequiredTexturePrecision::Bits_10;
        out_channels = WRequiredTextureChannels::Channels_3;
        break;
      case LegacySourceFormat::Depth16Bit:
        out_type = WRequiredTextureType::Depth;
        out_precision = WRequiredTexturePrecision::Bits_16;
        out_channels = WRequiredTextureChannels::Channels_1;
        break;
      case LegacySourceFormat::Depth24BitStencil8Bit:
        out_type = WRequiredTextureType::Depth;
        out_precision = WRequiredTexturePrecision::Bits_24;
        out_channels = WRequiredTextureChannels::Channels_2;
        break;
      case LegacySourceFormat::Depth32BitFloat:
        out_type = WRequiredTextureType::Depth;
        out_precision = WRequiredTexturePrecision::Bits_32;
        out_channels = WRequiredTextureChannels::Channels_1;
        break;
      default:
        out_type = WRequiredTextureType::Default;
        out_precision = WRequiredTexturePrecision::Default;
        out_channels = WRequiredTextureChannels::Default;
        break;
    }
  }

  static LegacySourceFormat GetLegacySourceFormat(WGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case WGALResourceFormat::RGBAHalf:
        return LegacySourceFormat::Color4Channel16BitFloat;
      case WGALResourceFormat::RGBAFloat:
        return LegacySourceFormat::Color4Channel32BitFloat;
      case WGALResourceFormat::RG11B10Float:
        return LegacySourceFormat::Color3Channel11_11_10BitFloat;
      case WGALResourceFormat::D16:
        return LegacySourceFormat::Depth16Bit;
      case WGALResourceFormat::D24S8:
        return LegacySourceFormat::Depth24BitStencil8Bit;
      case WGALResourceFormat::DFloat:
        return LegacySourceFormat::Depth32BitFloat;
      case WGALResourceFormat::RGBAUByteNormalized:
      case WGALResourceFormat::BGRAUByteNormalized:
        return LegacySourceFormat::Color4Channel8BitNormalized;
      default:
        return LegacySourceFormat::Color4Channel8BitNormalized_sRGB;
    }
  }
} // namespace

void WGetLegacyTextureFormatRequirements(WUInt32 uiLegacyValue, bool bGalResourceFormat, WEnum<WRequiredTextureType>& out_type, WEnum<WRequiredTexturePrecision>& out_precision, WEnum<WRequiredTextureChannels>& out_channels)
{
  const LegacySourceFormat format = bGalResourceFormat ? GetLegacySourceFormat(static_cast<WGALResourceFormat::Enum>(uiLegacyValue)) : static_cast<LegacySourceFormat>(uiLegacyValue);
  GetLegacyFormatRequirements(format, out_type, out_precision, out_channels);
}

WEnum<WGALResourceFormat> WSourcePass::FindFormat(WEnum<WRequiredTextureType> type, WEnum<WRequiredTexturePrecision> minPrecision, WEnum<WRequiredTextureChannels> minChannels, WBitflags<WGALResourceFormatSupport> requiredSupport)
{
  static constexpr TextureFormat unormTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RUByteNormalized},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RUShortNormalized},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGUByteNormalized},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGUShortNormalized},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_5, WGALResourceFormat::B5G6R5UNormalized},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_10, WGALResourceFormat::RGB10A2UIntNormalized},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::BGRAUByteNormalized},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGBAUByteNormalized},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGBAUShortNormalized}};
  static constexpr TextureFormat snormTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RByteNormalized},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RShortNormalized},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGByteNormalized},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGShortNormalized},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGBAByteNormalized},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGBAShortNormalized}};
  static constexpr TextureFormat srgbTextures[] = {
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::BGRAUByteNormalizedsRGB},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGBAUByteNormalizedsRGB}};
  static constexpr TextureFormat uintTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RUByte},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RUShort},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RUInt},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGUByte},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGUShort},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGUInt},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_10, WGALResourceFormat::RGB10A2UInt},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBUInt},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGBAUByte},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGBAUShort},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBAUInt}};
  static constexpr TextureFormat sintTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RByte},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RShort},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RInt},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGByte},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGShort},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGInt},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBInt},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_8, WGALResourceFormat::RGBAByte},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGBAShort},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBAInt}};
  static constexpr TextureFormat floatTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RHalf},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RFloat},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGHalf},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGFloat},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_10, WGALResourceFormat::RG11B10Float},
    {WRequiredTextureChannels::Channels_3, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBFloat},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::RGBAHalf},
    {WRequiredTextureChannels::Channels_4, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::RGBAFloat}};
  static constexpr TextureFormat depthTextures[] = {
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_16, WGALResourceFormat::D16},
    {WRequiredTextureChannels::Channels_1, WRequiredTexturePrecision::Bits_32, WGALResourceFormat::DFloat},
    {WRequiredTextureChannels::Channels_2, WRequiredTexturePrecision::Bits_24, WGALResourceFormat::D24S8}};

  WArrayPtr<const TextureFormat> formats;
  switch (type)
  {
    case WRequiredTextureType::UNorm:
      formats = unormTextures;
      break;
    case WRequiredTextureType::SNorm:
      formats = snormTextures;
      break;
    case WRequiredTextureType::SRGB:
      formats = srgbTextures;
      break;
    case WRequiredTextureType::UInt:
      formats = uintTextures;
      break;
    case WRequiredTextureType::SInt:
      formats = sintTextures;
      break;
    case WRequiredTextureType::Float:
      formats = floatTextures;
      break;
    case WRequiredTextureType::Depth:
      formats = depthTextures;
      break;
    default:
      return WGALResourceFormat::Invalid;
  }

  const WGALDeviceCapabilities& caps = WGALDevice::GetDefaultDevice()->GetCapabilities();
  for (const TextureFormat& candidate : formats)
  {
    if (candidate.m_Channels >= minChannels && candidate.m_Precision >= minPrecision && caps.m_FormatSupport[candidate.m_Format].AreAllSet(requiredSupport))
      return candidate.m_Format;
  }
  return WGALResourceFormat::Invalid;
}

WStatus WSourcePass::GetOutputDescription(const WViewData& viewData, const WCamera& camera, WEnum<WRequiredTextureType> type, WEnum<WRequiredTexturePrecision> minPrecision, WEnum<WRequiredTextureChannels> minChannels, WEnum<WGALMSAASampleCount> msaaMode, bool bUAV, WGALTextureCreationDescription& out_desc)
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  const WBitflags<WGALResourceFormatSupport> requiredSupport = GetRequiredFormatSupport(msaaMode, bUAV);
  WEnum<WGALResourceFormat> format = FindFormat(type, minPrecision, minChannels, requiredSupport);
  if (format == WGALResourceFormat::Invalid)
  {
    return WStatus(WFmt("No texture format found for type '{}', precision '{}', channels '{}', MSAA '{}', UAV '{}'.", WArgEnum(type), WArgEnum(minPrecision), WArgEnum(minChannels), WArgEnum(msaaMode), bUAV));
  }

  // Match the active render target's channel order when it satisfies the exact common backbuffer requirements.
  if (minChannels == WRequiredTextureChannels::Channels_4 && minPrecision == WRequiredTexturePrecision::Bits_8 &&
      (type == WRequiredTextureType::UNorm || type == WRequiredTextureType::SRGB))
  {
    const WGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();
    if (const WGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const WGALTextureCreationDescription& renderTargetDesc = pTexture->GetDescription();
      const WGALResourceFormat::Enum preferredFormat = renderTargetDesc.m_Format;
      const bool bMatchingType = (type == WRequiredTextureType::SRGB) == WGALResourceFormat::IsSrgb(preferredFormat);
      if (bMatchingType && !WGALResourceFormat::IsIntegerFormat(preferredFormat) && !WGALResourceFormat::IsDepthFormat(preferredFormat) &&
          WGALResourceFormat::GetChannelCount(preferredFormat) == 4 && WGALResourceFormat::GetBitsPerElement(preferredFormat) == 32 && pDevice->GetCapabilities().m_FormatSupport[preferredFormat].AreAllSet(requiredSupport))
      {
        format = preferredFormat;
      }
    }
  }

  out_desc.SetAsRenderTarget(static_cast<WUInt32>(viewData.m_ViewPortRect.width), static_cast<WUInt32>(viewData.m_ViewPortRect.height), camera.IsStereoscopic() ? 2 : 1, format, msaaMode);
  out_desc.m_Type = WGALTextureType::Texture2DArray;
  if (bUAV)
    out_desc.m_TextureFlags.Add(WGALTextureUsageFlags::UnorderedAccess);

  return W_SUCCESS;
}

WStatus WSourcePass::AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs)
{
  WGALTextureCreationDescription desc;
  W_SUCCEED_OR_RETURN(GetOutputDescription(viewData, camera, m_Type, m_MinPrecision, m_MinChannels, m_MsaaMode, m_bUAV, desc));
  WRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(desc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  if (m_bClear)
  {
    if (WGALResourceFormat::IsDepthFormat(desc.m_Format))
    {
      auto pass = ref_graph.AddGraphicsPass("ClearDepth");
      pass.AddDepthStencilTarget(hOutput, {}, WGALRenderTargetLoadOp::Clear, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearDepth(m_fClearDepth);
      pass.SetClearStencil();
    }
    else
    {
      auto pass = ref_graph.AddGraphicsPass("ClearColor");
      pass.AddColorTarget(hOutput, {}, WGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, m_ClearColor);
    }
  }

  return W_SUCCESS;
}

WResult WSourcePass::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Type;
  inout_stream << m_MinPrecision;
  inout_stream << m_MinChannels;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_fClearDepth;
  inout_stream << m_bClear;
  inout_stream << m_bUAV;
  return W_SUCCESS;
}

WResult WSourcePass::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  if (uiVersion >= 4)
  {
    inout_stream >> m_Type;
    inout_stream >> m_MinPrecision;
    inout_stream >> m_MinChannels;
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_fClearDepth;
    inout_stream >> m_bClear;
    inout_stream >> m_bUAV;
  }
  else
  {
    // Version 3 stored the removed WSourceFormat, everything before that an WGALResourceFormat. Both use WUInt8 as storage.
    WUInt8 uiLegacyFormat = 0;
    inout_stream >> uiLegacyFormat;
    WGetLegacyTextureFormatRequirements(uiLegacyFormat, uiVersion < 3, m_Type, m_MinPrecision, m_MinChannels);

    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_bClear;
  }
  return W_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

namespace
{
  /// Resolves a name of the removed WSourceFormat enum back to its former integer value.
  static WUInt32 GetLegacySourceFormatValue(WStringView sName)
  {
    static constexpr WStringView names[] = {
      "Color4Channel8BitNormalized_sRGB"_wsv,
      "Color4Channel8BitNormalized"_wsv,
      "Color4Channel16BitFloat"_wsv,
      "Color4Channel32BitFloat"_wsv,
      "Color3Channel11_11_10BitFloat"_wsv,
      "Depth16Bit"_wsv,
      "Depth24BitStencil8Bit"_wsv,
      "Depth32BitFloat"_wsv,
    };

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(names); ++i)
    {
      if (sName.EndsWith(names[i]))
        return i;
    }

    return static_cast<WUInt32>(LegacySourceFormat::Color4Channel8BitNormalized_sRGB);
  }
} // namespace

void WPatchLegacyTextureFormatProperty(WAbstractObjectNode* pNode, bool bGalResourceFormat)
{
  const WAbstractObjectNode::Property* pFormat = pNode->FindProperty("Format");
  if (pFormat == nullptr)
    return;

  const WString sFormat = pFormat->m_Value.ConvertTo<WString>();
  WUInt32 uiLegacyValue = 0;
  if (bGalResourceFormat)
  {
    WEnum<WGALResourceFormat> galFormat;
    WReflectionUtils::StringToEnumeration<WGALResourceFormat>(sFormat.GetData(), galFormat);
    uiLegacyValue = galFormat.GetValue();
  }
  else
  {
    uiLegacyValue = GetLegacySourceFormatValue(sFormat);
  }

  WEnum<WRequiredTextureType> type;
  WEnum<WRequiredTexturePrecision> precision;
  WEnum<WRequiredTextureChannels> channels;
  WGetLegacyTextureFormatRequirements(uiLegacyValue, bGalResourceFormat, type, precision, channels);

  WStringBuilder sValue;
  WReflectionUtils::EnumerationToString(type, sValue);
  pNode->AddProperty("Type", sValue.GetView());
  WReflectionUtils::EnumerationToString(precision, sValue);
  pNode->AddProperty("Precision", sValue.GetView());
  WReflectionUtils::EnumerationToString(channels, sValue);
  pNode->AddProperty("Channels", sValue.GetView());
  pNode->RemoveProperty("Format");
}

class WSourcePassPatch_1_2 : public WGraphPatch
{
public:
  WSourcePassPatch_1_2()
    : WGraphPatch("WSourcePass", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

WSourcePassPatch_1_2 g_WSourcePassPatch_1_2;

class WSourcePassPatch_2_3 : public WGraphPatch
{
public:
  WSourcePassPatch_2_3()
    : WGraphPatch("WSourcePass", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // will actually patch from version 2 to 4 schema, the call in WSourcePassPatch_3_4 will be a no-op.
    WPatchLegacyTextureFormatProperty(pNode, true);
  }
};

WSourcePassPatch_2_3 g_WSourcePassPatch_2_3;

class WSourcePassPatch_3_4 : public WGraphPatch
{
public:
  WSourcePassPatch_3_4()
    : WGraphPatch("WSourcePass", 4)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    WPatchLegacyTextureFormatProperty(pNode, false);
  }
};

WSourcePassPatch_3_4 g_WSourcePassPatch_3_4;

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);
