#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/CustomData/SampleCustomData.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct WMsgSetColor;

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

// Bitmask to allow the user to select what debug rendering the component should do
struct DebugRenderComponentMask
{
  using StorageType = WUInt8;

  // the enum names for the bits
  enum Enum
  {
    Box = W_BIT(0),
    Sphere = W_BIT(1),
    Cross = W_BIT(2),
    Quad = W_BIT(3),
    All = 0xFF,

    // required enum member; used by WBitflags for default initialization
    Default = All
  };

  // this allows the debugger to show us names for a bitmask
  // just try this out by looking at an WBitflags variable in a debugger
  struct Bits
  {
    WUInt8 Box : 1;
    WUInt8 Sphere : 1;
    WUInt8 Cross : 1;
    WUInt8 Quad : 1;
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_SAMPLEGAMEPLUGIN_DLL, DebugRenderComponentMask);

// use WComponentUpdateType::Always for this component to use 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
using DebugRenderComponentManager = WComponentManagerSimple<class DebugRenderComponent, WComponentUpdateType::Always>;

class W_SAMPLEGAMEPLUGIN_DLL DebugRenderComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(DebugRenderComponent, WComponent, DebugRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // DebugRenderComponent

public:
  DebugRenderComponent();
  ~DebugRenderComponent();

  float m_fSize = 1.0f;                               // [ property ]
  WColor m_Color = WColor::White;                   // [ property ]

  WTexture2DResourceHandle m_hTexture;               // [ property ]

  WBitflags<DebugRenderComponentMask> m_RenderTypes; // [ property ]

  void OnSetColor(WMsgSetColor& ref_msg);            // [ msg handler ]

  void SetRandomColor();                              // [ scriptable ]

  // BEGIN-DOCS-CODE-SNIPPET: customdata-interface
  SampleCustomDataResourceHandle m_hCustomData;
  // END-DOCS-CODE-SNIPPET


private:
  void Update();
};
