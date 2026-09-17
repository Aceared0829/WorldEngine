#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct WMsgComponentInternalTrigger;

// This component manager does literally nothing, meaning the managed components do not need to be update, at all
// BEGIN-DOCS-CODE-SNIPPET: component-manager-trivial
using SendMsgComponentManager = WComponentManager<class SendMsgComponent, WBlockStorageType::Compact>;
// END-DOCS-CODE-SNIPPET

class W_SAMPLEGAMEPLUGIN_DLL SendMsgComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(SendMsgComponent, WComponent, SendMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SendMsgComponent

public:
  SendMsgComponent();
  ~SendMsgComponent();

private:
  WDynamicArray<WString> m_TextArray;                // [ property ]

  void OnSendText(WMsgComponentInternalTrigger& msg); // [ msg handler ]

  WUInt32 m_uiNextString = 0;
};
