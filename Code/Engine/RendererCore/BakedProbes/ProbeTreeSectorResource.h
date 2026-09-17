#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

using WProbeTreeSectorResourceHandle = WTypedResourceHandle<class WProbeTreeSectorResource>;

struct W_RENDERERCORE_DLL WProbeTreeSectorResourceDescriptor
{
  W_DISALLOW_COPY_AND_ASSIGN(WProbeTreeSectorResourceDescriptor);

  WProbeTreeSectorResourceDescriptor();
  ~WProbeTreeSectorResourceDescriptor();
  WProbeTreeSectorResourceDescriptor& operator=(WProbeTreeSectorResourceDescriptor&& other);

  WVec3 m_vGridOrigin;
  WVec3 m_vProbeSpacing;
  WVec3U32 m_vProbeCount;

  WDynamicArray<WVec3> m_ProbePositions;
  WDynamicArray<WCompressedSkyVisibility> m_SkyVisibility;

  void Clear();
  WUInt64 GetHeapMemoryUsage() const;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

class W_RENDERERCORE_DLL WProbeTreeSectorResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WProbeTreeSectorResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WProbeTreeSectorResource);
  W_RESOURCE_DECLARE_CREATEABLE(WProbeTreeSectorResource, WProbeTreeSectorResourceDescriptor);

public:
  WProbeTreeSectorResource();
  ~WProbeTreeSectorResource();

  const WVec3& GetGridOrigin() const { return m_Desc.m_vGridOrigin; }
  const WVec3& GetProbeSpacing() const { return m_Desc.m_vProbeSpacing; }
  const WVec3U32& GetProbeCount() const { return m_Desc.m_vProbeCount; }

  WArrayPtr<const WVec3> GetProbePositions() const { return m_Desc.m_ProbePositions; }
  WArrayPtr<const WCompressedSkyVisibility> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  WProbeTreeSectorResourceDescriptor m_Desc;
};
