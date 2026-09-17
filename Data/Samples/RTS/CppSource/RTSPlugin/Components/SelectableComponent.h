#pragma once

#include <RTSPlugin/RTSPluginDLL.h>

#include <Core/World/ComponentManager.h>

using RtsSelectableComponentManager = WComponentManager<class RtsSelectableComponent, WBlockStorageType::Compact>;

struct WMsgUpdateLocalBounds;

class W_RTSPLUGIN_DLL RtsSelectableComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(RtsSelectableComponent, WComponent, RtsSelectableComponentManager);

public:
  RtsSelectableComponent();
  ~RtsSelectableComponent();

  //////////////////////////////////////////////////////////////////////////
  // WComponent interface

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fSelectionRadius = 1.0f;

  static WSpatialData::Category s_SelectableCategory;

protected:
};
