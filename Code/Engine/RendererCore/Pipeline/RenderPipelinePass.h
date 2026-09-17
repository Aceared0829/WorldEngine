#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraph.h>

struct WGALTextureCreationDescription;
class WStreamWriter;
struct WViewData;
class WCamera;

/// Passed into `WRenderPipelinePass::AddRenderPasses`. Defines the type and handle of a pin connection.
struct W_RENDERERCORE_DLL WRenderPipelinePinConnection
{
  enum class Connectivity : WUInt8
  {
    None,
    Texture,
    Buffer,
  };

  W_ALWAYS_INLINE WRenderPipelinePinConnection(Connectivity connectivity = Connectivity::None);
  W_ALWAYS_INLINE WRenderPipelinePinConnection(Connectivity connectivity, WRenderGraphTextureHandle hTextureHandle);
  W_ALWAYS_INLINE WRenderPipelinePinConnection(Connectivity connectivity, WRenderGraphBufferHandle hBufferHandle);
  W_ALWAYS_INLINE WRenderPipelinePinConnection(const WRenderPipelinePinConnection& other);

  W_ALWAYS_INLINE WRenderPipelinePinConnection& operator=(const WRenderPipelinePinConnection& other);

  const Connectivity m_Connectivity = Connectivity::None;
  union
  {
    WRenderGraphTextureHandle m_TextureHandle;
    WRenderGraphBufferHandle m_BufferHandle;
  };
};

/// Shading quality settings for forward rendering.
struct WForwardRenderShadingQuality
{
  using StorageType = WInt8;

  enum Enum
  {
    Normal,     ///< Full lighting and shading calculations.
    Simplified, ///< Reduced quality for performance.

    Default = Normal,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WForwardRenderShadingQuality);

class W_RENDERERCORE_DLL WRenderPipelinePass : public WRenderPipelineNode
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelinePass, WRenderPipelineNode);
  W_DISALLOW_COPY_AND_ASSIGN(WRenderPipelinePass);

public:
  WRenderPipelinePass(const char* szName, bool bIsStereoAware = false);
  ~WRenderPipelinePass();

  /// Sets the name of the pass.
  void SetName(const char* szName);

  /// returns the name of the pass.
  const char* GetName() const;

  /// True if the render pipeline pass can handle stereo cameras correctly.
  W_ALWAYS_INLINE bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \name New Render Graph Interface
  ///@{

  /// Called by the render pipeline when this pass is active. The pass declares its render-graph passes and writes the resulting transient handles into the `outputs` array.
  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) { return W_SUCCESS; }

  /// Called by the render pipeline when this pass is inactive instead of AddRenderPasses. The default implementation does nothing; passes that produce outputs must override this and at minimum write the same output handles they would in AddRenderPasses (typically by adding a clear pass) so downstream passes still see a valid resource.
  virtual WStatus AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) { return W_SUCCESS; }

  /// Returns the current texture this node provides at the given *ProviderPin.
  /// This function is called every frame if this node holds a WRenderPipelineNodeInputProviderPin or WRenderPipelineNodeOutputProviderPin pin. The node can return a valid texture handle, or an invalid handle, in which case the missing texture will be created from the texture pool.
  /// \param pPin The member pin for which the texture is requested.
  /// \param desc The format of the texture that should be provided.
  /// \return The texture to use for this pin's connections. Or invalid, in which case it reverts to a regular input / output pin.
  virtual WGALTextureHandle QueryTextureProvider(const WRenderPipelineNodePin* pPin, const WGALTextureCreationDescription& desc) { return {}; }

  ///@}


  /// Allows for the pass to write data back using WView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(WView* pView);

  virtual WResult Serialize(WStreamWriter& inout_stream) const;
  virtual WResult Deserialize(WStreamReader& inout_stream);

  /// Imports and declares the render-graph barrier dependencies that were recorded during extraction for `category` (via WMsgExtractRenderData::AddDependency).
  /// Call this in AddRenderPasses for every category you want to call `RenderDataWithCategory` on in the execution callback.
  void DeclareRendererDependenciesForCategory(WRenderData::Category category, WRenderGraph& ref_graph, WRenderGraphPassBuilder& ref_passBuilder);

  void RenderDataWithCategory(const WRenderViewContext& renderViewContext, WRenderData::Category category);

  W_ALWAYS_INLINE WRenderPipeline* GetPipeline() { return m_pPipeline; }
  W_ALWAYS_INLINE const WRenderPipeline* GetPipeline() const { return m_pPipeline; }

protected:
  void SetReadBackProperty(WView* pView, WStringView sPropertyName, const WVariant& value);

private:
  friend class WRenderPipeline;
  friend class WRenderPipelinePassGraph;

  bool m_bActive = true;

  const bool m_bIsStereoAware;
  WHashedString m_sName;

  WRenderPipeline* m_pPipeline = nullptr;
};

#include <RendererCore/Pipeline/Implementation/RenderPipelinePass_inl.h>
