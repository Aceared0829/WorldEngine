#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

using WParticleEffectResourceHandle = WTypedResourceHandle<class WParticleEffectResource>;

/// Descriptor for particle effect resources
///
/// Contains the particle effect configuration data.
struct W_PARTICLEPLUGIN_DLL WParticleEffectResourceDescriptor
{
  virtual void Save(WStreamWriter& inout_stream) const;
  virtual void Load(WStreamReader& inout_stream);

  WParticleEffectDescriptor m_Effect;
};

class W_PARTICLEPLUGIN_DLL WParticleEffectResource final : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEffectResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WParticleEffectResource);
  W_RESOURCE_DECLARE_CREATEABLE(WParticleEffectResource, WParticleEffectResourceDescriptor);

public:
  WParticleEffectResource();
  ~WParticleEffectResource();

  const WParticleEffectResourceDescriptor& GetDescriptor() { return m_Desc; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WParticleEffectResourceDescriptor m_Desc;
};
