#include <Core/CorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_BITFLAGS(WBlackboardEntryFlags, 1)
  W_BITFLAGS_CONSTANTS(WBlackboardEntryFlags::Save, WBlackboardEntryFlags::OnChangeEvent,
    WBlackboardEntryFlags::UserFlag0, WBlackboardEntryFlags::UserFlag1, WBlackboardEntryFlags::UserFlag2, WBlackboardEntryFlags::UserFlag3, WBlackboardEntryFlags::UserFlag4, WBlackboardEntryFlags::UserFlag5, WBlackboardEntryFlags::UserFlag6, WBlackboardEntryFlags::UserFlag7)
W_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WBlackboard, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOrCreateGlobal, In, "Name")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_FindGlobal, In, "Name")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardNamesEnum"))),

    W_SCRIPT_FUNCTION_PROPERTY(GetName),
    W_SCRIPT_FUNCTION_PROPERTY(Reflection_SetEntryValue, In, "Name", In, "Value")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),

    W_SCRIPT_FUNCTION_PROPERTY(GetBoolValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetIntValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetUIntValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetStringValue, In, "Name", In, "Fallback")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),

    W_SCRIPT_FUNCTION_PROPERTY(IncrementEntryValue, In, "Name")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(DecrementEntryValue, In, "Name")->AddAttributes(new WFunctionArgumentAttributes(0, new WDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    W_SCRIPT_FUNCTION_PROPERTY(GetBlackboardChangeCounter),
    W_SCRIPT_FUNCTION_PROPERTY(GetBlackboardEntryChangeCounter)
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_SUBSYSTEM_DECLARATION(Core, Blackboard)

  ON_CORESYSTEMS_SHUTDOWN
  {
    W_LOCK(WBlackboard::s_GlobalBlackboardsMutex);
    WBlackboard::s_GlobalBlackboards.Clear();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
WMutex WBlackboard::s_GlobalBlackboardsMutex;
WHashTable<WHashedString, WSharedPtr<WBlackboard>> WBlackboard::s_GlobalBlackboards;

// static
WSharedPtr<WBlackboard> WBlackboard::Create(const WStringView& sName, WAllocator* pAllocator /*= WFoundation::GetDefaultAllocator()*/)
{
  WSharedPtr<WBlackboard> pBlackboard = W_NEW(pAllocator, WBlackboard, false);
  pBlackboard->m_sName.Assign(sName);
  return pBlackboard;
}

// static
WSharedPtr<WBlackboard> WBlackboard::GetOrCreateGlobal(const WHashedString& sBlackboardName, WAllocator* pAllocator /*= WFoundation::GetDefaultAllocator()*/)
{
  W_LOCK(s_GlobalBlackboardsMutex);

  auto it = s_GlobalBlackboards.Find(sBlackboardName);

  if (it.IsValid())
  {
    return it.Value();
  }

  WSharedPtr<WBlackboard> pShrd = W_NEW(pAllocator, WBlackboard, true);
  pShrd->m_sName = sBlackboardName;
  s_GlobalBlackboards.Insert(sBlackboardName, pShrd);

  return pShrd;
}

// static
WSharedPtr<WBlackboard> WBlackboard::FindGlobal(const WTempHashedString& sBlackboardName)
{
  W_LOCK(s_GlobalBlackboardsMutex);

  WSharedPtr<WBlackboard> pBlackboard;
  s_GlobalBlackboards.TryGetValue(sBlackboardName, pBlackboard);
  return pBlackboard;
}

WBlackboard::WBlackboard(bool bIsGlobal)
{
  m_bIsGlobal = bIsGlobal;
}

WBlackboard::~WBlackboard() = default;

void WBlackboard::SetName(WStringView sName)
{
  W_LOCK(s_GlobalBlackboardsMutex);
  m_sName.Assign(sName);
}

void WBlackboard::RemoveEntry(const WHashedString& sName)
{
  if (m_Entries.Remove(sName))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void WBlackboard::RemoveAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

void WBlackboard::ImplSetEntryValue(const WHashedString& sName, Entry& entry, const WVariant& value)
{
  if (entry.m_Value != value)
  {
    ++m_uiBlackboardEntryChangeCounter;
    ++entry.m_uiChangeCounter;

    if (entry.m_Flags.IsSet(WBlackboardEntryFlags::OnChangeEvent))
    {
      EntryEvent e;
      e.m_sName = sName;
      e.m_OldValue = entry.m_Value;
      e.m_pEntry = &entry;

      entry.m_Value = value;

      m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
    }
    else
    {
      entry.m_Value = value;
    }
  }
}

void WBlackboard::SetEntryValue(WStringView sName, const WVariant& value)
{
  const WTempHashedString sNameTH(sName);

  auto itEntry = m_Entries.Find(sNameTH);

  if (!itEntry.IsValid())
  {
    WHashedString sNameHS;
    sNameHS.Assign(sName);
    m_Entries[sNameHS].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void WBlackboard::SetEntryValue(const WHashedString& sName, const WVariant& value)
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    m_Entries[sName].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void WBlackboard::Reflection_SetEntryValue(WStringView sName, const WVariant& value)
{
  SetEntryValue(sName, value);
}

bool WBlackboard::HasEntry(const WTempHashedString& sName) const
{
  return m_Entries.Find(sName).IsValid();
}

WResult WBlackboard::SetEntryFlags(const WTempHashedString& sName, WBitflags<WBlackboardEntryFlags> flags)
{
  auto itEntry = m_Entries.Find(sName);
  if (!itEntry.IsValid())
    return W_FAILURE;

  itEntry.Value().m_Flags = flags;
  return W_SUCCESS;
}

const WBlackboard::Entry* WBlackboard::GetEntry(const WTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
    return nullptr;

  return &itEntry.Value();
}

WVariant WBlackboard::GetEntryValue(const WTempHashedString& sName, const WVariant& fallback /*= WVariant()*/) const
{
  auto pEntry = m_Entries.GetValue(sName);
  return pEntry != nullptr ? pEntry->m_Value : fallback;
}

bool WBlackboard::GetBoolValue(const WTempHashedString& sName, bool bFallback) const
{
  return GetEntryValueAs<bool>(sName, bFallback);
}

int WBlackboard::GetIntValue(const WTempHashedString& sName, int iFallback) const
{
  return GetEntryValueAs<int>(sName, iFallback);
}

WUInt32 WBlackboard::GetUIntValue(const WTempHashedString& sName, WUInt32 uiFallback) const
{
  return GetEntryValueAs<WUInt32>(sName, uiFallback);
}

float WBlackboard::GetFloatValue(const WTempHashedString& sName, float fFallback) const
{
  return GetEntryValueAs<float>(sName, fFallback);
}

WString WBlackboard::GetStringValue(const WTempHashedString& sName, WStringView sFallback) const
{
  return GetEntryValueAs<WString>(sName, sFallback);
}

WResult WBlackboard::SetEditorIndex(const WTempHashedString& sName, WUInt8 uiEditorIndex)
{
  auto itEntry = m_Entries.Find(sName);
  if (!itEntry.IsValid())
    return W_FAILURE;

  itEntry.Value().m_uiEditorIndex = uiEditorIndex;
  return W_SUCCESS;
}

WHashedString WBlackboard::FindNameForEditorIndex(WUInt8 uiEditorIndex) const
{
  for (auto& e : m_Entries)
  {
    if (e.Value().m_uiEditorIndex == uiEditorIndex)
      return e.Key();
  }

  return {};
}

WVariant WBlackboard::IncrementEntryValue(const WTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    WVariant one = WVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value + one;
    return pEntry->m_Value;
  }

  return WVariant();
}

WVariant WBlackboard::DecrementEntryValue(const WTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    WVariant one = WVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value - one;
    return pEntry->m_Value;
  }

  return WVariant();
}

WBitflags<WBlackboardEntryFlags> WBlackboard::GetEntryFlags(const WTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    return WBlackboardEntryFlags::Invalid;
  }

  return itEntry.Value().m_Flags;
}

WResult WBlackboard::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  WUInt32 uiEntries = 0;

  for (auto it : m_Entries)
  {
    if (it.Value().m_Flags.IsSet(WBlackboardEntryFlags::Save))
    {
      ++uiEntries;
    }
  }

  inout_stream << uiEntries;

  for (auto it : m_Entries)
  {
    const Entry& e = it.Value();

    if (e.m_Flags.IsSet(WBlackboardEntryFlags::Save))
    {
      inout_stream << it.Key();
      inout_stream << e.m_Flags;
      inout_stream << e.m_Value;
    }
  }

  return W_SUCCESS;
}

WResult WBlackboard::Deserialize(WStreamReader& inout_stream)
{
  inout_stream.ReadVersion(1);

  WUInt32 uiEntries = 0;
  inout_stream >> uiEntries;

  for (WUInt32 e = 0; e < uiEntries; ++e)
  {
    WHashedString name;
    inout_stream >> name;

    WBitflags<WBlackboardEntryFlags> flags;
    inout_stream >> flags;

    WVariant value;
    inout_stream >> value;

    SetEntryValue(name, value);
    SetEntryFlags(name, flags).AssertSuccess();
  }

  return W_SUCCESS;
}

// static
WBlackboard* WBlackboard::Reflection_GetOrCreateGlobal(const WHashedString& sName)
{
  return GetOrCreateGlobal(sName).Borrow();
}

// static
WBlackboard* WBlackboard::Reflection_FindGlobal(WTempHashedString sName)
{
  return FindGlobal(sName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WBlackboardCondition, WNoBase, 1, WRTTIDefaultAllocator<WBlackboardCondition>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EntryName", m_sEntryName)->AddAttributes(new WDynamicStringEnumAttribute("BlackboardKeysEnum")),
    W_ENUM_MEMBER_PROPERTY("Operator", WComparisonOperator, m_Operator),
    W_MEMBER_PROPERTY("ComparisonValue", m_fComparisonValue),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_DEFINE_CUSTOM_VARIANT_TYPE(WBlackboardCondition);
// clang-format on

bool WBlackboardCondition::IsConditionMet(const WBlackboard& blackboard) const
{
  auto pEntry = blackboard.GetEntry(m_sEntryName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    double fEntryValue = pEntry->m_Value.ConvertTo<double>();
    return WComparisonOperator::Compare(m_Operator, fEntryValue, m_fComparisonValue);
  }

  return false;
}

constexpr WTypeVersion s_BlackboardConditionVersion = 1;

void operator<<(WStreamWriter& inout_stream, const WBlackboardCondition& cond)
{
  inout_stream.WriteVersion(s_BlackboardConditionVersion);

  inout_stream << cond.m_sEntryName;
  inout_stream << cond.m_Operator;
  inout_stream << cond.m_fComparisonValue;
}

void operator>>(WStreamReader& inout_stream, WBlackboardCondition& ref_cond)
{
  const WTypeVersion uiVersion = inout_stream.ReadVersion(s_BlackboardConditionVersion);
  W_IGNORE_UNUSED(uiVersion);

  inout_stream >> ref_cond.m_sEntryName;
  inout_stream >> ref_cond.m_Operator;
  inout_stream >> ref_cond.m_fComparisonValue;
}

W_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
