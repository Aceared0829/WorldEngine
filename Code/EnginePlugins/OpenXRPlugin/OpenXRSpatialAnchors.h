#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class WOpenXR;


class W_OPENXRPLUGIN_DLL WOpenXRSpatialAnchors : public WXRSpatialAnchorsInterface
{
  W_DECLARE_SINGLETON_OF_INTERFACE(WOpenXRSpatialAnchors, WXRSpatialAnchorsInterface);

public:
  WOpenXRSpatialAnchors(WOpenXR* pOpenXR);
  ~WOpenXRSpatialAnchors();

  WXRSpatialAnchorID CreateAnchor(const WTransform& globalTransform) override;
  WResult DestroyAnchor(WXRSpatialAnchorID id) override;
  WResult TryGetAnchorTransform(WXRSpatialAnchorID id, WTransform& out_globalTransform) override;

private:
  friend class WOpenXR;
  struct AnchorData
  {
    W_DECLARE_POD_TYPE();
    XrSpatialAnchorMSFT m_Anchor;
    XrSpace m_Space;
  };

  WOpenXR* m_pOpenXR = nullptr;

  WIdTable<WXRSpatialAnchorID, AnchorData> m_Anchors;
};
