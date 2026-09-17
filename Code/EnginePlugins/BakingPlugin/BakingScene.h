#pragma once

#include <BakingPlugin/Declarations.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class WWorld;
class WProgress;
class WTracerInterface;

class W_BAKINGPLUGIN_DLL WBakingScene
{
public:
  WResult Extract();

  WResult Bake(const WStringView& sOutputPath, WProgress& progress);

  WResult RenderDebugView(const WMat4& InverseViewProjection, WUInt32 uiWidth, WUInt32 uiHeight, WDynamicArray<WColorGammaUB>& out_Pixels,
    WProgress& progress) const;

public:
  const WWorldGeoExtractionUtil::MeshObjectList& GetMeshObjects() const { return m_MeshObjects; }
  const WBoundingBox& GetBoundingBox() const { return m_BoundingBox; }

  bool IsBaked() const { return m_bIsBaked; }

private:
  friend class WBaking;
  friend class WMemoryUtils;

  WBakingScene();
  ~WBakingScene();

  WBakingSettings m_Settings;
  WDynamicArray<WBakingInternal::Volume, WAlignedAllocatorWrapper> m_Volumes;
  WWorldGeoExtractionUtil::MeshObjectList m_MeshObjects;
  WBoundingBox m_BoundingBox;

  WUInt32 m_uiWorldIndex = WInvalidIndex;
  WUniquePtr<WTracerInterface> m_pTracer;

  bool m_bIsBaked = false;
};

class W_BAKINGPLUGIN_DLL WBaking : public WBakingInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WBaking, WBakingInterface);

public:
  WBaking();

  void Startup();
  void Shutdown();

  WBakingScene* GetOrCreateScene(const WWorld& world);
  WBakingScene* GetScene(const WWorld& world);
  const WBakingScene* GetScene(const WWorld& world) const;

  // WBakingInterface
  virtual WResult RenderDebugView(const WWorld& world, const WMat4& InverseViewProjection, WUInt32 uiWidth, WUInt32 uiHeight, WDynamicArray<WColorGammaUB>& out_Pixels, WProgress& progress) const override;
};
