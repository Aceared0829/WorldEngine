#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct WMsgUpdateLocalBounds;

using WBakedProbesVolumeComponentManager = WComponentManager<class WBakedProbesVolumeComponent, WBlockStorageType::Compact>;

class W_RENDERERCORE_DLL WBakedProbesVolumeComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WBakedProbesVolumeComponent, WComponent, WBakedProbesVolumeComponentManager);

public:
  WBakedProbesVolumeComponent();
  ~WBakedProbesVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const WVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const WVec3& vExtents);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg) const;

private:
  WVec3 m_vExtents = WVec3(10.0f);
};
