#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/RendererCoreDLL.h>

/// Platform profile configuration for core rendering settings.
///
/// Allows different rendering quality settings per platform (PC, console, mobile, etc.).
/// Settings are stored in platform profile files and can be overridden at runtime.
class W_RENDERERCORE_DLL WCoreRenderProfileConfig : public WProfileConfigData
{
  W_ADD_DYNAMIC_REFLECTION(WCoreRenderProfileConfig, WProfileConfigData);

public:
  virtual void SaveRuntimeData(WChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(WChunkStreamReader& inout_stream) override;

  WUInt32 m_uiShadowAtlasTextureSize = 4096;       ///< Size of the texture atlas used for shadow maps.
  WUInt32 m_uiMaxShadowMapSize = 1024;             ///< Maximum size for individual shadow maps.
  WUInt32 m_uiMinShadowMapSize = 64;               ///< Minimum size for individual shadow maps.

  WUInt32 m_uiRuntimeDecalAtlasTextureSize = 3072; ///< Size of the texture atlas for runtime decals.
};
