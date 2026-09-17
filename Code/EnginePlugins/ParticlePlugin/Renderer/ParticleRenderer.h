#pragma once

#include <Foundation/Math/Color16f.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/ParticleSystemConstants.h>

class WGALBufferPool;
class WRenderContext;

/// Implements rendering of particle systems
///
/// Base class for all particle renderers. Provides common functionality for uploading
/// particle data to the GPU and binding shaders with particle system constants.
class W_PARTICLEPLUGIN_DLL WParticleRenderer : public WRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleRenderer, WRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WParticleRenderer);

public:
  WParticleRenderer();
  ~WParticleRenderer();

protected:
  /// Helper for managing per-particle-system constant buffer data during rendering.
  ///
  /// Automatically binds the constant buffer on construction and unbinds on destruction.
  /// Provides methods to fill in system-specific rendering parameters.
  struct TempSystemCB
  {
    TempSystemCB(WRenderContext* pRenderContext);
    ~TempSystemCB();

    /// Sets general particle system rendering parameters including transform, texture variations, and lighting.
    void SetGenericData(const WTransform& objectTransform, WTime effectLifeTime, WUInt8 uiNumVariationsX, WUInt8 uiNumVariationsY, WUInt8 uiNumFlipbookAnimsX, WUInt8 uiNumFlipbookAnimsY, float fNormalCurvature = 0, float fLightDirectionality = 0, float fGeometryProximityFadeOut = 0.1f, float fCameraProximityFadeOut = 0.5f, WUInt8 uiTextureAtlasOrientation = 0);

    /// Sets trail-specific rendering parameters.
    void SetTrailData(float fSnapshotFraction, WInt32 iNumUsedTrailPoints);

    WConstantBufferStorage<WParticleSystemConstants>* m_pConstants;
    WConstantBufferStorageHandle m_hConstantBuffer;
  };

  /// Allocates a GPU buffer from the pool for uploading particle data.
  void CreateParticleDataBuffer(WGALBufferPool& inout_Buffer, WUInt32 uiDataTypeSize, WUInt32 uiNumParticlesPerBatch);

  /// Returns a particle data buffer to the pool.
  void DestroyParticleDataBuffer(WGALBufferPool& inout_Buffer);

  /// Loads and binds the specified particle shader for rendering.
  void BindParticleShader(WRenderContext* pRenderContext, const char* szShader) const;

protected:
  WShaderResourceHandle m_hShader;
};
