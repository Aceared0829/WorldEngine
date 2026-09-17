#pragma once

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>

/// Allows to access data from a previous frame. Always comes in a pair with a WHistoryTargetPass.
/// To preserve textures across the next frame you need to create this node to define the type of texture and initial state. This node's output pin will give access to the previous frame's content.
/// Next, create an WHistoryTargetPass. It's input pin exposes the same texture as provided by the source node but allows you to write to by connecting the input pin to another pass that produces the image that you want to carry to the next frame. To connect an WHistoryTargetPass to its counterpart you need to set it's "SourcePassName" property to the name of the WHistorySourcePass you want to match.
/// As both nodes expose the same texture, special care has to be taken that it's not used as input and output of another pass at the same time. In those cases, add a WCopyTexturePass to break up invalid state.
class W_RENDERERCORE_DLL WHistorySourcePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WHistorySourcePass, WRenderPipelinePass);

public:
  WHistorySourcePass(const char* szName = "HistorySourcePass");
  ~WHistorySourcePass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  /// Provides the history texture handle for the output pin.
  virtual WGALTextureHandle QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeOutputProviderPin m_PinOutput;                                       ///< Provides previous frame's texture.

  WEnum<WRequiredTextureType> m_Type = WRequiredTextureType::SRGB;                      ///< How the texture contents are interpreted.
  WEnum<WRequiredTexturePrecision> m_MinPrecision = WRequiredTexturePrecision::Bits_8;  ///< Minimum bits per channel.
  WEnum<WRequiredTextureChannels> m_MinChannels = WRequiredTextureChannels::Channels_4; ///< Minimum channel count.
  WEnum<WGALMSAASampleCount> m_MsaaMode = WGALMSAASampleCount::None;                    ///< MSAA sample count.
  WColor m_ClearColor = WColor::Black;                                                   ///< Initial clear color for first frame.
  float m_fClearDepth = 1.0f;                                                              ///< Initial clear depth for first frame.
  bool m_bUAV = false;
  WGALTextureHandle m_hTextureCleared = {};
};

/// Frame data provider that manages history texture storage across frames.
class W_RENDERERCORE_DLL WHistorySourcePassTextureDataProvider : public WFrameDataProviderBase
{
  W_ADD_DYNAMIC_REFLECTION(WHistorySourcePassTextureDataProvider, WFrameDataProviderBase);

public:
  WHistorySourcePassTextureDataProvider();
  ~WHistorySourcePassTextureDataProvider();

  /// Clears the history texture for a given source pass.
  void ResetTexture(WStringView sSourcePassName);

  /// Retrieves or creates a history texture for a given source pass.
  WGALTextureHandle GetOrCreateTexture(WStringView sSourcePassName, const WGALTextureCreationDescription& desc);

public:
  WHashTable<WString, WGALTextureHandle> m_Data; ///< Maps source pass names to history textures.

private:
  // We ignore the frame-based logic for this data provider as we only want to store cross frame data.
  virtual void* UpdateData(const WRenderViewContext& renderViewContext, const WExtractedRenderData& extractedData) override { return nullptr; }
};
