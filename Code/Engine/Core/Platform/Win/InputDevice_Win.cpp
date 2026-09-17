#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

#  include <Core/Input/InputManager.h>
#  include <Core/Platform/Win/InputDevice_Platform.h>
#  include <Foundation/Containers/HybridArray.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceMouseKeyboard_Win, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#  define WM_USER_UPDATE_CURSOR (WM_USER + 1)

WInputDeviceMouseKeyboard_Win* WInputDeviceMouseKeyboard_Win::s_pGlobalInputHandler = nullptr;

WInputDeviceMouseKeyboard_Win::WInputDeviceMouseKeyboard_Win(WMinWindows::HWND hWnd)
{
  m_hWnd = hWnd;

  if (s_pGlobalInputHandler == nullptr)
  {
    // the first window that gets created takes care of handling the global input
    s_pGlobalInputHandler = this;
  }

  m_DoubleClickTime = WTime::MakeFromMilliseconds(GetDoubleClickTime());
}

WInputDeviceMouseKeyboard_Win::~WInputDeviceMouseKeyboard_Win()
{
  if (!m_bShowMouseCursorEffective)
  {
    ShowCursor(true);
  }

  if (s_pGlobalInputHandler == this)
  {
    s_pGlobalInputHandler = nullptr;
    WLog::Dev("Global mouse/keyboard input handler destroyed.");
  }
}

void WInputDeviceMouseKeyboard_Win::RegisterRawInput()
{
  RAWINPUTDEVICE Rid[2];

  // keyboard
  Rid[0].usUsagePage = 0x01;
  Rid[0].usUsage = 0x06;
  Rid[0].dwFlags = m_bDisableOSHotkeys ? RIDEV_NOHOTKEYS : 0;
  Rid[0].hwndTarget = nullptr;

  // mouse
  Rid[1].usUsagePage = 0x01;
  Rid[1].usUsage = 0x02;
  Rid[1].dwFlags = 0;
  Rid[1].hwndTarget = nullptr;

  if (RegisterRawInputDevices(&Rid[0], (UINT)2, sizeof(RAWINPUTDEVICE)) == FALSE)
  {
    WLog::Error("Could not initialize RawInput for Mouse and Keyboard input.");
  }
  else
  {
    WLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
}

void WInputDeviceMouseKeyboard_Win::InitializeDevice()
{
  if (s_pGlobalInputHandler == this)
  {
    RegisterRawInput();
  }
  else
  {
    WLog::Dev("Window doesn't handle global mouse/keyboard input.");
  }
}

void WInputDeviceMouseKeyboard_Win::SetDisableOSHotkeys(bool bDisable)
{
  if (m_bDisableOSHotkeys != bDisable)
  {
    m_bDisableOSHotkeys = bDisable;

    if (s_pGlobalInputHandler == this)
    {
      RegisterRawInput();
    }
  }
}

void WInputDeviceMouseKeyboard_Win::RegisterInputSlots()
{
  RegisterInputSlot(WInputSlot_KeyLeft, "Left", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRight, "Right", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyUp, "Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyDown, "Down", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyEscape, "Escape", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeySpace, "Space", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBackspace, "Backspace", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyReturn, "Return", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyTab, "Tab", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftShift, "Left Shift", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightShift, "Right Shift", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftCtrl, "Left Ctrl", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightCtrl, "Right Ctrl", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftAlt, "Left Alt", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightAlt, "Right Alt", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyLeftWin, "Left Win", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyRightWin, "Right Win", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyBracketOpen, "[", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBracketClose, "]", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeySemicolon, ";", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyApostrophe, "'", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeySlash, "/", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyEquals, "=", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyTilde, "~", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyHyphen, "-", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyComma, ",", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPeriod, ".", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyBackslash, "\\", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPipe, "|", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_Key1, "1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key2, "2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key3, "3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key4, "4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key5, "5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key6, "6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key7, "7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key8, "8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key9, "9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_Key0, "0", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyNumpad1, "Numpad 1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad2, "Numpad 2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad3, "Numpad 3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad4, "Numpad 4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad5, "Numpad 5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad6, "Numpad 6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad7, "Numpad 7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad8, "Numpad 8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad9, "Numpad 9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpad0, "Numpad 0", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyA, "A", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyB, "B", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyC, "C", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyD, "D", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyE, "E", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF, "F", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyG, "G", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyH, "H", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyI, "I", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyJ, "J", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyK, "K", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyL, "L", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyM, "M", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyN, "N", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyO, "O", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyP, "P", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyQ, "Q", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyR, "R", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyS, "S", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyT, "T", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyU, "U", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyV, "V", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyW, "W", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyX, "X", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyY, "Y", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyZ, "Z", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyF1, "F1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF2, "F2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF3, "F3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF4, "F4", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF5, "F5", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF6, "F6", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF7, "F7", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF8, "F8", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF9, "F9", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF10, "F10", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF11, "F11", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyF12, "F12", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyHome, "Home", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyEnd, "End", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyDelete, "Delete", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyInsert, "Insert", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPageUp, "Page Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPageDown, "Page Down", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyNumLock, "Numlock", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadPlus, "Numpad +", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadMinus, "Numpad -", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadStar, "Numpad *", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadSlash, "Numpad /", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadPeriod, "Numpad .", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNumpadEnter, "Enter", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyCapsLock, "Capslock", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPrint, "Print", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyScroll, "Scroll", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPause, "Pause", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyApps, "Application", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_KeyPrevTrack, "Previous Track", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyNextTrack, "Next Track", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyPlayPause, "Play / Pause", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyStop, "Stop", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyVolumeUp, "Volume Up", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyVolumeDown, "Volume Down", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_KeyMute, "Mute", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_MouseWheelUp, "Mousewheel Up", WInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(WInputSlot_MouseWheelDown, "Mousewheel Down", WInputSlotFlags::IsMouseWheel);

  RegisterInputSlot(WInputSlot_MouseMoveNegX, "Mouse Move Left", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMovePosX, "Mouse Move Right", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMoveNegY, "Mouse Move Down", WInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(WInputSlot_MouseMovePosY, "Mouse Move Up", WInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(WInputSlot_MouseButton0, "Mousebutton 0", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton1, "Mousebutton 1", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton2, "Mousebutton 2", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton3, "Mousebutton 3", WInputSlotFlags::IsButton);
  RegisterInputSlot(WInputSlot_MouseButton4, "Mousebutton 4", WInputSlotFlags::IsButton);

  RegisterInputSlot(WInputSlot_MouseDblClick0, "Left Double Click", WInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(WInputSlot_MouseDblClick1, "Right Double Click", WInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(WInputSlot_MouseDblClick2, "Middle Double Click", WInputSlotFlags::IsDoubleClick);

  RegisterInputSlot(WInputSlot_MousePositionX, "Mouse Position X", WInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(WInputSlot_MousePositionY, "Mouse Position Y", WInputSlotFlags::IsMouseAxisPosition);


  RegisterInputSlot(WInputSlot_TouchPoint0, "Touchpoint 0", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint0_PositionX, "Touchpoint 0 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint0_PositionY, "Touchpoint 0 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint1, "Touchpoint 1", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint1_PositionX, "Touchpoint 1 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint1_PositionY, "Touchpoint 1 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint2, "Touchpoint 2", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint2_PositionX, "Touchpoint 2 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint2_PositionY, "Touchpoint 2 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint3, "Touchpoint 3", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint3_PositionX, "Touchpoint 3 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint3_PositionY, "Touchpoint 3 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint4, "Touchpoint 4", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint4_PositionX, "Touchpoint 4 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint4_PositionY, "Touchpoint 4 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint5, "Touchpoint 5", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint5_PositionX, "Touchpoint 5 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint5_PositionY, "Touchpoint 5 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint6, "Touchpoint 6", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint6_PositionX, "Touchpoint 6 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint6_PositionY, "Touchpoint 6 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint7, "Touchpoint 7", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint7_PositionX, "Touchpoint 7 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint7_PositionY, "Touchpoint 7 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint8, "Touchpoint 8", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint8_PositionX, "Touchpoint 8 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint8_PositionY, "Touchpoint 8 Position Y", WInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(WInputSlot_TouchPoint9, "Touchpoint 9", WInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(WInputSlot_TouchPoint9_PositionX, "Touchpoint 9 Position X", WInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(WInputSlot_TouchPoint9_PositionY, "Touchpoint 9 Position Y", WInputSlotFlags::IsTouchPosition);
}

void WInputDeviceMouseKeyboard_Win::ResetInputSlotValues()
{
  m_InputSlotValues[WInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[WInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[WInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[WInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[WInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[WInputSlot_MouseMovePosY] = 0;
  m_InputSlotValues[WInputSlot_MouseDblClick0] = 0;
  m_InputSlotValues[WInputSlot_MouseDblClick1] = 0;
  m_InputSlotValues[WInputSlot_MouseDblClick2] = 0;
}

void WInputDeviceMouseKeyboard_Win::UpdateInputSlotValues()
{
  const char* slotDown[5] = {WInputSlot_MouseButton0, WInputSlot_MouseButton1, WInputSlot_MouseButton2, WInputSlot_MouseButton3, WInputSlot_MouseButton4};

  // don't read uninitialized values
  if (!m_InputSlotValues.Contains(slotDown[4]))
  {
    for (int i = 0; i < 5; ++i)
    {
      m_InputSlotValues[slotDown[i]] = 0;
    }
  }

  for (int i = 0; i < 5; ++i)
  {
    if (m_InputSlotValues[slotDown[i]] > 0)
    {
      if (m_uiMouseButtonReceivedUp[i] > 0)
      {
        --m_uiMouseButtonReceivedUp[i];
        m_InputSlotValues[slotDown[i]] = 0;
      }
    }
    else
    {
      if (m_uiMouseButtonReceivedDown[i] > 0)
      {
        --m_uiMouseButtonReceivedDown[i];
        m_InputSlotValues[slotDown[i]] = 1.0f;
      }
      // This is a workaround for a win32 bug: Double clicking on a title bar maximizes a window but only fires a single mouse up event. If that happens, no further clicks would be recognized because the balance between up and down events is broken. So if the slot is not signaled and there is no down event but an up event instead, we just consume it.
      else if (m_uiMouseButtonReceivedUp[i] > 0)
      {
        --m_uiMouseButtonReceivedUp[i];
        m_InputSlotValues[slotDown[i]] = 0;
      }
    }
  }

  SUPER::UpdateInputSlotValues();
}

void WInputDeviceMouseKeyboard_Win::ApplyClipRect(WMouseCursorClipMode::Enum mode)
{
  if (!m_bApplyClipRect)
    return;

  m_bApplyClipRect = false;

  if (mode == WMouseCursorClipMode::NoClip)
  {
    ClipCursor(nullptr);
    return;
  }

  RECT r;
  {
    RECT area;
    GetClientRect(WMinWindows::ToNative(m_hWnd), &area);
    POINT p0, p1;
    p0.x = 0;
    p0.y = 0;
    p1.x = area.right;
    p1.y = area.bottom;

    ClientToScreen(WMinWindows::ToNative(m_hWnd), &p0);
    ClientToScreen(WMinWindows::ToNative(m_hWnd), &p1);

    r.top = p0.y;
    r.left = p0.x;
    r.right = p1.x;
    r.bottom = p1.y;
  }

  if (mode == WMouseCursorClipMode::ClipToPosition)
  {
    POINT mp;
    if (GetCursorPos(&mp))
    {
      // center the position inside the window rect
      mp.x = r.left + (r.right - r.left) / 2;
      mp.y = r.top + (r.bottom - r.top) / 2;

      r.top = mp.y;
      r.bottom = mp.y;
      r.left = mp.x;
      r.right = mp.x;
    }
  }

  ClipCursor(&r);
}

void WInputDeviceMouseKeyboard_Win::ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode)
{
  m_bApplyClipRect = mode != WMouseCursorClipMode::NoClip;

  if (mode == WMouseCursorClipMode::NoClip)
    ClipCursor(nullptr);
}

// WM_INPUT mouse clicks do not work in some VMs.
// When this is enabled, mouse clicks are retrieved via standard WM_LBUTTONDOWN.
#  define W_MOUSEBUTTON_COMPATIBILTY_MODE W_ON

void WInputDeviceMouseKeyboard_Win::WindowMessage(WMinWindows::UINT msg, WMinWindows::WPARAM wparam, WMinWindows::LPARAM lparam)
{
#  if W_ENABLED(W_MOUSEBUTTON_COMPATIBILTY_MODE)
  static WInt32 s_iMouseCaptureCount = 0;
#  endif

  if (m_bFirstWndMsg && m_ClipModeEffective != WMouseCursorClipMode::NoClip)
  {
    // hack fix to make sure the mouse is in the window center and gets clipped to the window, on startup

    m_bFirstWndMsg = false;
    ApplyClipRect(m_ClipModeEffective);

    RECT r;
    GetWindowRect(WMinWindows::ToNative(m_hWnd), &r);
    SetCursorPos(WMath::Lerp(r.left, r.right, 0.5f), WMath::Lerp(r.bottom, r.top, 0.5f));
  }

  switch (msg)
  {
    case WM_MOUSEWHEEL:
    {
      // The mousewheel does not work with rawinput over touchpads (at least not all)
      // So we handle that one individually

      const WInt32 iRotated = (WInt16)HIWORD(wparam);

      if (iRotated > 0)
        m_InputSlotValues[WInputSlot_MouseWheelUp] = iRotated / 120.0f;
      else
        m_InputSlotValues[WInputSlot_MouseWheelDown] = iRotated / -120.0f;

      break;
    }

    case WM_MOUSEMOVE:
    {
      RECT area;
      GetClientRect(WMinWindows::ToNative(m_hWnd), &area);

      const WUInt32 uiResX = area.right - area.left;
      const WUInt32 uiResY = area.bottom - area.top;

      const float fPosX = (float)((short)LOWORD(lparam));
      const float fPosY = (float)((short)HIWORD(lparam));

      s_pMouseOver = this;
      m_vLocalMouseCoordinates.x = (fPosX / uiResX);
      m_vLocalMouseCoordinates.y = (fPosY / uiResY);

      if (s_pGlobalInputHandler == this)
      {
        // only the 'main' window (the first one created) provides its mouse coordinates as the 'global' ones
        // if you have multiple windows and want window specific mouse pointer handling,
        // you need to ask the window specific input device, for its GetLocalMouseCoordinates() and IsMouseOver()
        m_InputSlotValues[WInputSlot_MousePositionX] = m_vLocalMouseCoordinates.x;
        m_InputSlotValues[WInputSlot_MousePositionY] = m_vLocalMouseCoordinates.y;
      }

      if (m_ClipModeEffective == WMouseCursorClipMode::ClipToPosition || m_ClipModeEffective == WMouseCursorClipMode::ClipToWindowImmediate)
      {
        ApplyClipRect(m_ClipModeEffective);
      }

      break;
    }

    case WM_SETFOCUS:
    {
      m_bApplyClipRect = true;
      ApplyClipRect(m_ClipModeEffective);
      break;
    }

    case WM_KILLFOCUS:
    {
      OnFocusLost();
      return;
    }

    case WM_CHAR:
    {
      WUInt32 uiCharacter = (wchar_t)wparam;
      if (uiCharacter == 13) // turn '\r' into '\n'
      {
        uiCharacter = '\n';
      }
      m_sLastCharacters.Append(uiCharacter);
      return;
    }

      // these messages would only arrive, if the window had the flag CS_DBLCLKS
      // see https://docs.microsoft.com/windows/win32/inputdev/wm-lbuttondblclk
      // this would add lag and hide single clicks when the user double clicks
      // therefore it is not used
      // case WM_LBUTTONDBLCLK:
      //  m_InputSlotValues[WInputSlot_MouseDblClick0] = 1.0f;
      //  return;
      // case WM_RBUTTONDBLCLK:
      //  m_InputSlotValues[WInputSlot_MouseDblClick1] = 1.0f;
      //  return;
      // case WM_MBUTTONDBLCLK:
      //  m_InputSlotValues[WInputSlot_MouseDblClick2] = 1.0f;
      //  return;

#  if W_ENABLED(W_MOUSEBUTTON_COMPATIBILTY_MODE)

    case WM_LBUTTONDOWN:
      m_uiMouseButtonReceivedDown[0]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(WMinWindows::ToNative(m_hWnd));
      ++s_iMouseCaptureCount;


      return;

    case WM_LBUTTONUP:
      m_uiMouseButtonReceivedUp[0]++;
      m_bApplyClipRect |= m_bFirstClick;
      m_bFirstClick = false;
      ApplyClipRect(m_ClipModeEffective);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_RBUTTONDOWN:
      m_uiMouseButtonReceivedDown[1]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(WMinWindows::ToNative(m_hWnd));
      ++s_iMouseCaptureCount;

      return;

    case WM_RBUTTONUP:
      m_uiMouseButtonReceivedUp[1]++;
      m_bApplyClipRect |= m_bFirstClick;
      ApplyClipRect(m_ClipModeEffective);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();


      return;

    case WM_MBUTTONDOWN:
      m_uiMouseButtonReceivedDown[2]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(WMinWindows::ToNative(m_hWnd));
      ++s_iMouseCaptureCount;
      return;

    case WM_MBUTTONUP:
      m_uiMouseButtonReceivedUp[2]++;

      m_bApplyClipRect |= m_bFirstClick;
      ApplyClipRect(m_ClipModeEffective);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_XBUTTONDOWN:
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
        m_uiMouseButtonReceivedDown[3]++;
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
        m_uiMouseButtonReceivedDown[4]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(WMinWindows::ToNative(m_hWnd));
      ++s_iMouseCaptureCount;

      return;

    case WM_XBUTTONUP:
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
        m_uiMouseButtonReceivedUp[3]++;
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
        m_uiMouseButtonReceivedUp[4]++;

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_CAPTURECHANGED: // Sent to the window that is losing the mouse capture.
      s_iMouseCaptureCount = 0;
      return;

#  else

    case WM_LBUTTONUP:
      ApplyClipRect(m_ClipModeEffective);
      return;

#  endif

    case WM_USER_UPDATE_CURSOR:
      ShowCursor(static_cast<BOOL>(wparam));
      return;

    case WM_INPUT:
    {
      WUInt32 uiSize = 0;

      GetRawInputData((HRAWINPUT)lparam, RID_INPUT, nullptr, &uiSize, sizeof(RAWINPUTHEADER));

      if (uiSize == 0)
        return;

      WTempHybridArray<WUInt8, sizeof(RAWINPUT)> InputData;
      InputData.SetCountUninitialized(uiSize);

      if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &InputData[0], &uiSize, sizeof(RAWINPUTHEADER)) != uiSize)
        return;

      RAWINPUT* raw = (RAWINPUT*)&InputData[0];

      if (raw->header.dwType == RIM_TYPEKEYBOARD)
      {
        static bool bIgnoreNext = false;

        if (bIgnoreNext)
        {
          bIgnoreNext = false;
          return;
        }

        static bool bWasStupidLeftShift = false;

        const WUInt8 uiScanCode = static_cast<WUInt8>(raw->data.keyboard.MakeCode);
        const bool bIsExtended = (raw->data.keyboard.Flags & RI_KEY_E0) != 0;

        if (uiScanCode == 42 && bIsExtended) // 42 has to be special I guess
        {
          bWasStupidLeftShift = true;
          return;
        }

        WStringView sInputSlotName = WInputManager::ConvertScanCodeToEngineName(uiScanCode, bIsExtended);

        // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
        // so we need to fix this manually
        if (raw->data.keyboard.Flags & RI_KEY_E1)
        {
          sInputSlotName = WInputSlot_KeyPause;
          bIgnoreNext = true;
        }

        // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
        // we ignore the first stupid shift key entirely and then modify the following Numpad* key
        // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
        // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
        if (sInputSlotName == WInputSlot_KeyNumpadStar && bWasStupidLeftShift)
          sInputSlotName = WInputSlot_KeyPrint;

        bWasStupidLeftShift = false;

        const bool bPressed = !(raw->data.keyboard.Flags & 0x01);

        m_InputSlotValues[sInputSlotName] = bPressed ? 1.0f : 0.0f;

        if ((m_InputSlotValues[WInputSlot_KeyLeftCtrl] > 0.1f) && (m_InputSlotValues[WInputSlot_KeyLeftAlt] > 0.1f) &&
            (m_InputSlotValues[WInputSlot_KeyNumpadEnter] > 0.1f))
        {
          switch (GetClipMouseCursor())
          {
            case WMouseCursorClipMode::NoClip:
              SetClipMouseCursor(WMouseCursorClipMode::ClipToWindow);
              break;

            default:
              SetClipMouseCursor(WMouseCursorClipMode::NoClip);
              break;
          }
        }
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        const WUInt32 uiButtons = raw->data.mouse.usButtonFlags;

        // "absolute" positions are only reported by devices such as Pens
        // if at all, we should handle them as touch points, not as mouse positions
        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
          const float fMouseScale = 1.0f / 10.0f;

          // values are reported by the mouse and are resolution independent
          // however, they can be extremely sensitive especially on high DPI mice
          // so we scale the value down by a constant factor, so that they are closer to what we get in other implementations
          // were we have to use absolute mouse coordinate differences
          m_InputSlotValues[WInputSlot_MouseMoveNegX] +=
            ((raw->data.mouse.lLastX < 0) ? (float)-raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x * fMouseScale;
          m_InputSlotValues[WInputSlot_MouseMovePosX] +=
            ((raw->data.mouse.lLastX > 0) ? (float)raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x * fMouseScale;
          m_InputSlotValues[WInputSlot_MouseMoveNegY] +=
            ((raw->data.mouse.lLastY < 0) ? (float)-raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y * fMouseScale;
          m_InputSlotValues[WInputSlot_MouseMovePosY] +=
            ((raw->data.mouse.lLastY > 0) ? (float)raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y * fMouseScale;

// Mouse input does not always work via WM_INPUT
// e.g. some VMs don't send mouse click input via WM_INPUT when the mouse cursor is visible
// therefore in 'compatibility mode' it is just queried via standard WM_LBUTTONDOWN etc.
// to get 'high performance' mouse clicks, this code would work fine though
// but I doubt it makes much difference in latency
#  if W_DISABLED(W_MOUSEBUTTON_COMPATIBILTY_MODE)
          for (WInt32 mb = 0; mb < 5; ++mb)
          {
            char szTemp[32];
            WStringUtils::snprintf(szTemp, 32, "mouse_button_%i", mb);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2))) != 0)
              m_InputSlotValues[szTemp] = 1.0f;

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2 + 1))) != 0)
              m_InputSlotValues[szTemp] = 0.0f;
          }
#  endif
        }
        else if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
          if ((raw->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0)
          {
            // if this flag is set, we are getting mouse input through a remote desktop session
            // and that means we will not get any relative mouse move events, so we need to emulate them

            static const WInt32 iVirtualDesktopW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            static const WInt32 iVirtualDesktopH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            static WVec2 vLastPos(WMath::MaxValue<float>());
            const WVec2 vNewPos(
              (raw->data.mouse.lLastX / 65535.0f) * iVirtualDesktopW, (raw->data.mouse.lLastY / 65535.0f) * iVirtualDesktopH);

            if (vLastPos.x != WMath::MaxValue<float>())
            {
              const WVec2 vDiff = vNewPos - vLastPos;

              m_InputSlotValues[WInputSlot_MouseMoveNegX] += ((vDiff.x < 0) ? (float)-vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[WInputSlot_MouseMovePosX] += ((vDiff.x > 0) ? (float)vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[WInputSlot_MouseMoveNegY] += ((vDiff.y < 0) ? (float)-vDiff.y : 0.0f) * GetMouseSpeed().y;
              m_InputSlotValues[WInputSlot_MouseMovePosY] += ((vDiff.y > 0) ? (float)vDiff.y : 0.0f) * GetMouseSpeed().y;
            }

            vLastPos = vNewPos;
          }
          else
          {
            static int iTouchPoint = 0;

            WStringView sSlot = WInputManager::GetInputSlotTouchPoint(iTouchPoint);
            WStringView sSlotX = WInputManager::GetInputSlotTouchPointPositionX(iTouchPoint);
            WStringView sSlotY = WInputManager::GetInputSlotTouchPointPositionY(iTouchPoint);

            m_InputSlotValues[sSlotX] = (raw->data.mouse.lLastX / 65535.0f);
            m_InputSlotValues[sSlotY] = (raw->data.mouse.lLastY / 65535.0f);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN)) != 0)
            {
              m_InputSlotValues[sSlot] = 1.0f;
            }

            if ((uiButtons & (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP)) != 0)
            {
              m_InputSlotValues[sSlot] = 0.0f;
            }
          }
        }
        else
        {
          WLog::Info("Unknown Mouse Move: {0} | {1}, Flags = {2}", WArgF(raw->data.mouse.lLastX, 1), WArgF(raw->data.mouse.lLastY, 1),
            (WUInt32)raw->data.mouse.usFlags);
        }
      }
    }
  }
}


static void SetKeyNameForScanCode(int iScanCode, bool bExtended, const char* szInputSlot)
{
  const WUInt32 uiKeyCode = (iScanCode << 16) | (bExtended ? (1 << 24) : 0);

  wchar_t szKeyName[32] = {0};
  GetKeyNameTextW(uiKeyCode, szKeyName, 30);

  WStringUtf8 sName(szKeyName);

  WLog::Dev("Translated '{0}' to '{1}'", WInputManager::GetInputSlotDisplayName(szInputSlot), sName.GetData());

  WInputManager::SetInputSlotDisplayName(szInputSlot, sName.GetData());
}

void WInputDeviceMouseKeyboard_Win::LocalizeButtonDisplayNames()
{
  W_LOG_BLOCK("WInputDeviceMouseKeyboard_Win::LocalizeButtonDisplayNames");

  SetKeyNameForScanCode(1, false, WInputSlot_KeyEscape);
  SetKeyNameForScanCode(2, false, WInputSlot_Key1);
  SetKeyNameForScanCode(3, false, WInputSlot_Key2);
  SetKeyNameForScanCode(4, false, WInputSlot_Key3);
  SetKeyNameForScanCode(5, false, WInputSlot_Key4);
  SetKeyNameForScanCode(6, false, WInputSlot_Key5);
  SetKeyNameForScanCode(7, false, WInputSlot_Key6);
  SetKeyNameForScanCode(8, false, WInputSlot_Key7);
  SetKeyNameForScanCode(9, false, WInputSlot_Key8);
  SetKeyNameForScanCode(10, false, WInputSlot_Key9);
  SetKeyNameForScanCode(11, false, WInputSlot_Key0);

  SetKeyNameForScanCode(12, false, WInputSlot_KeyHyphen);
  SetKeyNameForScanCode(13, false, WInputSlot_KeyEquals);
  SetKeyNameForScanCode(14, false, WInputSlot_KeyBackspace);

  SetKeyNameForScanCode(15, false, WInputSlot_KeyTab);
  SetKeyNameForScanCode(16, false, WInputSlot_KeyQ);
  SetKeyNameForScanCode(17, false, WInputSlot_KeyW);
  SetKeyNameForScanCode(18, false, WInputSlot_KeyE);
  SetKeyNameForScanCode(19, false, WInputSlot_KeyR);
  SetKeyNameForScanCode(20, false, WInputSlot_KeyT);
  SetKeyNameForScanCode(21, false, WInputSlot_KeyY);
  SetKeyNameForScanCode(22, false, WInputSlot_KeyU);
  SetKeyNameForScanCode(23, false, WInputSlot_KeyI);
  SetKeyNameForScanCode(24, false, WInputSlot_KeyO);
  SetKeyNameForScanCode(25, false, WInputSlot_KeyP);
  SetKeyNameForScanCode(26, false, WInputSlot_KeyBracketOpen);
  SetKeyNameForScanCode(27, false, WInputSlot_KeyBracketClose);
  SetKeyNameForScanCode(28, false, WInputSlot_KeyReturn);

  SetKeyNameForScanCode(29, false, WInputSlot_KeyLeftCtrl);
  SetKeyNameForScanCode(30, false, WInputSlot_KeyA);
  SetKeyNameForScanCode(31, false, WInputSlot_KeyS);
  SetKeyNameForScanCode(32, false, WInputSlot_KeyD);
  SetKeyNameForScanCode(33, false, WInputSlot_KeyF);
  SetKeyNameForScanCode(34, false, WInputSlot_KeyG);
  SetKeyNameForScanCode(35, false, WInputSlot_KeyH);
  SetKeyNameForScanCode(36, false, WInputSlot_KeyJ);
  SetKeyNameForScanCode(37, false, WInputSlot_KeyK);
  SetKeyNameForScanCode(38, false, WInputSlot_KeyL);
  SetKeyNameForScanCode(39, false, WInputSlot_KeySemicolon);
  SetKeyNameForScanCode(40, false, WInputSlot_KeyApostrophe);

  SetKeyNameForScanCode(41, false, WInputSlot_KeyTilde);
  SetKeyNameForScanCode(42, false, WInputSlot_KeyLeftShift);
  SetKeyNameForScanCode(43, false, WInputSlot_KeyBackslash);

  SetKeyNameForScanCode(44, false, WInputSlot_KeyZ);
  SetKeyNameForScanCode(45, false, WInputSlot_KeyX);
  SetKeyNameForScanCode(46, false, WInputSlot_KeyC);
  SetKeyNameForScanCode(47, false, WInputSlot_KeyV);
  SetKeyNameForScanCode(48, false, WInputSlot_KeyB);
  SetKeyNameForScanCode(49, false, WInputSlot_KeyN);
  SetKeyNameForScanCode(50, false, WInputSlot_KeyM);
  SetKeyNameForScanCode(51, false, WInputSlot_KeyComma);
  SetKeyNameForScanCode(52, false, WInputSlot_KeyPeriod);
  SetKeyNameForScanCode(53, false, WInputSlot_KeySlash);
  SetKeyNameForScanCode(54, false, WInputSlot_KeyRightShift);

  SetKeyNameForScanCode(55, false, WInputSlot_KeyNumpadStar); // Overlaps with Print

  SetKeyNameForScanCode(56, false, WInputSlot_KeyLeftAlt);
  SetKeyNameForScanCode(57, false, WInputSlot_KeySpace);
  SetKeyNameForScanCode(58, false, WInputSlot_KeyCapsLock);

  SetKeyNameForScanCode(59, false, WInputSlot_KeyF1);
  SetKeyNameForScanCode(60, false, WInputSlot_KeyF2);
  SetKeyNameForScanCode(61, false, WInputSlot_KeyF3);
  SetKeyNameForScanCode(62, false, WInputSlot_KeyF4);
  SetKeyNameForScanCode(63, false, WInputSlot_KeyF5);
  SetKeyNameForScanCode(64, false, WInputSlot_KeyF6);
  SetKeyNameForScanCode(65, false, WInputSlot_KeyF7);
  SetKeyNameForScanCode(66, false, WInputSlot_KeyF8);
  SetKeyNameForScanCode(67, false, WInputSlot_KeyF9);
  SetKeyNameForScanCode(68, false, WInputSlot_KeyF10);

  SetKeyNameForScanCode(69, true, WInputSlot_KeyNumLock);       // Prints 'Pause' if it is not 'extended'
  SetKeyNameForScanCode(70, false, WInputSlot_KeyScroll);       // This overlaps with Pause

  SetKeyNameForScanCode(71, false, WInputSlot_KeyNumpad7);      // This overlaps with Home
  SetKeyNameForScanCode(72, false, WInputSlot_KeyNumpad8);      // This overlaps with Arrow Up
  SetKeyNameForScanCode(73, false, WInputSlot_KeyNumpad9);      // This overlaps with Page Up
  SetKeyNameForScanCode(74, false, WInputSlot_KeyNumpadMinus);

  SetKeyNameForScanCode(75, false, WInputSlot_KeyNumpad4);      // This overlaps with Arrow Left
  SetKeyNameForScanCode(76, false, WInputSlot_KeyNumpad5);
  SetKeyNameForScanCode(77, false, WInputSlot_KeyNumpad6);      // This overlaps with Arrow Right
  SetKeyNameForScanCode(78, false, WInputSlot_KeyNumpadPlus);

  SetKeyNameForScanCode(79, false, WInputSlot_KeyNumpad1);      // This overlaps with End
  SetKeyNameForScanCode(80, false, WInputSlot_KeyNumpad2);      // This overlaps with Arrow Down
  SetKeyNameForScanCode(81, false, WInputSlot_KeyNumpad3);      // This overlaps with Page Down
  SetKeyNameForScanCode(82, false, WInputSlot_KeyNumpad0);      // This overlaps with Insert
  SetKeyNameForScanCode(83, false, WInputSlot_KeyNumpadPeriod); // This overlaps with Insert

  SetKeyNameForScanCode(86, false, WInputSlot_KeyPipe);

  SetKeyNameForScanCode(87, false, "keyboard_f11");
  SetKeyNameForScanCode(88, false, "keyboard_f12");

  SetKeyNameForScanCode(91, true, WInputSlot_KeyLeftWin);  // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(92, true, WInputSlot_KeyRightWin); // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(93, true, WInputSlot_KeyApps);     // Prints '' if it is not 'extended'

  // 'Extended' keys
  SetKeyNameForScanCode(28, true, WInputSlot_KeyNumpadEnter);
  SetKeyNameForScanCode(29, true, WInputSlot_KeyRightCtrl);
  SetKeyNameForScanCode(53, true, WInputSlot_KeyNumpadSlash);
  SetKeyNameForScanCode(55, true, WInputSlot_KeyPrint);
  SetKeyNameForScanCode(56, true, WInputSlot_KeyRightAlt);
  SetKeyNameForScanCode(70, true, WInputSlot_KeyPause);
  SetKeyNameForScanCode(71, true, WInputSlot_KeyHome);
  SetKeyNameForScanCode(72, true, WInputSlot_KeyUp);
  SetKeyNameForScanCode(73, true, WInputSlot_KeyPageUp);

  SetKeyNameForScanCode(75, true, WInputSlot_KeyLeft);
  SetKeyNameForScanCode(77, true, WInputSlot_KeyRight);

  SetKeyNameForScanCode(79, true, WInputSlot_KeyEnd);
  SetKeyNameForScanCode(80, true, WInputSlot_KeyDown);
  SetKeyNameForScanCode(81, true, WInputSlot_KeyPageDown);
  SetKeyNameForScanCode(82, true, WInputSlot_KeyInsert);
  SetKeyNameForScanCode(83, true, WInputSlot_KeyDelete);
}

void WInputDeviceMouseKeyboard_Win::ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive)
{
  W_IGNORE_UNUSED(bCustomCursorActive);

  // note that ::ShowCursor() maintains an internal counter, rather than a boolean state,
  // so this may only ever be called when the state really changed, which the base class guarantees

  if (WThreadUtils::IsMainThread())
  {
    ShowCursor(bShow);
  }
  else
  {
    PostMessageW(WMinWindows::ToNative(m_hWnd), WM_USER_UPDATE_CURSOR, bShow ? 1 : 0, 0);
  }
}

WUInt32 WInputDeviceMouseKeyboard_Win::GetHardwareCursorSize() const
{
  // SM_CXCURSOR reflects the user's 'mouse pointer size' accessibility setting.
  // Asking for the window's DPI on top of that gives the size in physical pixels, which is what a
  // custom cursor has to be rendered at to match the OS cursor on any monitor.
  if (m_hWnd != nullptr)
  {
    const UINT uiDpi = GetDpiForWindow(WMinWindows::ToNative(m_hWnd));

    if (uiDpi != 0)
    {
      return (WUInt32)GetSystemMetricsForDpi(SM_CXCURSOR, uiDpi);
    }
  }

  return (WUInt32)GetSystemMetrics(SM_CXCURSOR);
}

void WInputDeviceMouseKeyboard_Win::OnFocusLost()
{
  if (s_pMouseOver == this)
  {
    s_pMouseOver = nullptr;
  }

  m_bApplyClipRect = true;
  ApplyClipRect(WMouseCursorClipMode::NoClip);

  auto it = m_InputSlotValues.GetIterator();

  while (it.IsValid())
  {
    it.Value() = 0.0f;
    it.Next();
  }


  const char* slotDown[5] = {WInputSlot_MouseButton0, WInputSlot_MouseButton1, WInputSlot_MouseButton2, WInputSlot_MouseButton3, WInputSlot_MouseButton4};

  static_assert(W_ARRAY_SIZE(m_uiMouseButtonReceivedDown) == W_ARRAY_SIZE(slotDown));

  for (int i = 0; i < W_ARRAY_SIZE(m_uiMouseButtonReceivedDown); ++i)
  {
    m_uiMouseButtonReceivedDown[i] = 0;
    m_uiMouseButtonReceivedUp[i] = 0;

    m_InputSlotValues[slotDown[i]] = 0;
  }
}

#endif


W_STATICLINK_FILE(Core, Core_Platform_Win_InputDevice_Win);
