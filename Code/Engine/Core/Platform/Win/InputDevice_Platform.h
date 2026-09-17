#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Platform/Win/Utils/MinWindows.h>

class W_CORE_DLL WInputDeviceMouseKeyboard_Win : public WInputDeviceMouseKeyboard
{
  W_ADD_DYNAMIC_REFLECTION(WInputDeviceMouseKeyboard_Win, WInputDeviceMouseKeyboard);

public:
  WInputDeviceMouseKeyboard_Win(WMinWindows::HWND hWnd);
  ~WInputDeviceMouseKeyboard_Win();

  /// This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(WMinWindows::UINT msg, WMinWindows::WPARAM wparam, WMinWindows::LPARAM lparam);

  /// Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  virtual WUInt32 GetHardwareCursorSize() const override;

  virtual void SetDisableOSHotkeys(bool bDisable) override;

protected:
  virtual void ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive) override;
  virtual void ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode) override;

  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void ApplyClipRect(WMouseCursorClipMode::Enum mode);
  void OnFocusLost();
  void RegisterRawInput();

  static WInputDeviceMouseKeyboard_Win* s_pGlobalInputHandler;

  WMinWindows::HWND m_hWnd;
  bool m_bApplyClipRect = false;
  // m_bFirstWndMsg and m_bFirstClick are used to fix issues Windows not giving focus to applications that have been launched
  // through a parent process
  bool m_bFirstWndMsg = true;
  bool m_bFirstClick = true;
  WUInt8 m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  WUInt8 m_uiMouseButtonReceivedUp[5] = {0, 0, 0, 0, 0};
};
