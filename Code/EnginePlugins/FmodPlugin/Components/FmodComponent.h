#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <FmodPlugin/FmodPluginDLL.h>

/// Base class for all FMOD components, such that they all have a common ancestor
class W_FMODPLUGIN_DLL WFmodComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WFmodComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(WWorldReader& inout_stream) override {}

  //////////////////////////////////////////////////////////////////////////
  // WFmodComponent

public:
  WFmodComponent();
  ~WFmodComponent();

private:
  virtual void WFmodComponentIsAbstract() = 0; // abstract classes are not shown in the UI, since this class has no other abstract functions so far, this is a dummy
};
