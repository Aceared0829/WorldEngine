#include <Core/CorePCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Core/Platform/Android/InputDevice_Platform.h>

#  include <Core/Input/InputManager.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/System/Screen.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDevice_Android, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// Comment in to get verbose output on android input
// #  define DEBUG_ANDROID_INPUT

#  ifdef DEBUG_ANDROID_INPUT
#    define DEBUG_LOG(...) WLog::Debug(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

WInputDevice_Android::WInputDevice_Android()
{
  WAndroidUtils::s_InputEvent.AddEventHandler(WMakeDelegate(&WInputDevice_Android::AndroidInputEventHandler, this));
  WAndroidUtils::s_AppCommandEvent.AddEventHandler(WMakeDelegate(&WInputDevice_Android::AndroidAppCommandEventHandler, this));
}

WInputDevice_Android::~WInputDevice_Android()
{
  WAndroidUtils::s_AppCommandEvent.RemoveEventHandler(WMakeDelegate(&WInputDevice_Android::AndroidAppCommandEventHandler, this));
  WAndroidUtils::s_InputEvent.RemoveEventHandler(WMakeDelegate(&WInputDevice_Android::AndroidInputEventHandler, this));
}

void WInputDevice_Android::InitializeDevice()
{
  WTempHybridArray<WScreenInfo, 2> screens;
  if (WScreen::EnumerateScreens(screens).Succeeded())
  {
    m_iResolutionX = screens[0].m_iResolutionX;
    m_iResolutionY = screens[0].m_iResolutionY;
  }
}

void WInputDevice_Android::UpdateInputSlotValues()
{
  // nothing to do here
}

void WInputDevice_Android::RegisterInputSlots()
{
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

  RegisterInputSlot(WInputSlot_MouseWheelUp, "Mousewheel Up", WInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(WInputSlot_MouseWheelDown, "Mousewheel Down", WInputSlotFlags::IsMouseWheel);
}

void WInputDevice_Android::ResetInputSlotValues()
{
  m_InputSlotValues[WInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[WInputSlot_MouseWheelDown] = 0;
  for (int id = 0; id < 10; ++id)
  {
    // We can't reset the position inside AndroidHandleInput as we want the position to be valid when lifting a finger. Thus, we clear the position here after the update has been performed.
    if (m_InputSlotValues[WInputManager::GetInputSlotTouchPoint(id)] == 0)
    {
      m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionX(id)] = 0;
      m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionY(id)] = 0;
    }
  }
}

void WInputDevice_Android::AndroidInputEventHandler(WAndroidInputEvent& event)
{
  event.m_bHandled = AndroidHandleInput(event.m_pEvent);
  UpdateInputSlotValues();
}

void WInputDevice_Android::AndroidAppCommandEventHandler(WInt32 iCmd)
{
  if (iCmd == APP_CMD_WINDOW_RESIZED)
  {
    WTempHybridArray<WScreenInfo, 2> screens;
    if (WScreen::EnumerateScreens(screens).Succeeded())
    {
      m_iResolutionX = screens[0].m_iResolutionX;
      m_iResolutionY = screens[0].m_iResolutionY;
    }
  }
}

bool WInputDevice_Android::AndroidHandleInput(AInputEvent* pEvent)
{
  // #TODO_ANDROID Only touchscreen input is implemented right now.
  const WInt32 iEventType = AInputEvent_getType(pEvent);
  const WInt32 iEventSource = AInputEvent_getSource(pEvent);
  const WUInt32 uiAction = (WUInt32)AMotionEvent_getAction(pEvent);
  const WInt32 iKeyCode = AKeyEvent_getKeyCode(pEvent);
  const WInt32 iButtonState = AMotionEvent_getButtonState(pEvent);
  W_IGNORE_UNUSED(iKeyCode);
  W_IGNORE_UNUSED(iButtonState);
  DEBUG_LOG("Android INPUT: iEventType: {}, iEventSource: {}, uiAction: {}, iKeyCode: {}, iButtonState: {}", iEventType,
    iEventSource, uiAction, iKeyCode, iButtonState);

  if (m_iResolutionX == 0 || m_iResolutionY == 0)
    return false;

  // I.e. fingers have touched the touchscreen.
  if (iEventType == AINPUT_EVENT_TYPE_MOTION && (iEventSource & AINPUT_SOURCE_TOUCHSCREEN) != 0)
  {
    // Update pointer positions
    const WUInt64 uiPointerCount = AMotionEvent_getPointerCount(pEvent);
    for (WUInt32 uiPointerIndex = 0; uiPointerIndex < uiPointerCount; uiPointerIndex++)
    {
      const float fPixelX = AMotionEvent_getX(pEvent, uiPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiPointerIndex);
      const WInt32 id = AMotionEvent_getPointerId(pEvent, uiPointerIndex);
      if (id < 10)
      {
        m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
        m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
        DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionY(id)]);
      }
    }

    // Update pointer state
    const WUInt32 uiActionEvent = uiAction & AMOTION_EVENT_ACTION_MASK;
    const WUInt32 uiActionPointerIndex = (uiAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const WInt32 id = AMotionEvent_getPointerId(pEvent, uiActionPointerIndex);
    // We only support up to 10 touch points at the same time.
    if (id >= 10)
      return false;

    {
      // Not sure if the action finger is always present in the upper loop of uiPointerCount, so we update it here for good measure.
      const float fPixelX = AMotionEvent_getX(pEvent, uiActionPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiActionPointerIndex);
      m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
      m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
      DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[WInputManager::GetInputSlotTouchPointPositionY(id)]);
    }

    switch (uiActionEvent)
    {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        m_InputSlotValues[WInputManager::GetInputSlotTouchPoint(id)] = 1;
        DEBUG_LOG("Finger DOWN: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_MOVE:
        // Finger moved (we always update that at the top).
        return true;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
      case AMOTION_EVENT_ACTION_CANCEL:
      case AMOTION_EVENT_ACTION_OUTSIDE:
        m_InputSlotValues[WInputManager::GetInputSlotTouchPoint(id)] = 0;
        DEBUG_LOG("Finger UP: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_SCROLL:
      {
        float fRotated = AMotionEvent_getAxisValue(pEvent, AMOTION_EVENT_AXIS_VSCROLL, 0);
        if (fRotated > 0)
          m_InputSlotValues[WInputSlot_MouseWheelUp] = fRotated;
        else
          m_InputSlotValues[WInputSlot_MouseWheelDown] = fRotated;
        return true;
      }
      case AMOTION_EVENT_ACTION_HOVER_ENTER:
      case AMOTION_EVENT_ACTION_HOVER_MOVE:
      case AMOTION_EVENT_ACTION_HOVER_EXIT:
        return false;
      default:
        DEBUG_LOG("Unknown AMOTION_EVENT_ACTION: {}", uiActionEvent);
        return false;
    }
  }
  return false;
}

#endif


W_STATICLINK_FILE(Core, Core_Platform_Android_InputDevice_Android);
