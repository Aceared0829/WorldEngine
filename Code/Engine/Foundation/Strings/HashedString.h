#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>

class WTempHashedString;

/// This class is optimized to take nearly no memory (sizeof(void*)) and to allow very fast checks whether two strings are identical.
///
/// Internally only a reference to the string data is stored. The data itself is stored in a central location, where no duplicates are
/// possible. Thus two identical strings will result in identical WHashedString objects, which makes equality comparisons very easy
/// (it's a pointer comparison).\n
/// Copying WHashedString objects around and assigning between them is very fast as well.\n
/// \n
/// Assigning from some other string type is rather slow though, as it requires thread synchronization.\n
/// You can also get access to the actual string data via GetString().\n
/// \n
/// You should use WHashedString whenever the size of the encapsulating object is important and when changes to the string itself
/// are rare, but checks for equality might be frequent (e.g. in a system where objects are identified via their name).\n
/// At runtime when you need to compare WHashedString objects with some temporary string object, used WTempHashedString,
/// as it will only use the string's hash value for comparison, but will not store the actual string anywhere.
class W_FOUNDATION_DLL WHashedString
{
public:
  struct HashedData
  {
#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
    WAtomicInteger32 m_iRefCount;
#endif
    WString m_sString;
  };

  // Do NOT use a hash-table! The map does not relocate memory when it resizes, which is a vital aspect for the hashed strings to work.
  using StringStorage = WMap<WUInt64, HashedData, WCompareHelper<WUInt64>, WStaticsAllocatorWrapper>;
  using HashedType = StringStorage::Iterator;

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  /// This will remove all hashed strings from the central storage, that are not referenced anymore.
  ///
  /// All hashed string values are stored in a central location and WHashedString just references them. Those strings are then
  /// reference counted. Once some string is not referenced anymore, its ref count reaches zero, but it will not be removed from
  /// the storage, as it might be reused later again.
  /// This function will clean up all unused strings. It should typically not be necessary to call this function at all, unless lots of
  /// strings get stored in WHashedString that are not really used throughout the applications life time.
  ///
  /// Returns the number of unused strings that were removed.
  static WUInt32 ClearUnusedStrings();
#endif

  W_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Initializes this string to the empty string.
  WHashedString(); // [tested]

  /// Copies the given WHashedString.
  WHashedString(const WHashedString& rhs); // [tested]

  /// Moves the given WHashedString.
  WHashedString(WHashedString&& rhs); // [tested]

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  /// Releases the reference to the internal data. Does NOT deallocate any data, even if this held the last reference to some string.
  ~WHashedString();
#endif

  /// Copies the given WHashedString.
  void operator=(const WHashedString& rhs); // [tested]

  /// Moves the given WHashedString.
  void operator=(WHashedString&& rhs); // [tested]

  /// Assigning a new string from a string constant is a slow operation, but the hash computation can happen at compile time.
  ///
  /// If you need to create an object to compare WHashedString objects against, prefer to use WTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  template <size_t N>
  void Assign(const char (&string)[N]); // [tested]

  template <size_t N>
  void Assign(char (&string)[N]) = delete;

  /// Assigning a new string from a non-hashed string is a very slow operation, this should be used rarely.
  ///
  /// If you need to create an object to compare WHashedString objects against, prefer to use WTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  void Assign(WStringView sString); // [tested]

  /// Comparing whether two hashed strings are identical is just a pointer comparison. This operation is what WHashedString is
  /// optimized for.
  ///
  /// \note Comparing between WHashedString objects is always error-free, so even if two string had the same hash value, although they are
  /// different, this comparison function will not report they are the same.
  bool operator==(const WHashedString& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WHashedString&);

  /// Compares this string object to an WTempHashedString object. This should be used whenever some object needs to be found
  /// and the string to compare against is not yet an WHashedString object.
  bool operator==(const WTempHashedString& rhs) const; // [tested]
  bool operator!=(const WTempHashedString& rhs) const; // [tested]

  /// This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const WHashedString& rhs) const; // [tested]

  /// This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const WTempHashedString& rhs) const; // [tested]

  /// Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const WString& GetString() const; // [tested]

  /// Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const char* GetData() const;

  /// Returns the hash of the stored string.
  WUInt64 GetHash() const; // [tested]

  /// Returns whether the string is empty.
  bool IsEmpty() const;

  /// Resets the string to the empty string.
  void Clear();

  /// Returns a string view to this string's data.
  W_ALWAYS_INLINE operator WStringView() const { return GetString().GetView(); }

  /// Returns a string view to this string's data.
  W_ALWAYS_INLINE WStringView GetView() const { return GetString().GetView(); }

  /// Returns a pointer to the internal Utf8 string.
  W_ALWAYS_INLINE operator const char*() const { return GetData(); }

  // since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison
  W_ALWAYS_INLINE bool operator==(const char* szString) const { return GetString().GetView() == WStringView(szString); }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const char*);

  /// Attempts to find a known string for the given hash value.
  ///
  /// Careful, this is a slow operation (involving a mutex). It is only meant for debug output purposes.
  /// The string hash may not be known, if the value was never assigned to any WHashedString, in which case W_FAILURE is returned.
  static WResult LookupStringHash(WUInt64 uiHash, WStringView& out_sResult);

private:
  static void InitHashedString();
  static HashedType AddHashedString(WStringView sString, WUInt64 uiHash);

  HashedType m_Data;
};

// since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison
W_ALWAYS_INLINE bool operator==(const char* szString, const WHashedString& rhs)
{
  return rhs.GetView() == WStringView(szString);
}

#if W_DISABLED(W_USE_CPP20_OPERATORS)
W_ALWAYS_INLINE bool operator!=(const char* szString, const WHashedString& rhs)
{
  return rhs.GetView() != WStringView(szString);
}

#endif

/// Helper function to create an WHashedString. This can be used to initialize static hashed string variables.
template <size_t N>
WHashedString WMakeHashedString(const char (&string)[N]);


/// A class to use together with WHashedString for quick comparisons with temporary strings that need not be stored further.
///
/// Whenever you have objects that use WHashedString members and you need to compare against them with some temporary string,
/// prefer to use WTempHashedString instead of WHashedString, as the latter requires thread synchronization to actually set up the
/// object.
class W_FOUNDATION_DLL WTempHashedString
{
  friend class WHashedString;

public:
  WTempHashedString(); // [tested]

  /// Creates an WTempHashedString object from the given string constant. The hash can be computed at compile time.
  template <size_t N>
  constexpr WTempHashedString(const char (&string)[N]); // [tested]

  template <size_t N>
  WTempHashedString(char (&string)[N]) = delete;

  /// Creates an WTempHashedString object from the given string. Computes the hash of the given string during runtime, which might
  /// be slow.
  explicit WTempHashedString(WStringView sString); // [tested]

  /// Copies the hash from rhs.
  WTempHashedString(const WTempHashedString& rhs); // [tested]

  /// Copies the hash from the WHashedString.
  WTempHashedString(const WHashedString& rhs); // [tested]

  explicit WTempHashedString(WUInt32 uiHash) = delete;

  /// Copies the hash from the 64 bit integer.
  explicit WTempHashedString(WUInt64 uiHash);

  /// The hash of the given string can be computed at compile time.
  template <size_t N>
  void operator=(const char (&string)[N]); // [tested]

  /// Computes and stores the hash of the given string during runtime, which might be slow.
  void operator=(WStringView sString); // [tested]

  /// Copies the hash from rhs.
  void operator=(const WTempHashedString& rhs); // [tested]

  /// Copies the hash from the WHashedString.
  void operator=(const WHashedString& rhs); // [tested]

  /// Compares the two objects by their hash value. Might report incorrect equality, if two strings have the same hash value.
  bool operator==(const WTempHashedString& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WTempHashedString&);

  /// This operator allows soring objects by hash value, not by alphabetical order.
  bool operator<(const WTempHashedString& rhs) const; // [tested]

  /// Checks whether the WTempHashedString represents the empty string.
  bool IsEmpty() const; // [tested]

  /// Resets the string to the empty string.
  void Clear(); // [tested]

  /// Returns the hash of the stored string.
  WUInt64 GetHash() const; // [tested]

  /// Convenience function to call WHashedString::LookupStringHash().
  WResult LookupStringHash(WStringView& out_sResult) const
  {
    return WHashedString::LookupStringHash(m_uiHash, out_sResult);
  }

private:
  WUInt64 m_uiHash;
};

// For WFormatString
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WHashedString& sArg);

#include <Foundation/Strings/Implementation/HashedString_inl.h>
