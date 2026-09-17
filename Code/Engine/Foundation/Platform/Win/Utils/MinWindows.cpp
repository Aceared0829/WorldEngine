#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <type_traits>

template <typename WType, typename WindowsType, bool mustBeConvertible>
void WVerifyWindowsType()
{
  static_assert(sizeof(WType) == sizeof(WindowsType), "W <=> windows.h size mismatch");
  static_assert(alignof(WType) == alignof(WindowsType), "W <=> windows.h alignment mismatch");
  static_assert(std::is_pointer<WType>::value == std::is_pointer<WindowsType>::value, "W <=> windows.h pointer type mismatch");
  static_assert(!mustBeConvertible || WConversionTest<WType, WindowsType>::exists == 1, "W <=> windows.h conversion failure");
  static_assert(!mustBeConvertible || WConversionTest<WindowsType, WType>::exists == 1, "windows.h <=> W conversion failure");
};

void CALLBACK WindowsCallbackTest1();
void W_WINDOWS_CALLBACK WindowsCallbackTest2();
void WINAPI WindowsWinapiTest1();
void W_WINDOWS_WINAPI WindowsWinapiTest2();

// Will never be called and thus removed by the linker
void WCheckWindowsTypeSizes()
{
  WVerifyWindowsType<WMinWindows::DWORD, DWORD, true>();
  WVerifyWindowsType<WMinWindows::UINT, UINT, true>();
  WVerifyWindowsType<WMinWindows::BOOL, BOOL, true>();
  WVerifyWindowsType<WMinWindows::LPARAM, LPARAM, true>();
  WVerifyWindowsType<WMinWindows::WPARAM, WPARAM, true>();
  WVerifyWindowsType<WMinWindows::HINSTANCE, HINSTANCE, false>();
  WVerifyWindowsType<WMinWindows::HMODULE, HMODULE, false>();
  WVerifyWindowsType<WMinWindows::LPSTR, LPSTR, true>();
  WVerifyWindowsType<WMinWindows::HWND, HWND, false>();
  WVerifyWindowsType<WMinWindows::HRESULT, HRESULT, true>();

  static_assert(std::is_same<decltype(&WindowsCallbackTest1), decltype(&WindowsCallbackTest2)>::value, "W_WINDOWS_CALLBACK does not match CALLBACK");
  static_assert(std::is_same<decltype(&WindowsWinapiTest1), decltype(&WindowsWinapiTest2)>::value, "W_WINDOWS_WINAPI does not match WINAPI");

  // Clang doesn't allow us to do this check at compile time
#  if W_ENABLED(W_COMPILER_MSVC_PURE)
  static_assert(W_WINDOWS_INVALID_HANDLE_VALUE == INVALID_HANDLE_VALUE, "W_WINDOWS_INVALID_HANDLE_VALUE does not match INVALID_HANDLE_VALUE");
#  endif
}
#endif
