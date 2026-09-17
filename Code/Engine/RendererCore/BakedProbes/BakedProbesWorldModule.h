#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <RendererCore/Declarations.h>

using WProbeTreeSectorResourceHandle = WTypedResourceHandle<class WProbeTreeSectorResource>;

class W_RENDERERCORE_DLL WBakedProbesWorldModule : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WBakedProbesWorldModule, WWorldModule);

public:
  WBakedProbesWorldModule(WWorld* pWorld);
  ~WBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool HasProbeData() const;

  struct ProbeIndexData
  {
    static constexpr WUInt32 NumProbes = 8;
    WUInt32 m_probeIndices[NumProbes];
    float m_probeWeights[NumProbes];
  };

  WResult GetProbeIndexData(const WVec3& vGlobalPosition, const WVec3& vNormal, ProbeIndexData& out_probeIndexData) const;

  WAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;

private:
  friend class WBakedProbesComponent;

  void SetProbeTreeResourcePrefix(const WHashedString& prefix);

  WProbeTreeSectorResourceHandle m_hProbeTree;
};
