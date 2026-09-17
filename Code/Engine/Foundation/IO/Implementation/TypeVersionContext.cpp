#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

static constexpr WTypeVersion s_uiTypeVersionContextVersion = 1;

W_IMPLEMENT_SERIALIZATION_CONTEXT(WTypeVersionWriteContext)

WTypeVersionWriteContext::WTypeVersionWriteContext() = default;
WTypeVersionWriteContext::~WTypeVersionWriteContext() = default;

WStreamWriter& WTypeVersionWriteContext::Begin(WStreamWriter& ref_originalStream)
{
  m_pOriginalStream = &ref_originalStream;

  W_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a type version context.");
  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

WResult WTypeVersionWriteContext::End()
{
  W_ASSERT_DEV(m_pOriginalStream != nullptr, "End() called before Begin()");

  WriteTypeVersions(*m_pOriginalStream);

  // Now append the original stream
  W_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(*m_pOriginalStream));

  return W_SUCCESS;
}

void WTypeVersionWriteContext::AddType(const WRTTI* pRtti)
{
  if (m_KnownTypes.Insert(pRtti) == false)
  {
    if (const WRTTI* pParentRtti = pRtti->GetParentType())
    {
      AddType(pParentRtti);
    }
  }
}

void WTypeVersionWriteContext::WriteTypeVersions(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiTypeVersionContextVersion);

  const WUInt32 uiNumTypes = m_KnownTypes.GetCount();
  inout_stream << uiNumTypes;

  WMap<WString, const WRTTI*> sortedTypes;
  for (auto pType : m_KnownTypes)
  {
    sortedTypes.Insert(pType->GetTypeName(), pType);
  }

  for (const auto& it : sortedTypes)
  {
    inout_stream << it.Key();
    inout_stream << it.Value()->GetTypeVersion();
  }
}

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_SERIALIZATION_CONTEXT(WTypeVersionReadContext)

WTypeVersionReadContext::WTypeVersionReadContext(WStreamReader& inout_stream)
{
  auto version = inout_stream.ReadVersion(s_uiTypeVersionContextVersion);
  W_IGNORE_UNUSED(version);

  WUInt32 uiNumTypes = 0;
  inout_stream >> uiNumTypes;

  WStringBuilder sTypeName;
  WUInt32 uiTypeVersion;

  for (WUInt32 i = 0; i < uiNumTypes; ++i)
  {
    inout_stream >> sTypeName;
    inout_stream >> uiTypeVersion;

    if (const WRTTI* pType = WRTTI::FindTypeByName(sTypeName))
    {
      m_TypeVersions.Insert(pType, uiTypeVersion);
    }
    else
    {
      WLog::Warning("Ignoring unknown type '{}'", sTypeName);
    }
  }
}

WTypeVersionReadContext::~WTypeVersionReadContext() = default;

WUInt32 WTypeVersionReadContext::GetTypeVersion(const WRTTI* pRtti) const
{
  WUInt32 uiVersion = WInvalidIndex;
  m_TypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}
