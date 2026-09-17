#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Variant.h>

class WStreamReader;
class WStreamWriter;

/// Flags for entries in WBlackboard.
struct W_CORE_DLL WBlackboardEntryFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    None = 0,
    Save = W_BIT(0),          ///< Include the entry during serialization
    OnChangeEvent = W_BIT(1), ///< Broadcast the 'ValueChanged' event when this entry's value is modified

    UserFlag0 = W_BIT(7),
    UserFlag1 = W_BIT(8),
    UserFlag2 = W_BIT(9),
    UserFlag3 = W_BIT(10),
    UserFlag4 = W_BIT(11),
    UserFlag5 = W_BIT(12),
    UserFlag6 = W_BIT(13),
    UserFlag7 = W_BIT(14),

    Invalid = W_BIT(15),

    Default = None
  };

  struct Bits
  {
    StorageType Save : 1;
    StorageType OnChangeEvent : 1;
    StorageType Reserved : 5;
    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
    StorageType Invalid : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WBlackboardEntryFlags);
W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WBlackboardEntryFlags);


/// A blackboard is a key/value store that provides OnChange events to be informed when a value changes.
///
/// Blackboards are used to gather typically small pieces of data. Some systems write the data, other systems read it.
/// Through the blackboard, arbitrary systems can interact.
///
/// For example this is commonly used in game AI, where some system gathers interesting pieces of data about the environment,
/// and then NPCs might use that information to make decisions.
class W_CORE_DLL WBlackboard : public WRefCounted
{
private:
  WBlackboard(bool bIsGlobal);

public:
  ~WBlackboard();

  bool IsGlobalBlackboard() const { return m_bIsGlobal; }

  /// Factory method to create a new blackboard.
  ///
  /// Since blackboards use shared ownership we need to make sure that blackboards are created in WCore.dll.
  /// Some compilers (MSVC) create local v-tables which can become stale if a blackboard was registered as global but the DLL
  /// which created the blackboard is already unloaded.
  ///
  /// See https://groups.google.com/g/microsoft.public.vc.language/c/atSh_2VSc2w/m/EgJ3r_7OzVUJ?pli=1
  static WSharedPtr<WBlackboard> Create(const WStringView& sName, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  /// Factory method to get access to a globally registered blackboard.
  ///
  /// If a blackboard with that name was already created globally before, its reference is returned.
  /// Otherwise it will be created and permanently registered under that name.
  /// Global blackboards cannot be removed. Although you can change their name via "SetName()",
  /// the name under which they are registered globally will not change.
  ///
  /// If at some point you want to "remove" a global blackboard, instead call UnregisterAllEntries() to
  /// clear all its values.
  static WSharedPtr<WBlackboard> GetOrCreateGlobal(const WHashedString& sBlackboardName, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());

  /// Finds a global blackboard with the given name.
  static WSharedPtr<WBlackboard> FindGlobal(const WTempHashedString& sBlackboardName);

  /// Changes the name of the blackboard.
  ///
  /// \note For global blackboards this has no effect under which name they are found. A global blackboard continues to
  /// be found by the name under which it was originally registered.
  void SetName(WStringView sName);
  const char* GetName() const { return m_sName; }
  const WHashedString& GetNameHashed() const { return m_sName; }

  struct Entry
  {
    WVariant m_Value;
    WBitflags<WBlackboardEntryFlags> m_Flags;
    WUInt8 m_uiEditorIndex = 0xFF;

    /// The change counter is increased every time the entry's value changes.
    /// Read this and compare it to a previous known value, to detect whether the value was changed since the last check.
    WUInt32 m_uiChangeCounter = 0;
  };

  struct EntryEvent
  {
    WHashedString m_sName;
    WVariant m_OldValue;
    const Entry* m_pEntry;
  };

  /// Removes the named entry. Does nothing, if no such entry exists.
  void RemoveEntry(const WHashedString& sName);

  ///  Removes all entries.
  void RemoveAllEntries();

  /// Returns whether an entry with the given name already exists.
  bool HasEntry(const WTempHashedString& sName) const;

  /// Sets the value of the named entry. If the entry doesn't exist, yet, it will be created with default flags.
  ///
  /// If the 'OnChangeEvent' flag is set for this entry, OnEntryEvent() will be broadcast.
  /// However, if the new value is no different to the old, no event will be broadcast.
  ///
  /// For new entries, no OnEntryEvent() is sent.
  ///
  /// For best efficiency, cache the entry name in an WHashedString and use the other overload of this function.
  /// DO NOT RECREATE the WHashedString every time, though.
  void SetEntryValue(WStringView sName, const WVariant& value);

  /// Overload of SetEntryValue() that takes an WHashedString rather than an WStringView.
  ///
  /// Using this function is more efficient, if you access the blackboard often, but you must ensure
  /// to only create the WHashedString once and cache it for reuse.
  /// Assigning a value to an WHashedString is an expensive operation, so if you do not cache the string,
  /// prefer to use the other overload.
  void SetEntryValue(const WHashedString& sName, const WVariant& value);

  /// Returns a pointer to the named entry, or nullptr if no such entry was registered.
  const Entry* GetEntry(const WTempHashedString& sName) const;

  /// Returns the flags of the named entry, or WBlackboardEntryFlags::Invalid, if no such entry was registered.
  WBitflags<WBlackboardEntryFlags> GetEntryFlags(const WTempHashedString& sName) const;

  /// Sets the flags of an existing entry. Returns W_FAILURE, if it wasn't created via SetEntryValue() or SetEntryValue() before.
  WResult SetEntryFlags(const WTempHashedString& sName, WBitflags<WBlackboardEntryFlags> flags);

  /// Returns the value of the named entry, or the fallback WVariant, if no such entry was registered.
  WVariant GetEntryValue(const WTempHashedString& sName, const WVariant& fallback = WVariant()) const;

  /// Convenience functions to directly get the value of an entry as a specific type.
  ///
  /// Returns the fallback value, if no such entry was registered or if the entry's value cannot be converted to the requested type.
  bool GetBoolValue(const WTempHashedString& sName, bool bFallback = false) const;
  int GetIntValue(const WTempHashedString& sName, int iFallback = 0) const;
  WUInt32 GetUIntValue(const WTempHashedString& sName, WUInt32 uiFallback = 0) const;
  float GetFloatValue(const WTempHashedString& sName, float fFallback = 0.0f) const;
  WString GetStringValue(const WTempHashedString& sName, WStringView sFallback = WStringView()) const;

  /// For the editor to know what index an element had, so that it can pass through exposed properties (which are given by index).
  WResult SetEditorIndex(const WTempHashedString& sName, WUInt8 uiEditorIndex);

  /// Searches for the first item that has the previously set index. Returns an empty string, if none was found.
  WHashedString FindNameForEditorIndex(WUInt8 uiEditorIndex) const;

  /// Increments the value of the named entry. Returns the incremented value or an invalid variant if the entry does not exist or is not a number type.
  WVariant IncrementEntryValue(const WTempHashedString& sName);

  /// Decrements the value of the named entry. Returns the decremented value or an invalid variant if the entry does not exist or is not a number type.
  WVariant DecrementEntryValue(const WTempHashedString& sName);

  /// Grants read access to the entire map of entries.
  const WHashTable<WHashedString, Entry>& GetAllEntries() const { return m_Entries; }

  /// Allows you to register to the OnEntryEvent. This is broadcast whenever an entry is modified that has the flag WBlackboardEntryFlags::OnChangeEvent.
  const WEvent<const EntryEvent&>& OnEntryEvent() const { return m_EntryEvents; }

  /// This counter is increased every time an entry is added or removed (but not when it is modified).
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether the set of entries has changed.
  WUInt32 GetBlackboardChangeCounter() const { return m_uiBlackboardChangeCounter; }

  /// This counter is increased every time any entry's value is modified.
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether any entry has changed recently.
  WUInt32 GetBlackboardEntryChangeCounter() const { return m_uiBlackboardEntryChangeCounter; }

  /// Stores all entries that have the 'Save' flag in the stream.
  WResult Serialize(WStreamWriter& inout_stream) const;

  /// Restores entries from the stream.
  ///
  /// If the blackboard already contains entries, the deserialized data is ADDED to the blackboard.
  /// If deserialized entries overlap with existing ones, the deserialized entries will overwrite the existing ones (both values and flags).
  WResult Deserialize(WStreamReader& inout_stream);

private:
  W_ALLOW_PRIVATE_PROPERTIES(WBlackboard);

  static WBlackboard* Reflection_GetOrCreateGlobal(const WHashedString& sName);
  static WBlackboard* Reflection_FindGlobal(WTempHashedString sName);
  void Reflection_SetEntryValue(WStringView sName, const WVariant& value);

  void ImplSetEntryValue(const WHashedString& sName, Entry& entry, const WVariant& value);

  template <typename T, typename U>
  T GetEntryValueAs(const WTempHashedString& sName, U fallback) const
  {
    const Entry* pEntry = GetEntry(sName);
    if (pEntry != nullptr && pEntry->m_Value.CanConvertTo<T>())
      return pEntry->m_Value.ConvertTo<T>();

    return fallback;
  }

  bool m_bIsGlobal = false;
  WHashedString m_sName;
  WEvent<const EntryEvent&> m_EntryEvents;
  WUInt32 m_uiBlackboardChangeCounter = 0;
  WUInt32 m_uiBlackboardEntryChangeCounter = 0;
  WHashTable<WHashedString, Entry> m_Entries;

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, Blackboard);
  static WMutex s_GlobalBlackboardsMutex;
  static WHashTable<WHashedString, WSharedPtr<WBlackboard>> s_GlobalBlackboards;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WBlackboard);

//////////////////////////////////////////////////////////////////////////

struct W_CORE_DLL WBlackboardCondition
{
  WHashedString m_sEntryName;
  double m_fComparisonValue = 0.0;
  WEnum<WComparisonOperator> m_Operator;

  bool IsConditionMet(const WBlackboard& blackboard) const;

  bool operator==(const WBlackboardCondition& rhs) const
  {
    return m_sEntryName == rhs.m_sEntryName && m_fComparisonValue == rhs.m_fComparisonValue && m_Operator == rhs.m_Operator;
  }
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WBlackboardCondition);
W_DECLARE_CUSTOM_VARIANT_TYPE(WBlackboardCondition);

W_CORE_DLL void operator<<(WStreamWriter& inout_stream, const WBlackboardCondition& cond);
W_CORE_DLL void operator>>(WStreamReader& inout_stream, WBlackboardCondition& ref_cond);

template <>
struct WHashHelper<WBlackboardCondition>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WBlackboardCondition& cond)
  {
    WUInt32 uiHash = WHashHelper<WUInt64>::Hash(cond.m_sEntryName.GetHash());
    uiHash = WHashingUtils::xxHash32(&cond.m_fComparisonValue, sizeof(double), uiHash);
    const WComparisonOperator::StorageType uiOperator = cond.m_Operator.GetValue();
    uiHash = WHashingUtils::xxHash32(&uiOperator, sizeof(uiOperator), uiHash);

    return uiHash;
  }

  W_ALWAYS_INLINE static bool Equal(const WBlackboardCondition& a, const WBlackboardCondition& b) { return a == b; }
};
