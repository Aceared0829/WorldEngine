#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Core/Input/InputDevice.h>

/// A synthetic input device that writes whatever an agent tells it to.
///
/// A real WInputDevice rather than WInputManager::InjectInputSlotValue(), for three reasons that all
/// fall out of how the manager works:
///
///  - GatherDeviceInputSlotValues() merges every device per slot with WMath::Max, so this coexists with
///    the actual keyboard and mouse instead of fighting them. A human at the machine keeps control.
///  - Key down/up transitions and dead zones are computed by the manager from the merged value, so
///    WKeyState::Pressed and Released come out correct without this class knowing anything about them.
///  - Holding a key for several frames is just leaving the value in m_InputSlotValues, and letting go is
///    erasing it. There is no per-slot bookkeeping to get wrong.
///
/// It registers no slots of its own. It writes the names that the real devices already registered -
/// 'keyboard_w', 'mouse_button_0' - which is what makes the Max merge meaningful in the first place.
/// A slot nobody registered can still be written, it just never reaches anything that reads input.
///
/// The header of WInputDevice warns that a device may need integration into window message handling.
/// That applies to hardware; there is no hardware here.
class WMcpInputDevice : public WInputDevice
{
  W_ADD_DYNAMIC_REFLECTION(WMcpInputDevice, WInputDevice);

public:
  WMcpInputDevice();
  ~WMcpInputDevice();

  /// The one instance, or nullptr while the plugin's input tool does not exist.
  static WMcpInputDevice* GetInstance() { return s_pInstance; }

  /// Sets a slot's value, held until it is changed again or cleared.
  ///
  /// \param uiFrames How many frames to hold it for, counted down once per input update. 0 means
  ///        indefinitely - which is what a caller who wants to press a key now and release it later
  ///        wants, and also the way to leave a movement axis held down.
  void SetSlotValue(WStringView sSlot, float fValue, WUInt32 uiFrames);

  /// Stops writing a slot entirely. Anything else writing it is then the only source again.
  void ClearSlot(WStringView sSlot);

  /// Stops writing every slot.
  void ClearAllSlots();

  /// Injects typed text in one call, as if the OS had delivered every character within the same frame.
  ///
  /// Unlike SetSlotValue(), this is not held: it is consumed the next time something reads
  /// WInputManager::RetrieveLastCharacters() - same as a real keystroke - and gone afterwards. Queuing
  /// again before that happens appends, it does not replace.
  void QueueText(WStringView sText);

  /// What this device is currently writing, and for how much longer.
  struct HeldSlot
  {
    WString m_sSlot;
    float m_fValue = 0.0f;

    /// 0 means 'until changed'.
    WUInt32 m_uiFramesRemaining = 0;
  };

  void GetHeldSlots(WDynamicArray<HeldSlot>& out_slots) const;

private:
  virtual void InitializeDevice() override {}
  virtual void RegisterInputSlots() override {}
  virtual void UpdateInputSlotValues() override;

  /// Slots the caller asked to be held for a limited number of frames, and the count that is left.
  /// Slots held indefinitely are not in here at all - they just sit in m_InputSlotValues.
  WMap<WString, WUInt32> m_RemainingFrames;

  static WMcpInputDevice* s_pInstance;
};
