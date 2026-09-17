#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Strings/UnicodeUtils.h>

// **************** WStringWChar ****************

inline WStringWChar::WStringWChar(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline WStringWChar::WStringWChar(const WUInt16* pUtf16, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline WStringWChar::WStringWChar(const WUInt32* pUtf32, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline WStringWChar::WStringWChar(const wchar_t* pWChar, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

inline WStringWChar::WStringWChar(WStringView sUtf8, WAllocator* pAllocator /*= WFoundation::GetDefaultAllocator()*/)
  : m_Data(pAllocator)
{
  *this = sUtf8;
}


// **************** WStringUtf8 ****************

inline WStringUtf8::WStringUtf8(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline WStringUtf8::WStringUtf8(const char* szUtf8, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline WStringUtf8::WStringUtf8(const WUInt16* pUtf16, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline WStringUtf8::WStringUtf8(const WUInt32* pUtf32, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline WStringUtf8::WStringUtf8(const wchar_t* pWChar, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

// **************** WStringUtf16 ****************

inline WStringUtf16::WStringUtf16(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline WStringUtf16::WStringUtf16(const char* szUtf8, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline WStringUtf16::WStringUtf16(const WUInt16* pUtf16, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline WStringUtf16::WStringUtf16(const WUInt32* pUtf32, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline WStringUtf16::WStringUtf16(const wchar_t* pWChar, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}



// **************** WStringUtf32 ****************

inline WStringUtf32::WStringUtf32(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline WStringUtf32::WStringUtf32(const char* szUtf8, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline WStringUtf32::WStringUtf32(const WUInt16* pUtf16, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline WStringUtf32::WStringUtf32(const WUInt32* pUtf32, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline WStringUtf32::WStringUtf32(const wchar_t* pWChar, WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}
