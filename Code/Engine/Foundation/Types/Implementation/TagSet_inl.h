#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>


// Template specialization to be able to use WTagSet properties as W_SET_MEMBER_PROPERTY.
template <typename T>
struct WContainerSubTypeResolver<WTagSetTemplate<T>>
{
  using Type = const char*;
};

// Template specialization to be able to use WTagSet properties as W_SET_MEMBER_PROPERTY.
template <typename Class>
class WMemberSetProperty<Class, WTagSet, const char*> : public WTypedSetProperty<typename WTypeTraits<const char*>::NonConstReferenceType>
{
public:
  using Container = WTagSet;
  using Type = WConstCharPtr;
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class*);
  using GetContainerFunc = Container& (*)(Class*);

  WMemberSetProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : WTypedSetProperty<RealType>(szPropertyName)
  {
    W_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an set property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      WAbstractSetProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    W_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveByName(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, WDynamicArray<WVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : m_ConstGetter(static_cast<const Class*>(pInstance)))
    {
      out_keys.PushBack(WVariant(value.GetTagString()));
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

// Template specialization to be able to use WTagSet properties as W_SET_ACCESSOR_PROPERTY.
template <typename Class>
class WAccessorSetProperty<Class, const char*, const WTagSet&> : public WTypedSetProperty<const char*>
{
public:
  using Container = const WTagSet&;
  using Type = WConstCharPtr;

  using ContainerType = typename WTypeTraits<Container>::NonConstReferenceType;
  using RealType = typename WTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc = void (Class::*)(Type value);
  using RemoveFunc = void (Class::*)(Type value);
  using GetValuesFunc = Container (Class::*)() const;

  WAccessorSetProperty(const char* szPropertyName, GetValuesFunc getValues, InsertFunc insert, RemoveFunc remove)
    : WTypedSetProperty<Type>(szPropertyName)
  {
    W_ASSERT_DEBUG(getValues != nullptr, "The get values function of an set property cannot be nullptr.");

    m_GetValues = getValues;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Insert == nullptr || m_Remove == nullptr)
      WAbstractSetProperty::m_Flags.Add(WPropertyFlags::ReadOnly);
  }


  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    W_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is read-only",
      WAbstractProperty::GetPropertyName());

    // We must not cache the container c here as the Remove can make it invalid
    // e.g. WArrayPtr by value.
    while (!IsEmpty(pInstance))
    {
      // this should be decltype(auto) c = ...; but MSVC 16 is too dumb for that (MSVC 15 works fine)
      decltype((static_cast<const Class*>(pInstance)->*m_GetValues)()) c = (static_cast<const Class*>(pInstance)->*m_GetValues)();
      auto it = cbegin(c);
      const WTag& value = *it;
      Remove(pInstance, value.GetTagString().GetData());
    }
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    W_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    W_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", WAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, WDynamicArray<WVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : (static_cast<const Class*>(pInstance)->*m_GetValues)())
    {
      out_keys.PushBack(WVariant(value.GetTagString()));
    }
  }

private:
  GetValuesFunc m_GetValues;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};


template <typename BlockStorageAllocator>
WTagSetTemplate<BlockStorageAllocator>::Iterator::Iterator(const WTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd)
  : m_pTagSet(pSet)

{
  if (!bEnd)
  {
    m_uiIndex = m_pTagSet->GetTagBlockStart() * (sizeof(WTagSetBlockStorage) * 8);

    if (m_pTagSet->IsEmpty())
      m_uiIndex = 0xFFFFFFFF;
    else
    {
      if (!IsBitSet())
        operator++();
    }
  }
  else
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::Iterator::IsBitSet() const
{
  WTag TempTag;
  TempTag.m_uiBlockIndex = m_uiIndex / (sizeof(WTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = m_uiIndex - (TempTag.m_uiBlockIndex * sizeof(WTagSetBlockStorage) * 8);

  return m_pTagSet->IsSet(TempTag);
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::Iterator::operator++()
{
  const WUInt32 uiMax = m_pTagSet->GetTagBlockEnd() * (sizeof(WTagSetBlockStorage) * 8);

  do
  {
    ++m_uiIndex;
  } while (m_uiIndex < uiMax && !IsBitSet());

  if (m_uiIndex >= uiMax)
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
const WTag& WTagSetTemplate<BlockStorageAllocator>::Iterator::operator*() const
{
  return *WTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
const WTag* WTagSetTemplate<BlockStorageAllocator>::Iterator::operator->() const
{
  return WTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
WTagSetTemplate<BlockStorageAllocator>::WTagSetTemplate()
{
  SetTagBlockStart(WSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::operator==(const WTagSetTemplate& other) const
{
  return m_TagBlocks == other.m_TagBlocks && m_TagBlocks.template GetUserData<WUInt32>() == other.m_TagBlocks.template GetUserData<WUInt32>();
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::operator!=(const WTagSetTemplate& other) const
{
  return !(*this == other);
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::Set(const WTag& tag)
{
  W_ASSERT_DEV(tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (m_TagBlocks.IsEmpty())
  {
    Reallocate(tag.m_uiBlockIndex, tag.m_uiBlockIndex);
  }
  else if (IsTagInAllocatedRange(tag) == false)
  {
    const WUInt32 uiNewBlockStart = WMath::Min<WUInt32>(tag.m_uiBlockIndex, GetTagBlockStart());
    const WUInt32 uiNewBlockEnd = WMath::Max<WUInt32>(tag.m_uiBlockIndex, GetTagBlockEnd());

    Reallocate(uiNewBlockStart, uiNewBlockEnd);
  }

  WUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

  const WUInt64 bitMask = W_BIT(tag.m_uiBitIndex);
  const bool bBitWasSet = ((tagBlock & bitMask) != 0);

  tagBlock |= bitMask;

  if (!bBitWasSet)
  {
    IncreaseTagCount();
  }
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::Remove(const WTag& tag)
{
  W_ASSERT_DEV(tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(tag))
  {
    WUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

    const WUInt64 bitMask = W_BIT(tag.m_uiBitIndex);
    const bool bBitWasSet = ((tagBlock & bitMask) != 0);

    tagBlock &= ~bitMask;

    if (bBitWasSet)
    {
      DecreaseTagCount();
    }
  }
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::IsSet(const WTag& tag) const
{
  W_ASSERT_DEV(tag.IsValid(), "Only valid tags can be checked!");

  if (IsTagInAllocatedRange(tag))
  {
    return (m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()] & W_BIT(tag.m_uiBitIndex)) != 0;
  }
  else
  {
    return false;
  }
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::IsAnySet(const WTagSetTemplate& otherSet) const
{
  // If any of the sets is empty nothing can match
  if (IsEmpty() || otherSet.IsEmpty())
    return false;

  // Calculate range to compare
  const WUInt32 uiMaxBlockStart = WMath::Max(GetTagBlockStart(), otherSet.GetTagBlockStart());
  const WUInt32 uiMinBlockEnd = WMath::Min(GetTagBlockEnd(), otherSet.GetTagBlockEnd());

  if (uiMaxBlockStart > uiMinBlockEnd)
    return false;

  for (WUInt32 i = uiMaxBlockStart; i < uiMinBlockEnd; ++i)
  {
    const WUInt32 uiThisBlockStorageIndex = i - GetTagBlockStart();
    const WUInt32 uiOtherBlockStorageIndex = i - otherSet.GetTagBlockStart();

    if ((m_TagBlocks[uiThisBlockStorageIndex] & otherSet.m_TagBlocks[uiOtherBlockStorageIndex]) != 0)
    {
      return true;
    }
  }

  return false;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WUInt32 WTagSetTemplate<BlockStorageAllocator>::GetNumTagsSet() const
{
  return GetTagCount();
}

template <typename BlockStorageAllocator>
W_ALWAYS_INLINE bool WTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  return GetTagCount() == 0;
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::Clear()
{
  m_TagBlocks.Clear();
  SetTagBlockStart(WSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::SetByName(WStringView sTag)
{
  const WTag& tag = WTagRegistry::GetGlobalRegistry().RegisterTag(sTag);
  Set(tag);
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::RemoveByName(WStringView sTag)
{
  if (const WTag* tag = WTagRegistry::GetGlobalRegistry().GetTagByName(WTempHashedString(sTag)))
  {
    Remove(*tag);
  }
}

template <typename BlockStorageAllocator>
bool WTagSetTemplate<BlockStorageAllocator>::IsSetByName(WStringView sTag) const
{
  if (const WTag* tag = WTagRegistry::GetGlobalRegistry().GetTagByName(WTempHashedString(sTag)))
  {
    return IsSet(*tag);
  }

  return false;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
bool WTagSetTemplate<BlockStorageAllocator>::IsSetByName(const WTempHashedString& sTag) const
{
  if (const WTag* tag = WTagRegistry::GetGlobalRegistry().GetTagByName(sTag))
  {
    return IsSet(*tag);
  }

  return false;
}

template <typename BlockStorageAllocator>
W_ALWAYS_INLINE bool WTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const WTag& Tag) const
{
  return Tag.m_uiBlockIndex >= GetTagBlockStart() && Tag.m_uiBlockIndex < GetTagBlockEnd();
}

template <typename BlockStorageAllocator>
void WTagSetTemplate<BlockStorageAllocator>::Reallocate(WUInt32 uiNewTagBlockStart, WUInt32 uiNewMaxBlockIndex)
{
  W_ASSERT_DEV(uiNewTagBlockStart < WSmallInvalidIndex, "Tag block start is too big");
  const WUInt16 uiNewBlockArraySize = static_cast<WUInt16>((uiNewMaxBlockIndex - uiNewTagBlockStart) + 1);

  // Early out for non-filled tag sets
  if (m_TagBlocks.IsEmpty())
  {
    m_TagBlocks.SetCount(uiNewBlockArraySize);
    SetTagBlockStart(static_cast<WUInt16>(uiNewTagBlockStart));

    return;
  }

  W_ASSERT_DEBUG(uiNewTagBlockStart <= GetTagBlockStart(), "New block start must be smaller or equal to current block start!");

  WSmallArray<WUInt64, 32, BlockStorageAllocator> helperArray;
  helperArray.SetCount(uiNewBlockArraySize);

  const WUInt32 uiOldBlockStartOffset = GetTagBlockStart() - uiNewTagBlockStart;

  // Copy old data to the new array
  WMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  // Use array ptr copy assignment so it doesn't modify the user data in m_TagBlocks
  m_TagBlocks = helperArray.GetArrayPtr();
  SetTagBlockStart(static_cast<WUInt16>(uiNewTagBlockStart));
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WUInt16 WTagSetTemplate<BlockStorageAllocator>::GetTagBlockStart() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WUInt16 WTagSetTemplate<BlockStorageAllocator>::GetTagBlockEnd() const
{
  return static_cast<WUInt16>(GetTagBlockStart() + m_TagBlocks.GetCount());
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WTagSetTemplate<BlockStorageAllocator>::SetTagBlockStart(WUInt16 uiTagBlockStart)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart = uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WUInt16 WTagSetTemplate<BlockStorageAllocator>::GetTagCount() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagCount;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WTagSetTemplate<BlockStorageAllocator>::SetTagCount(WUInt16 uiTagCount)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount = uiTagCount;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WTagSetTemplate<BlockStorageAllocator>::IncreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount++;
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WTagSetTemplate<BlockStorageAllocator>::DecreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount--;
}

static WTypeVersion s_TagSetVersion = 1;

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
void WTagSetTemplate<BlockStorageAllocator>::Save(WStreamWriter& inout_stream) const
{
  const WUInt16 uiNumTags = static_cast<WUInt16>(GetNumTagsSet());
  inout_stream << uiNumTags;

  inout_stream.WriteVersion(s_TagSetVersion);

  for (Iterator it = GetIterator(); it.IsValid(); ++it)
  {
    const WTag& tag = *it;

    inout_stream << tag.m_sTagString;
  }
}

template <typename BlockStorageAllocator /*= WDefaultAllocatorWrapper*/>
void WTagSetTemplate<BlockStorageAllocator>::Load(WStreamReader& inout_stream, WTagRegistry& inout_registry)
{
  WUInt16 uiNumTags = 0;
  inout_stream >> uiNumTags;

  // Manually read version value since 0 can be a valid version here
  WTypeVersion version;
  inout_stream.ReadWordValue(&version).IgnoreResult();

  if (version == 0)
  {
    for (WUInt32 i = 0; i < uiNumTags; ++i)
    {
      WUInt32 uiTagMurmurHash = 0;
      inout_stream >> uiTagMurmurHash;

      if (const WTag* pTag = inout_registry.GetTagByMurmurHash(uiTagMurmurHash))
      {
        Set(*pTag);
      }
    }
  }
  else
  {
    for (WUInt32 i = 0; i < uiNumTags; ++i)
    {
      WHashedString tagString;
      inout_stream >> tagString;

      const WTag& tag = inout_registry.RegisterTag(tagString);
      Set(tag);
    }
  }
}
