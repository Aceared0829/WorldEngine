#pragma once

namespace WMinWindows
{
  using BOOL = int;
  using DWORD = unsigned long;
  using UINT = unsigned int;
  using LPSTR = char*;
  struct WHINSTANCE;
  using HINSTANCE = WHINSTANCE*;
  using HMODULE = HINSTANCE;
  struct WHWND;
  using HWND = WHWND*;
  using HRESULT = long;
  using HANDLE = void*;

#if W_ENABLED(W_PLATFORM_64BIT)
  using WPARAM = WUInt64;
  using LPARAM = WUInt64;
#else
  using WPARAM = WUInt32;
  using LPARAM = WUInt32;
#endif

  template <typename T>
  struct ToNativeImpl
  {
  };

  template <typename T>
  struct FromNativeImpl
  {
  };

  /// Helper function to convert WMinWindows types into native windows.h types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  W_ALWAYS_INLINE typename ToNativeImpl<T>::type ToNative(T t)
  {
    return ToNativeImpl<T>::ToNative(t);
  }

  /// Helper function to native windows.h types to WMinWindows types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  W_ALWAYS_INLINE typename FromNativeImpl<T>::type FromNative(T t)
  {
    return FromNativeImpl<T>::FromNative(t);
  }
} // namespace WMinWindows
#define W_WINDOWS_CALLBACK __stdcall
#define W_WINDOWS_WINAPI __stdcall
#define W_WINDOWS_INVALID_HANDLE_VALUE ((void*)(long long)-1)
