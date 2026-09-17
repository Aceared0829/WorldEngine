#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>

struct WMsgSetColor;

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

// Bitmask to allow the user to select what debug rendering the component should do
struct SampleRenderComponentMask
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

W_DECLARE_REFLECTABLE_TYPE(W_CPPPROJECTPLUGIN_DLL, SampleRenderComponentMask);

// use WComponentUpdateType::Always for this component to have 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
using SampleRenderComponentManager = WComponentManagerSimple<class SampleRenderComponent, WComponentUpdateType::Always>;

class SampleRenderComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(SampleRenderComponent, WComponent, SampleRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // SampleRenderComponent

public:
  SampleRenderComponent();
  ~SampleRenderComponent();

  float m_fSize = 1.0f;                                // [ property ]
  WColor m_Color = WColor::White;                    // [ property ]
  WTexture2DResourceHandle m_hTexture;                // [ property ]
  WBitflags<SampleRenderComponentMask> m_RenderTypes; // [ property ]

  void OnSetColor(WMsgSetColor& msg); // [ msg handler ]

  void SetRandomColor(); // [ scriptable ]

private:
  void Update();
};
