#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Base class for frame data providers.
///
/// Frame data providers supply per-frame data to the rendering pipeline (e.g., clustered light data).
/// The data is computed once per frame and cached. Derived classes implement
/// UpdateData() to create or update the data. The pipeline calls GetData() to retrieve it.
class W_RENDERERCORE_DLL WFrameDataProviderBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WFrameDataProviderBase, WReflectedClass);

protected:
  WFrameDataProviderBase();

  /// Derived classes implement this to create or update frame data.
  ///
  /// Called once per frame when the data is first requested. Returns a pointer to the data.
  virtual void* UpdateData(const WRenderViewContext& renderViewContext, const WExtractedRenderData& extractedData) = 0;

  /// Returns the cached frame data, updating it if necessary.
  void* GetData(const WRenderViewContext& renderViewContext);

private:
  friend class WRenderPipeline;

  const WRenderPipeline* m_pOwnerPipeline = nullptr;
  void* m_pData = nullptr;
  WUInt64 m_uiLastUpdateFrame = 0;
};

/// Typed frame data provider template.
///
/// Simplifies creating frame data providers by providing type-safe access to the data.
template <typename T>
class WFrameDataProvider : public WFrameDataProviderBase
{
public:
  T* GetData(const WRenderViewContext& renderViewContext) { return static_cast<T*>(WFrameDataProviderBase::GetData(renderViewContext)); }
};
