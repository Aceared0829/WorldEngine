#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;
struct WTrailParticleData;

/// Factory for creating trail particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypeTrailFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeTrailFactory, WParticleTypeFactory);

public:
  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WUInt16 m_uiMaxPoints;
  WTime m_UpdateDiff;
  WHashedString m_sTexture;
  WEnum<WParticleTextureAtlasType> m_TextureAtlasType;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  WUInt8 m_uiNumSpritesX = 1;
  WUInt8 m_uiNumSpritesY = 1;
  WHashedString m_sTintColorParameter;
  WEnum<WParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  bool m_bUseCustomMaterial = false;
  WHashedString m_sCustomMaterial;

  WTexture2DResourceHandle m_hTexture;
  WMaterialResourceHandle m_hCustomMaterial;
};

/// Renders particles as textured ribbons following their movement path.
///
/// Trails record particle positions over time and render a ribbon connecting
/// the trail points. The trail length is determined by the maximum number of
/// points and the update interval. Older trail points fade out automatically.
/// Trails can be lit or fullbright and support texture atlases.
class W_PARTICLEPLUGIN_DLL WParticleTypeTrail final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeTrail, WParticleType);

public:
  WParticleTypeTrail();
  ~WParticleTypeTrail();

  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WUInt16 m_uiMaxPoints;
  WTime m_UpdateDiff;
  WTexture2DResourceHandle m_hTexture;
  WEnum<WParticleTextureAtlasType> m_TextureAtlasType;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  WUInt8 m_uiNumSpritesX = 1;
  WUInt8 m_uiNumSpritesY = 1;
  WTempHashedString m_sTintColorParameter;
  WEnum<WParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  WMaterialResourceHandle m_hCustomMaterial;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

  /// Returns an approximation of the maximum trail radius.
  ///
  /// This is an estimate based on particle size and maximum trail length.
  /// Inspecting actual trail positions would be more accurate but costly.
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return fParticleSize + m_uiMaxPoints * 0.05f; }

  /// Computes the memory bucket size needed for storing trail points.
  static WUInt16 ComputeTrailPointBucketSize(WUInt16 uiMaxTrailPoints);

protected:
  friend class WParticleTypeTrailFactory;

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override;
  void OnParticleDeath(const WStreamGroupElementRemovedEvent& e);

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamSize = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WProcessingStream* m_pStreamTrailData = nullptr;
  WProcessingStream* m_pStreamVariation = nullptr;
  WTime m_LastSnapshot;
  WUInt8 m_uiCurFirstIndex = 0;
  float m_fSnapshotFraction;

  mutable WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;
  mutable WArrayPtr<WTrailParticleShaderData> m_TrailParticleData;
  mutable WArrayPtr<WVec4> m_TrailPointsShared;

  struct TrailData
  {
    WUInt16 m_uiNumPoints;
    WUInt16 m_uiIndexForTrailPoints;
  };

  WUInt16 GetIndexForTrailPoints();
  const WVec4* GetTrailPointsPositions(WUInt32 index) const;
  WVec4* GetTrailPointsPositions(WUInt32 index);

  // Currently only 64-point trails are used. Smaller bucket sizes are reserved for future use.
  WDynamicArray<WTrailParticlePointsData64, WAlignedAllocatorWrapper> m_TrailPoints64; ///< Storage for trail points
  WDynamicArray<WUInt16> m_FreeTrailData;                                               ///< Freelist for trail data allocation
};
