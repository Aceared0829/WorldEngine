#pragma once

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/EventMessageSender.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameEngineDLL.h>
struct WMsgSetPlaying;

using WPropertyAnimComponentManager = WComponentManagerSimple<class WPropertyAnimComponent, WComponentUpdateType::WhenSimulating>;

/// Animates properties on other objects and components according to the property animation resource
///
/// Notes:
///  - There is no messages to change speed, simply modify the speed property.
class W_GAMEENGINE_DLL WPropertyAnimComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WPropertyAnimComponent, WComponent, WPropertyAnimComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WPropertyAnimComponent

public:
  WPropertyAnimComponent();
  ~WPropertyAnimComponent();

  void SetPropertyAnim(const WPropertyAnimResourceHandle& hResource);                                     // [ property ]
  W_ALWAYS_INLINE const WPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; } // [ property ]

  /// Sets the animation playback range and resets the playing position to the range start position. Also activates the component if it isn't.
  void PlayAnimationRange(WTime rangeLow, WTime rangeHigh); // [ scriptable ]

  /// Pauses or resumes animation playback. Does not reset any state.
  void OnMsgSetPlaying(WMsgSetPlaying& ref_msg);                       // [ msg handler ]

  WEnum<WPropertyAnimMode> m_AnimationMode;                           // [ property ]
  WTime m_RandomOffset;                                                // [ property ]
  float m_fSpeed = 1.0f;                                                // [ property ]
  WTime m_AnimationRangeLow;                                           // [ property ]
  WTime m_AnimationRangeHigh;                                          // [ property ]
  bool m_bPlaying = true;                                               // [ property ]

protected:
  WEventMessageSender<WMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]
  WEventMessageSender<WMsgGenericEvent> m_EventTrackMsgSender;        // [ event ]

  struct Binding
  {
    const WAbstractMemberProperty* m_pMemberProperty = nullptr;
    mutable void* m_pObject = nullptr; // needs to be updated in case components / objects get relocated in memory
  };

  struct FloatBinding : public Binding
  {
    const WFloatPropertyAnimEntry* m_pAnimation[4] = {nullptr, nullptr, nullptr, nullptr};
  };

  struct ComponentFloatBinding : public FloatBinding
  {
    WComponentHandle m_hComponent;
  };

  struct GameObjectBinding : public FloatBinding
  {
    WGameObjectHandle m_hObject;
  };

  struct ColorBinding : public Binding
  {
    WComponentHandle m_hComponent;
    const WColorPropertyAnimEntry* m_pAnimation = nullptr;
  };

  void Update();
  void CreatePropertyBindings();
  void CreateGameObjectBinding(const WFloatPropertyAnimEntry* pAnim, const WRTTI* pRtti, void* pObject, const WGameObjectHandle& hGameObject);
  void CreateFloatPropertyBinding(const WFloatPropertyAnimEntry* pAnim, const WRTTI* pRtti, void* pObject, const WComponentHandle& hComponent);
  void CreateColorPropertyBinding(const WColorPropertyAnimEntry* pAnim, const WRTTI* pRtti, void* pObject, const WComponentHandle& hComponent);
  void ApplyAnimations(const WTime& tDiff);
  void ApplyFloatAnimation(const FloatBinding& binding, WTime lookupTime);
  void ApplySingleFloatAnimation(const FloatBinding& binding, WTime lookupTime);
  void ApplyColorAnimation(const ColorBinding& binding, WTime lookupTime);
  WTime ComputeAnimationLookup(WTime tDiff);
  void EvaluateEventTrack(WTime startTime, WTime endTime);
  void StartPlayback();

  bool m_bReverse = false;

  WTime m_AnimationTime;
  WHybridArray<GameObjectBinding, 4> m_GoFloatBindings;
  WHybridArray<ComponentFloatBinding, 4> m_ComponentFloatBindings;
  WHybridArray<ColorBinding, 4> m_ColorBindings;
  WPropertyAnimResourceHandle m_hPropertyAnim;

  // we do not want to recreate the binding when the resource changes at runtime
  // therefore we use a sharedptr to keep the data around as long as necessary
  // otherwise that would lead to weird state, because the animation would be interrupted at some point
  // and then the new animation would start from there
  // e.g. when the position is animated, objects could jump around the level
  // when the animation resource is reloaded
  // instead we go with one animation state until this component is reset entirely
  // that means you need to restart a level to see the updated animation
  WSharedPtr<WPropertyAnimResourceDescriptor> m_pAnimDesc;
};
