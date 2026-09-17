#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as WString & WStringBuilder instances)

WStreamWriter& operator<<(WStreamWriter& inout_stream, const char* szValue)
{
  WStringView szView(szValue);
  inout_stream.WriteString(szView).AssertSuccess();

  return inout_stream;
}

WStreamWriter& operator<<(WStreamWriter& inout_stream, WStringView sValue)
{
  inout_stream.WriteString(sValue).AssertSuccess();

  return inout_stream;
}

// WStringBuilder

WStreamWriter& operator<<(WStreamWriter& inout_stream, const WStringBuilder& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

WStreamReader& operator>>(WStreamReader& inout_stream, WStringBuilder& out_sValue)
{
  inout_stream.ReadString(out_sValue).AssertSuccess();
  return inout_stream;
}
