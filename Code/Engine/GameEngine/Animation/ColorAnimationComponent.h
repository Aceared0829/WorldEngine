#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

using WColorAnimationComponentManager = WComponentManagerSimple<class WColorAnimationComponent, WComponentUpdateType::WhenSimulating>;

/// Samples a color gradient and sends an WMsgSetColor to the object it is attached to
///
/// The color gradient is sampled linearly over time.
/// This can be used to animate the color of a light source or mesh.
class W_GAMEENGINE_DLL WColorAnimationComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WColorAnimationComponent, WComponent, WColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent
public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WColorAnimationComponent
public:
  WColorAnimationComponent();

  /// How long it takes to sample the entire color gradient.
  WTime m_Duration;                                                                                     // [ property ]

  void SetColorGradient(const WColorGradientResourceHandle& hResource);                                 // [ property ]
  W_ALWAYS_INLINE const WColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; } // [ property ]

  /// How the animation should be played and looped.
  WEnum<WPropertyAnimMode> m_AnimationMode; // [ property ]

  /// How the color should be applied to the target.
  WEnum<WSetColorMode> m_SetColorMode; // [ property ]

  bool GetApplyRecursive() const;        // [ property ]
  void SetApplyRecursive(bool value);    // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  WTime m_CurAnimTime;
  WColorGradientResourceHandle m_hGradient;
};
