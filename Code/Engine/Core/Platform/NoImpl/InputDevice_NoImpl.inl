#include <Core/Platform/NoImpl/InputDevice_NoImpl.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInputDeviceMouseKeyboard_NoImpl, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WInputDeviceMouseKeyboard_NoImpl::WInputDeviceMouseKeyboard_NoImpl(WUInt32 uiWindowNumber) {}
WInputDeviceMouseKeyboard_NoImpl::~WInputDeviceMouseKeyboard_NoImpl() = default;

void WInputDeviceMouseKeyboard_NoImpl::ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive)
{
  W_IGNORE_UNUSED(bShow);
  W_IGNORE_UNUSED(bCustomCursorActive);
}

void WInputDeviceMouseKeyboard_NoImpl::ApplyClipMouseCursor(WMouseCursorClipMode::Enum mode)
{
  W_IGNORE_UNUSED(mode);
}

void WInputDeviceMouseKeyboard_NoImpl::InitializeDevice() {}

void WInputDeviceMouseKeyboard_NoImpl::RegisterInputSlots() {}
