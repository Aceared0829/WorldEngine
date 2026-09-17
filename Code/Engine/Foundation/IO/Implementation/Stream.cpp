#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

WStreamReader::WStreamReader() = default;
WStreamReader::~WStreamReader() = default;

WResult WStreamReader::ReadString(WStringBuilder& ref_sBuilder)
{
  if (auto context = WStringDeduplicationReadContext::GetContext())
  {
    ref_sBuilder = context->DeserializeString(*this);
  }
  else
  {
    WUInt32 uiCount = 0;
    W_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      ref_sBuilder.m_Data.Reserve(uiCount + 1);
      ref_sBuilder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(ref_sBuilder.m_Data.GetData(), uiCount);
      ref_sBuilder.AppendTerminator();
    }
    else
    {
      ref_sBuilder.Clear();
    }
  }

  return W_SUCCESS;
}

WResult WStreamReader::ReadString(WString& ref_sString)
{
  WStringBuilder tmp;
  const WResult res = ReadString(tmp);
  ref_sString = tmp;

  return res;
}

WStreamWriter::WStreamWriter() = default;
WStreamWriter::~WStreamWriter() = default;

WResult WStreamWriter::WriteString(const WStringView sStringView)
{
  const WUInt32 uiCount = sStringView.GetElementCount();

  if (auto context = WStringDeduplicationWriteContext::GetContext())
  {
    context->SerializeString(sStringView, *this);
  }
  else
  {
    W_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      W_SUCCEED_OR_RETURN(WriteBytes(sStringView.GetStartPointer(), uiCount));
    }
  }

  return W_SUCCESS;
}
