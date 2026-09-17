#pragma once

#include <BakingPlugin/BakingPluginDLL.h>

class WBakingScene;

class W_BAKINGPLUGIN_DLL WTracerInterface
{
public:
  virtual WResult BuildScene(const WBakingScene& scene) = 0;

  struct Ray
  {
    W_DECLARE_POD_TYPE();

    WVec3 m_vStartPos;
    WVec3 m_vDir;
    float m_fDistance;
  };

  struct Hit
  {
    W_DECLARE_POD_TYPE();

    WVec3 m_vPosition;
    WVec3 m_vNormal;
    float m_fDistance;
  };

  virtual void TraceRays(WArrayPtr<const Ray> rays, WArrayPtr<Hit> hits) = 0;
};
