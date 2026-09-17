#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringView.h>

/// A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32
/// Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class W_FOUNDATION_DLL WStringWChar
{
public:
  WStringWChar(WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringWChar(const WUInt16* pUtf16, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringWChar(const WUInt32* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringWChar(const wchar_t* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringWChar(WStringView sUtf8, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  void operator=(const WUInt16* pUtf16);
  void operator=(const WUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);
  void operator=(WStringView sUtf8);

  W_ALWAYS_INLINE operator const wchar_t*() const { return &m_Data[0]; }
  W_ALWAYS_INLINE const wchar_t* GetData() const { return &m_Data[0]; }
  W_ALWAYS_INLINE WUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr WUInt32 BufferSize = 1024;
  WHybridArray<wchar_t, BufferSize> m_Data;
};


/// A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class W_FOUNDATION_DLL WStringUtf8
{
public:
  WStringUtf8(WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf8(const char* szUtf8, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf8(const WUInt16* pUtf16, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf8(const WUInt32* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf8(const wchar_t* pWChar, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const WUInt16* pUtf16);
  void operator=(const WUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  W_ALWAYS_INLINE operator const char*() const
  {
    return &m_Data[0];
  }
  W_ALWAYS_INLINE const char* GetData() const
  {
    return &m_Data[0];
  }
  W_ALWAYS_INLINE WUInt32 GetElementCount() const
  {
    return m_Data.GetCount() - 1; /* exclude the '\0' terminator */
  }
  W_ALWAYS_INLINE operator WStringView() const
  {
    return GetView();
  }
  W_ALWAYS_INLINE WStringView GetView() const
  {
    return WStringView(&m_Data[0], GetElementCount());
  }

private:
  static constexpr WUInt32 BufferSize = 1024;
  WHybridArray<char, BufferSize> m_Data;
};



/// A very simple class to convert text to Utf16 encoding.
///
/// Use this class only temporarily, if you need to output something in Utf16 format, e.g. for writing it to a file.
/// Never use this for storage.
/// When working with OS functions that expect '16 Bit strings', use WStringWChar instead.
class W_FOUNDATION_DLL WStringUtf16
{
public:
  WStringUtf16(WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf16(const char* szUtf8, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf16(const WUInt16* pUtf16, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf16(const WUInt32* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf16(const wchar_t* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const WUInt16* pUtf16);
  void operator=(const WUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);

  W_ALWAYS_INLINE const WUInt16* GetData() const { return &m_Data[0]; }
  W_ALWAYS_INLINE WUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr WUInt32 BufferSize = 1024;
  WHybridArray<WUInt16, BufferSize> m_Data;
};



/// This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class W_FOUNDATION_DLL WStringUtf32
{
public:
  WStringUtf32(WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf32(const char* szUtf8, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf32(const WUInt16* pUtf16, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf32(const WUInt32* pUtf32, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  WStringUtf32(const wchar_t* pWChar, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const WUInt16* pUtf16);
  void operator=(const WUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  W_ALWAYS_INLINE const WUInt32* GetData() const { return &m_Data[0]; }
  W_ALWAYS_INLINE WUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr WUInt32 BufferSize = 1024;
  WHybridArray<WUInt32, BufferSize> m_Data;
};

#include <Foundation/Strings/Implementation/StringConversion_inl.h>
