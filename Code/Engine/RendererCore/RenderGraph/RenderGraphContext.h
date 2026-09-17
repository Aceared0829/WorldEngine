#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Delegate.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WGALCommandEncoder;
class WGALDevice;
class WRenderContext;

/// Provided to the pass execution callback. Gives access to the command encoder and allows resolving transient handles to real GPU resources.
///
/// For render passes, BeginRendering has already been called before the callback is invoked and EndRendering will be called after it returns.
class W_RENDERERCORE_DLL WRenderGraphContext
{
public:
  WRenderGraphContext() = default;
  WRenderGraphContext(WGALCommandEncoder* pCommandEncoder, WGALDevice* pDevice, WRenderContext* pRenderContext, const WReflectedClass* pUserData = nullptr)
    : m_pCommandEncoder(pCommandEncoder)
    , m_pDevice(pDevice)
    , m_pRenderContext(pRenderContext)
    , m_pUserData(pUserData)
  {
  }

  /// Resolve a transient texture handle to the real GPU texture.
  WGALTextureHandle ResolveTexture(WRenderGraphTextureHandle hTexture) const;

  /// Resolve a transient buffer handle to the real GPU buffer.
  WGALBufferHandle ResolveBuffer(WRenderGraphBufferHandle hBuffer) const;

  WGALCommandEncoder* GetCommandEncoder() const;
  WGALDevice* GetDevice() const;
  WRenderContext* GetRenderContext() const;

  /// Gives access to the graph's user data set via `WRenderGraph::SetUserData`.
  template <typename T>
  const T* GetUserData() const
  {
    return WDynamicCast<const T*>(m_pUserData);
  }

private:
  friend class WRenderGraph;

  WGALCommandEncoder* m_pCommandEncoder = nullptr;
  WGALDevice* m_pDevice = nullptr;
  WRenderContext* m_pRenderContext = nullptr;
  const WReflectedClass* m_pUserData = nullptr;
  const WDynamicArray<WUInt16>* m_pTextureToResolvedTexture = nullptr;
  const WDynamicArray<WUInt16>* m_pBufferToResolvedBuffer = nullptr;
  const WDynamicArray<WGALTextureHandle>* m_pResolvedTextures = nullptr;
  const WDynamicArray<WGALBufferHandle>* m_pResolvedBuffers = nullptr;
};

/// Execution callback type for render graph passes.
using WRenderGraphExecuteFunction = WDelegate<void(const WRenderGraphContext&), 16, WTempAllocatorWrapper>;
