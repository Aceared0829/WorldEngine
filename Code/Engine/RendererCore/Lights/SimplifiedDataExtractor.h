#pragma once

#include <RendererCore/Pipeline/Extractor.h>

/// Minimal CPU-side lighting data for simplified rendering.
///
/// Used when clustered rendering is not needed or available. Contains only basic
/// lighting information like sky irradiance.
class WSimplifiedDataCPU : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WSimplifiedDataCPU, WRenderData);

public:
  WSimplifiedDataCPU();
  ~WSimplifiedDataCPU();

  WUInt32 m_uiSkyIrradianceIndex = 0;
  WEnum<WCameraUsageHint> m_cameraUsageHint = WCameraUsageHint::Default;
};

/// Minimal GPU-side lighting data for simplified rendering.
///
/// Contains only essential lighting data uploaded to the GPU. Used with WSimplifiedDataExtractor
/// for rendering paths that don't require full clustered lighting.
struct W_RENDERERCORE_DLL WSimplifiedDataGPU
{
  W_DISALLOW_COPY_AND_ASSIGN(WSimplifiedDataGPU);

public:
  WSimplifiedDataGPU();
  ~WSimplifiedDataGPU();

  WUInt32 m_uiSkyIrradianceIndex = 0;
  WEnum<WCameraUsageHint> m_cameraUsageHint = WCameraUsageHint::Default;
  WGALBufferHandle m_hConstantBuffer;
};

/// Extracts minimal lighting data for simplified rendering.
///
/// Alternative to WClusteredDataExtractor for cases where full clustered rendering
/// is not required. Provides basic lighting information without the overhead of
/// spatial clustering. Used for lower-end rendering paths or specific view types.
class W_RENDERERCORE_DLL WSimplifiedDataExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WSimplifiedDataExtractor, WExtractor);

public:
  WSimplifiedDataExtractor(const char* szName = "SimplifiedDataExtractor");
  ~WSimplifiedDataExtractor();

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

private:
  void UpdateGpuData(const WView& view, const WSimplifiedDataCPU* pData);
  void AddGpuData(const WView& view, WExtractedRenderData& ref_extractedRenderData);

private:
  WSimplifiedDataGPU m_DataGPU;
};
