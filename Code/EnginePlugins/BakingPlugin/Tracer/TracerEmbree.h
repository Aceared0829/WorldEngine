#pragma once

#include <BakingPlugin/Tracer/TracerInterface.h>
#include <Foundation/Types/UniquePtr.h>

class W_BAKINGPLUGIN_DLL WTracerEmbree : public WTracerInterface
{
public:
  WTracerEmbree();
  ~WTracerEmbree();

  virtual WResult BuildScene(const WBakingScene& scene) override;

  virtual void TraceRays(WArrayPtr<const Ray> rays, WArrayPtr<Hit> hits) override;

private:
  struct Data;

  WUniquePtr<Data> m_pData;
};
