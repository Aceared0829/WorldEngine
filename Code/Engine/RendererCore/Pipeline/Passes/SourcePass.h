#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>

/// Minimum number of bits per channel that a texture format has to provide.
///
/// The values are the bit counts themselves, so a larger value always means a higher precision. Format selection picks the cheapest format that provides at least this precision, so a value that no format satisfies results in an error rather than a silent downgrade.
struct WRequiredTexturePrecision
{
  using StorageType = WUInt8;

  enum Enum
  {
    Bits_5 = 5,
    Bits_8 = 8,
    Bits_10 = 10,
    Bits_16 = 16,
    Bits_24 = 24,
    Bits_32 = 32,
    Default = Bits_8
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRequiredTexturePrecision);

/// How the contents of a texture are interpreted by the GPU.
///
/// Together with WRequiredTexturePrecision and WRequiredTextureChannels this describes what a pass needs from a texture, instead of naming a concrete WGALResourceFormat that may not exist on every device.
struct WRequiredTextureType
{
  using StorageType = WUInt8;

  enum Enum
  {
    UNorm = 1,
    SNorm = 2,
    SRGB = 3,
    UInt = 4,
    SInt = 5,
    Float = 6,
    Depth = 7, ///< Depth formats use the channel count to request a stencil part: Channels_1 is depth only, Channels_2 is depth and stencil.
    Default = SRGB
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRequiredTextureType);

/// Minimum number of channels that a texture format has to provide.
///
/// Format selection may pick a format with more channels if no exact match is supported by the device.
struct WRequiredTextureChannels
{
  using StorageType = WUInt8;

  enum Enum
  {
    Channels_1 = 1,
    Channels_2 = 2,
    Channels_3 = 3,
    Channels_4 = 4,
    Default = Channels_4
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRequiredTextureChannels);

class WAbstractObjectNode;

/// Maps a texture format that was serialized before the switch to WRequiredTexture* onto the new requirements.
///
/// \param uiLegacyValue The raw integer value that was stored in the old data.
/// \param bGalResourceFormat If true, the value is an WGALResourceFormat, otherwise it is the removed WSourceFormat enum.
W_RENDERERCORE_DLL void WGetLegacyTextureFormatRequirements(WUInt32 uiLegacyValue, bool bGalResourceFormat, WEnum<WRequiredTextureType>& out_type, WEnum<WRequiredTexturePrecision>& out_precision, WEnum<WRequiredTextureChannels>& out_channels);

/// Replaces the 'Format' property of an older graph node with the Type / Precision / Channels properties that describe the same texture.
/// \param bGalResourceFormat If true, the property holds an WGALResourceFormat name, otherwise a name of the removed WSourceFormat enum.
W_RENDERERCORE_DLL void WPatchLegacyTextureFormatProperty(WAbstractObjectNode* pNode, bool bGalResourceFormat);

/// Render pass that creates a render target for other passes to render into.
///
/// Entry point pass that allocates and optionally clears a render target with specified
/// format and MSAA settings. The output is then used by downstream passes for rendering.
class W_RENDERERCORE_DLL WSourcePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSourcePass, WRenderPipelinePass);

public:
  WSourcePass(const char* szName = "SourcePass");
  ~WSourcePass();

  /// Picks the cheapest WGALResourceFormat that satisfies all requirements and is supported by the current device.
  ///
  /// \return WGALResourceFormat::Invalid if no supported format matches.
  static WEnum<WGALResourceFormat> FindFormat(WEnum<WRequiredTextureType> type, WEnum<WRequiredTexturePrecision> minPrecision, WEnum<WRequiredTextureChannels> minChannels, WBitflags<WGALResourceFormatSupport> requiredSupport);

  /// Builds a render target description that matches the given requirements, the view's viewport size and the camera's stereo mode.
  ///
  /// Fails if the device supports no format for the requested combination.
  static WStatus GetOutputDescription(const WViewData& viewData, const WCamera& camera, WEnum<WRequiredTextureType> type, WEnum<WRequiredTexturePrecision> minPrecision, WEnum<WRequiredTextureChannels> minChannels, WEnum<WGALMSAASampleCount> msaaMode, bool bUAV, WGALTextureCreationDescription& out_desc);

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeOutputPin m_PinOutput;                                               ///< Output render target.

  WEnum<WRequiredTextureType> m_Type = WRequiredTextureType::SRGB;                      ///< How the texture contents are interpreted.
  WEnum<WRequiredTexturePrecision> m_MinPrecision = WRequiredTexturePrecision::Bits_8;  ///< Minimum bits per channel.
  WEnum<WRequiredTextureChannels> m_MinChannels = WRequiredTextureChannels::Channels_4; ///< Minimum channel count.
  WEnum<WGALMSAASampleCount> m_MsaaMode = WGALMSAASampleCount::None;                    ///< MSAA sample count.
  WColor m_ClearColor = WColor::Black;                                                   ///< Clear color if clearing is enabled and the format is not a depth format.
  float m_fClearDepth = 1.0f;                                                              ///< Clear depth if clearing is enabled and the format is a depth format.
  bool m_bClear = false;                                                                   ///< Whether to clear the render target on each execution.
  bool m_bUAV = false;                                                                     ///< Whether the texture also has to be writable from compute shaders.
};
