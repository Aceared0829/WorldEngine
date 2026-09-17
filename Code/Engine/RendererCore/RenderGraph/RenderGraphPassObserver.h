#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

class WGALDevice;
class WRenderGraph;
class WStreamReader;
class WStreamWriter;

/// Identifies a specific texture access within a render graph pass to observe.
/// Optionally also sets a target swap-chain to blit a preview image on.
struct WRenderGraphObserverRequest
{
  // Source texture
  WUInt64 m_uiRenderGraphId = 0;
  WString m_sPassName;
  WUInt16 m_uiAccessIndex = 0; ///< Nth texture access (reads then writes) within the pass.

  // Preview target
  WUInt32 m_uiSwapChainId = 0xFFFFFFFF; ///< If invalid, no preview will be rendered.

  // Sub-resource
  WUInt16 m_uiArraySlice = 0;
  WUInt8 m_uiMipLevel = 0;
  WUInt8 m_uiChannelMask = W_BIT(0) | W_BIT(1) | W_BIT(2) | W_BIT(3); ///< red, green, blue, alpha
  WInt8 m_iSampleIndex = -1;                                              ///< Negative = auto-resolve, 0+ = specific MSAA sample.

  // Value Clamp
  float m_fRangeMin = 0.0f;
  float m_fRangeMax = 1.0f;

  // Zoom
  float m_fZoom = 1.0f;
  WVec2 m_vPanCenter = WVec2(0.5f);

  // Pixel readback
  WVec2I32 m_vPixelPosition = WVec2I32(-1, -1); ///< Updated while pixel sampling is active. If negative, no pixel is selected.
  bool m_bHighlightPixel = false;                 ///< If true, preview rendering highlights the selected pixel row and column.
};

/// Contains the readback data produced for the active render graph observer request.
struct WRenderGraphObserverResponse
{
  float m_fImageMin = 0.0f;
  float m_fImageMax = 1.0f;
  WStaticArray<WUInt8, 1024> m_Histogram; ///< Four 256-bin RGBA histograms, relative to m_fRangeMin / m_fRangeMax and normalized to the highest non-flat channel value.
  bool m_bHistogramValid = false;
  WVariant m_PixelValue;
};

W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphObserverRequest& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphObserverRequest& ref_value);
W_RENDERERCORE_DLL void operator<<(WStreamWriter& inout_stream, const WRenderGraphObserverResponse& value);
W_RENDERERCORE_DLL void operator>>(WStreamReader& inout_stream, WRenderGraphObserverResponse& ref_value);

/// Observes a texture at a specific point in the render graph pipeline by copying it into a persistent texture after the target pass executes. Register instances with WRenderGraphManager. The render graph will compute barriers and execute the copy without modifying its own data structures.
class W_RENDERERCORE_DLL WRenderGraphPassObserver : public WRefCounted
{
public:
  ~WRenderGraphPassObserver();

  /// Thread-safe. Can be called from any thread. The request is applied
  /// on the render thread before barrier computation.
  void SetRequest(const WRenderGraphObserverRequest& request);

  /// Returns the active request. Only valid to call from the render thread.
  const WRenderGraphObserverRequest& GetRequest() const { return m_Request; }

  /// Returns the persistent copy of the observed texture as a resource handle.
  /// Can be used with material texture bindings. Invalid if no capture has occurred.
  const WTexture2DResourceHandle& GetCopyTextureResource() const { return m_hCopyTextureResource; }

  /// Returns the underlying GAL handle of the copy texture. Used internally for
  /// the GPU copy operation.
  WGALTextureHandle GetCopyTexture() const { return m_hCopyTexture; }

  /// Thread-safe. Returns the most recent inspection data that has finished readback.
  WRenderGraphObserverResponse GetResponse() const;

  /// \name Internal — called by WRenderGraph during ComputeBarriers / Execute
  ///@{

  /// Clears per-frame state. Called at the start of each frame's barrier computation.
  void Reset();

  /// Copies the pending request into the active request. Called on the
  /// render thread under the manager's mutex before barrier computation.
  void ApplyPendingRequest();

  /// (Re)creates the copy texture to match the source description if needed.
  void EnsureCopyTexture(const WGALTextureCreationDescription& srcDesc);

  void RecordPreview(WRenderGraph& ref_graph);

  ///@}

private:
  friend class WRenderGraph;
  friend class WRenderGraphManager;
  WRenderGraphPassObserver(WGALDevice* pDevice);

  void EnsureInspectionResources();
  void DestroyInspectionResources();
  void PollReadbacks();
  void ProcessPixelReadback(WArrayPtr<const WUInt8> memory);
  void ProcessHistogramReadback(WArrayPtr<const WUInt8> memory);
  void ProcessMinMaxReadback(WArrayPtr<const WUInt8> memory);

  mutable WMutex m_Mutex;
  WGALDevice* m_pDevice = nullptr;
  WRenderGraphObserverRequest m_Request;        ///< Active request, only accessed on render thread.
  WRenderGraphObserverRequest m_PendingRequest; ///< Written by any thread via SetRequest(), guarded by m_Mutex.
  WRenderGraphObserverResponse m_Response;      ///< Guarded by m_Mutex.
  bool m_bPendingRequestDirty = false;           ///< True when m_PendingRequest has been set but not yet applied.
  WRenderGraph* m_pGraph = nullptr;
  WGALSwapChainHandle m_hSwapChain;
  WGALTextureHandle m_hCopyTexture;
  WTexture2DResourceHandle m_hCopyTextureResource;

  WGALBufferHandle m_hPixelReadbackBuffer;
  WGALBufferHandle m_hHistogramBuffer;
  WGALBufferHandle m_hMinMaxBuffer;
  WGALReadbackBufferHelper m_PixelReadback;
  WGALReadbackBufferHelper m_HistogramReadback;
  WGALReadbackBufferHelper m_MinMaxReadback;
  WShaderResourceHandle m_hReadbackPixelShader;
  WShaderResourceHandle m_hClearHistogramShader;
  WShaderResourceHandle m_hBuildHistogramShader;
  WShaderResourceHandle m_hClearMinMaxShader;
  WShaderResourceHandle m_hBuildMinMaxShader;
  WShaderResourceHandle m_hPreviewShader;
  bool m_bPixelReadbackInFlight = false;
  bool m_bHistogramReadbackInFlight = false;
  bool m_bMinMaxReadbackInFlight = false;

  bool m_bValid = false;
  WUInt32 m_uiSortedPassIndex = 0xFFFFFFFF;
  WGALTextureHandle m_hResolvedSourceTexture;

  WHybridArray<WGALTextureBarrier, 4> m_PreCopyBarriers;
  WHybridArray<WGALTextureBarrier, 4> m_PostCopyBarriers;
};
