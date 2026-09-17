#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

static WTagRegistry s_GlobalRegistry;

WTagRegistry::WTagRegistry() = default;

WTagRegistry& WTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

const WTag& WTagRegistry::RegisterTag(WStringView sTagString)
{
  WHashedString TagString;
  TagString.Assign(sTagString);

  return RegisterTag(TagString);
}

const WTag& WTagRegistry::RegisterTag(const WHashedString& sTagString)
{
  W_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  const WTag* pResult = GetTagByName(sTagString);

  if (pResult != nullptr)
    return *pResult;

  const WUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  WTag TempTag;
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(WTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(WTagSetBlockStorage) * 8);
  TempTag.m_sTagString = sTagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(sTagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  WLog::Debug("Registered Tag '{0}'", sTagString);
  return *m_TagsByIndex.PeekBack();
}

const WTag* WTagRegistry::GetTagByName(const WTempHashedString& sTagString) const
{
  W_LOCK(m_TagRegistryMutex);

  auto It = m_RegisteredTags.Find(sTagString);
  if (It.IsValid())
  {
    return &It.Value();
  }

  return nullptr;
}

const WTag* WTagRegistry::GetTagByMurmurHash(WUInt32 uiMurmurHash) const
{
  W_LOCK(m_TagRegistryMutex);

  for (WTag* pTag : m_TagsByIndex)
  {
    if (WHashingUtils::MurmurHash32String(pTag->GetTagString()) == uiMurmurHash)
    {
      return pTag;
    }
  }

  return nullptr;
}

const WTag* WTagRegistry::GetTagByIndex(WUInt32 uiIndex) const
{
  W_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex[uiIndex];
}

WUInt32 WTagRegistry::GetNumTags() const
{
  W_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex.GetCount();
}

WResult WTagRegistry::Load(WStreamReader& inout_stream)
{
  W_LOCK(m_TagRegistryMutex);

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion != 1)
  {
    WLog::Error("Invalid WTagRegistry version {0}", uiVersion);
    return W_FAILURE;
  }

  WUInt32 uiNumTags = 0;
  inout_stream >> uiNumTags;

  if (uiNumTags > 16 * 1024)
  {
    WLog::Error("WTagRegistry::Load, unreasonable amount of tags {0}, cancelling load.", uiNumTags);
    return W_FAILURE;
  }

  WStringBuilder temp;
  for (WUInt32 i = 0; i < uiNumTags; ++i)
  {
    inout_stream >> temp;

    RegisterTag(temp);
  }

  return W_SUCCESS;
}
