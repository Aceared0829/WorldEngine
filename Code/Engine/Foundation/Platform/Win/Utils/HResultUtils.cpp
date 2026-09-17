#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

W_FOUNDATION_DLL WString WHRESULTtoString(WMinWindows::HRESULT result)
{
  wchar_t buffer[4096];
  if (::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        result,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        buffer,
        W_ARRAY_SIZE(buffer),
        nullptr) == 0)
  {
    return {};
  }

  // Com error tends to put /r/n at the end. Remove it.
  WStringBuilder message(WStringUtf8(&buffer[0]).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}
#else

#  include <Foundation/Platform/Win/Utils/HResultUtils.h>

W_FOUNDATION_DLL WString WHRESULTtoString(WMinWindows::HRESULT result)
{
  return "NOT_SUPPORTED";
}
#endif
