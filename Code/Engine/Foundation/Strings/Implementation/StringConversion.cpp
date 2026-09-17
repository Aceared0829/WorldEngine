#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringConversion.h>

// **************** WStringWChar ****************

void WStringWChar::operator=(const WUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    WUnicodeUtils::SkipUtf16BomLE(pUtf16);
    W_ASSERT_DEV(!WUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    WUnicodeUtils::UtfInserter<wchar_t, WHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringWChar::operator=(const WUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    WUnicodeUtils::UtfInserter<wchar_t, WHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringWChar::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {

    while (*pWChar != '\0')
    {
      m_Data.PushBack(*pWChar);
      ++pWChar;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringWChar::operator=(WStringView sUtf8)
{
  m_Data.Clear();

  if (!sUtf8.IsEmpty())
  {
    const char* szUtf8 = sUtf8.GetStartPointer();

    W_ASSERT_DEV(WUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

    // skip any Utf8 Byte Order Mark
    WUnicodeUtils::SkipUtf8Bom(szUtf8);

    WUnicodeUtils::UtfInserter<wchar_t, WHybridArray<wchar_t, BufferSize>> tempInserter(&m_Data);

    while (szUtf8 < sUtf8.GetEndPointer() && *szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToWChar(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

// **************** WStringUtf8 ****************

void WStringUtf8::operator=(const char* szUtf8)
{
  W_ASSERT_DEV(
    WUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    WUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      m_Data.PushBack(*szUtf8);
      ++szUtf8;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf8::operator=(const WUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    WUnicodeUtils::SkipUtf16BomLE(pUtf16);
    W_ASSERT_DEV(!WUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    WUnicodeUtils::UtfInserter<char, WHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf16 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeUtf16ToUtf32(pUtf16);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf8::operator=(const WUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    WUnicodeUtils::UtfInserter<char, WHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringUtf8::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    WUnicodeUtils::UtfInserter<char, WHybridArray<char, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

#if W_ENABLED(W_PLATFORM_WINDOWS_UWP)

void WStringUtf8::operator=(const Microsoft::WRL::Wrappers::HString& hstring)
{
  WUInt32 len = 0;
  const wchar_t* raw = hstring.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

void WStringUtf8::operator=(const HSTRING& hstring)
{
  Microsoft::WRL::Wrappers::HString tmp;
  tmp.Attach(hstring);

  WUInt32 len = 0;
  const wchar_t* raw = tmp.GetRawBuffer(&len);

  // delegate to wchar_t operator
  *this = raw;
}

#endif


// **************** WStringUtf16 ****************

void WStringUtf16::operator=(const char* szUtf8)
{
  W_ASSERT_DEV(
    WUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    WUnicodeUtils::SkipUtf8Bom(szUtf8);

    WUnicodeUtils::UtfInserter<WUInt16, WHybridArray<WUInt16, BufferSize>> tempInserter(&m_Data);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf16::operator=(const WUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    WUnicodeUtils::SkipUtf16BomLE(pUtf16);
    W_ASSERT_DEV(!WUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      m_Data.PushBack(*pUtf16);
      ++pUtf16;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf16::operator=(const WUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    WUnicodeUtils::UtfInserter<WUInt16, WHybridArray<WUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pUtf32 != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = *pUtf32;
      ++pUtf32;

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringUtf16::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    WUnicodeUtils::UtfInserter<WUInt16, WHybridArray<WUInt16, BufferSize>> tempInserter(&m_Data);

    while (*pWChar != '\0')
    {
      // decode utf8 to utf32
      const WUInt32 uiUtf32 = WUnicodeUtils::DecodeWCharToUtf32(pWChar);

      // encode utf32 to wchar_t
      WUnicodeUtils::EncodeUtf32ToUtf16(uiUtf32, tempInserter);
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}



// **************** WStringUtf32 ****************

void WStringUtf32::operator=(const char* szUtf8)
{
  W_ASSERT_DEV(
    WUnicodeUtils::IsValidUtf8(szUtf8), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

  m_Data.Clear();

  if (szUtf8 != nullptr)
  {
    // skip any Utf8 Byte Order Mark
    WUnicodeUtils::SkipUtf8Bom(szUtf8);

    while (*szUtf8 != '\0')
    {
      // decode utf8 to utf32
      m_Data.PushBack(WUnicodeUtils::DecodeUtf8ToUtf32(szUtf8));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf32::operator=(const WUInt16* pUtf16)
{
  m_Data.Clear();

  if (pUtf16 != nullptr)
  {
    // skip any Utf16 little endian Byte Order Mark
    WUnicodeUtils::SkipUtf16BomLE(pUtf16);
    W_ASSERT_DEV(!WUnicodeUtils::SkipUtf16BomBE(pUtf16), "Utf-16 Big Endian is currently not supported.");

    while (*pUtf16 != '\0')
    {
      // decode utf16 to utf32
      m_Data.PushBack(WUnicodeUtils::DecodeUtf16ToUtf32(pUtf16));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}


void WStringUtf32::operator=(const WUInt32* pUtf32)
{
  m_Data.Clear();

  if (pUtf32 != nullptr)
  {
    while (*pUtf32 != '\0')
    {
      m_Data.PushBack(*pUtf32);
      ++pUtf32;
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}

void WStringUtf32::operator=(const wchar_t* pWChar)
{
  m_Data.Clear();

  if (pWChar != nullptr)
  {
    while (*pWChar != '\0')
    {
      // decode wchar_t to utf32
      m_Data.PushBack(WUnicodeUtils::DecodeWCharToUtf32(pWChar));
    }
  }

  // append terminator
  m_Data.PushBack('\0');
}
