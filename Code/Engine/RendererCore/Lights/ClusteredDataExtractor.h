#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Extractor.h>

struct WPerLightData;
struct WPerDecalData;
struct WPerReflectionProbeData;
struct WPerClusterData;

/// CPU-side data for clustered rendering containing lights, decals, and reflection probes.
///
/// Used by the clustered rendering system to organize lights, decals, and probes into spatial clusters
/// for efficient per-pixel lookup during shading. The clusters divide the view frustum into a 3D grid.
class WClusteredDataCPU : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WClusteredDataCPU, WRenderData);

public:
  WClusteredDataCPU();
  ~WClusteredDataCPU();

  enum
  {
    MAX_NUM_LIGHTS = W_BIT(10),
    MAX_NUM_DECALS = W_BIT(10),
    MAX_NUM_REFLECTION_PROBES = W_BIT(8),
  };

  WArrayPtr<WPerLightData> m_LightData;
  WArrayPtr<WPerDecalData> m_DecalData;
  WArrayPtr<WPerReflectionProbeData> m_ReflectionProbeData;
  WArrayPtr<WPerClusterData> m_ClusterData;
  WArrayPtr<WUInt32> m_ClusterItemList;

  WUInt32 m_uiBrightestDirectionalLightIndex = 0;
  WUInt32 m_uiSkyIrradianceIndex = 0;
  WEnum<WCameraUsageHint> m_cameraUsageHint = WCameraUsageHint::Default;

  float m_fFogHeight = 0.0f;
  float m_fFogHeightFalloff = 0.0f;
  float m_fFogDensityAtCameraPos = 0.0f;
  float m_fFogDensity = 0.0f;
  float m_fFogStartDistance = 0.0f;
  float m_fFogInvSkyDistance = 0.0f;
  WColor m_FogColor = WColor::Black;

  WVec3 m_vLightShaftsDirection = WVec3::MakeZero();
  float m_fLightShaftsIntensity = 0.0f;
  float m_fLightShaftsMaxBrightness = 0.0f;
  float m_fLightShaftsBrightnessThreshold = 0.0f;
  float m_fLightShaftsDiskMaskRadius = 0.0f;
  WColorGammaUB m_LightShaftsTintColor = WColor::White;
};

/// GPU-side data for clustered rendering.
///
/// Contains GPU buffers for lights, decals, probes, cluster assignments, and related resources.
/// Uploaded from WClusteredDataCPU by the data provider and bound to shaders for rendering.
struct W_RENDERERCORE_DLL WClusteredDataGPU
{
  W_DISALLOW_COPY_AND_ASSIGN(WClusteredDataGPU);

public:
  WClusteredDataGPU();
  ~WClusteredDataGPU();

  WUInt32 m_uiSkyIrradianceIndex = 0;
  WEnum<WCameraUsageHint> m_cameraUsageHint = WCameraUsageHint::Default;

  WGALBufferHandle m_hLightDataBuffer;
  WGALBufferHandle m_hDecalDataBuffer;
  WGALBufferHandle m_hReflectionProbeDataBuffer;
  WGALBufferHandle m_hClusterDataBuffer;
  WGALBufferHandle m_hClusterItemBuffer;

  WGALBufferHandle m_hConstantBuffer;

  WGALSamplerStateHandle m_hShadowSampler;

  WDecalAtlasResourceHandle m_hDecalAtlas;
  WGALSamplerStateHandle m_hDecalAtlasSampler;
};


/// Extracts lights, decals, and reflection probes into a clustered data structure.
///
/// Divides the view frustum into a 3D grid of clusters and assigns visible lights, decals,
/// and reflection probes to each cluster. This enables efficient per-pixel light lookup during
/// rendering. Runs after visibility determination in PostSortAndBatch().
class W_RENDERERCORE_DLL WClusteredDataExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WClusteredDataExtractor, WExtractor);

public:
  WClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~WClusteredDataExtractor();

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

private:
  void FillItemListAndClusterData(WClusteredDataCPU* pData);
  void UpdateGpuData(const WView& view, const WClusteredDataCPU* pData);
  void AddGpuData(const WView& view, WExtractedRenderData& ref_extractedRenderData);

  template <WUInt32 MaxData>
  struct TempCluster
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_BitMask[MaxData / 32];
  };

  WDynamicArray<WPerLightData, WAlignedAllocatorWrapper> m_TempLightData;
  WDynamicArray<WPerDecalData, WAlignedAllocatorWrapper> m_TempDecalData;
  WDynamicArray<WPerReflectionProbeData, WAlignedAllocatorWrapper> m_TempReflectionProbeData;
  WDynamicArray<TempCluster<WClusteredDataCPU::MAX_NUM_LIGHTS>> m_TempLightsClusters;
  WDynamicArray<TempCluster<WClusteredDataCPU::MAX_NUM_DECALS>> m_TempDecalsClusters;
  WDynamicArray<TempCluster<WClusteredDataCPU::MAX_NUM_REFLECTION_PROBES>> m_TempReflectionProbeClusters;
  WDynamicArray<WUInt32> m_TempClusterItemList;

  WDynamicArray<WSimdBSphere, WAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
  WDynamicArray<WSimdBSphere, WAlignedAllocatorWrapper> m_ClusterBoundingSpheresRightEye;
  WMat4 m_mProjection = WMat4::MakeZero();
  WMat4 m_mProjectionRightEye = WMat4::MakeZero();

  WClusteredDataGPU m_DataGPU;
};
