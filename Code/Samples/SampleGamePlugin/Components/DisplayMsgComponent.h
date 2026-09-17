#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct WMsgSetText;
struct WMsgSetColor;

// BEGIN-DOCS-CODE-SNIPPET: component-manager-simple
using DisplayMsgComponentManager = WComponentManagerSimple<class DisplayMsgComponent, WComponentUpdateType::WhenSimulating, WBlockStorageType::FreeList>;
// END-DOCS-CODE-SNIPPET

class W_SAMPLEGAMEPLUGIN_DLL DisplayMsgComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(DisplayMsgComponent, WComponent, DisplayMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DisplayMsgComponent

public:
  DisplayMsgComponent();
  ~DisplayMsgComponent();

private:
  void Update();

  void OnSetText(WMsgSetText& msg);   // [ msg handler ]
  void OnSetColor(WMsgSetColor& msg); // [ msg handler ]

  WString m_sCurrentText;
  WColor m_TextColor = WColor::Yellow;
};
