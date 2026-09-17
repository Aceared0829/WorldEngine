#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

/// Orientation modes for quad particles.
struct W_PARTICLEPLUGIN_DLL WQuadParticleOrientation
{
  using StorageType = WUInt8;

  enum Enum
  {
    Billboard,                ///< Always faces the camera

    Rotating_OrthoEmitterDir, ///< Rotates around axis orthogonal to emitter direction
    Rotating_EmitterDir,      ///< Rotates around emitter direction

    Fixed_EmitterDir,         ///< Fixed orientation based on emitter direction
    Fixed_WorldUp,            ///< Fixed orientation aligned with world up
    Fixed_RandomDir,          ///< Fixed random orientation per particle

    FixedAxis_EmitterDir,     ///< Fixed axis aligned with emitter direction
    FixedAxis_ParticleDir,    ///< Fixed axis aligned with particle direction

    Default = Billboard
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WQuadParticleOrientation);

/// Factory for creating quad particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypeQuadFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeQuadFactory, WParticleTypeFactory);

public:
  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  WHashedString m_sTexture;
  WHashedString m_sCustomMaterial;
  WHashedString m_sTintColorParameter;

  WTexture2DResourceHandle m_hTexture;
  WMaterialResourceHandle m_hCustomMaterial;

  WEnum<WQuadParticleOrientation> m_Orientation;
  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WEnum<WParticleLightingMode> m_LightingMode;
  WEnum<WParticleTextureAtlasType> m_TextureAtlasType;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  WUInt8 m_uiNumSpritesX = 1;
  WUInt8 m_uiNumSpritesY = 1;
  bool m_bUseCustomMaterial = false;

  WAngle m_MaxDeviation;
  float m_fStretch = 1;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;
};

/// Renders particles as camera-facing or oriented textured quads.
///
/// This is the most common particle type, rendering each particle as a textured
/// quad. Quads can be billboarded to face the camera or oriented in various ways.
/// Supports texture atlases for sprite animations and variations. Can be lit or
/// fullbright. The stretch parameter allows elongating particles along their
/// velocity direction for motion blur effects.
class W_PARTICLEPLUGIN_DLL WParticleTypeQuad final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeQuad, WParticleType);

public:
  WParticleTypeQuad();
  ~WParticleTypeQuad();

  virtual void CreateRequiredStreams() override;

  WTexture2DResourceHandle m_hTexture;
  WMaterialResourceHandle m_hCustomMaterial;
  WTempHashedString m_sTintColorParameter;

  WEnum<WQuadParticleOrientation> m_Orientation;
  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WEnum<WParticleLightingMode> m_LightingMode;
  WEnum<WParticleTextureAtlasType> m_TextureAtlasType;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  WUInt8 m_uiNumSpritesX = 1;
  WUInt8 m_uiNumSpritesY = 1;

  WAngle m_MaxDeviation;
  float m_fStretch = 1;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;

  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

  /// Helper struct for depth sorting particles.
  struct sod
  {
    W_DECLARE_POD_TYPE();

    float dist;     ///< Distance from camera
    WUInt32 index; ///< Particle index
  };


protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override {}
  void AllocateParticleData(const WUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const;
  void AddParticleRenderData(WMsgExtractRenderData& msg, const WTransform& instanceTransform) const;
  void CreateExtractedData(const WHybridArray<sod, 64>* pSorted) const;

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamSize = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WProcessingStream* m_pStreamRotationSpeed = nullptr;
  WProcessingStream* m_pStreamRotationOffset = nullptr;
  WProcessingStream* m_pStreamAxis = nullptr;
  WProcessingStream* m_pStreamVariation = nullptr;
  WProcessingStream* m_pStreamLastPosition = nullptr;

  mutable WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;
  mutable WArrayPtr<WBillboardQuadParticleShaderData> m_BillboardParticleData;
  mutable WArrayPtr<WTangentQuadParticleShaderData> m_TangentParticleData;
};
