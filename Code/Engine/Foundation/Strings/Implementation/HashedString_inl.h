#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

inline WHashedString::WHashedString(const WHashedString& rhs)
{
  m_Data = rhs.m_Data;

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  // the string has a refcount of at least one (rhs holds a reference), thus it will definitely not get deleted on some other thread
  // therefore we can simply increase the refcount without locking
  m_Data.Value().m_iRefCount.Increment();
#endif
}

W_FORCE_INLINE WHashedString::WHashedString(WHashedString&& rhs)
{
  m_Data = rhs.m_Data;
  rhs.m_Data = HashedType(); // This leaves the string in an invalid state, all operations will fail except the destructor
}

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
inline WHashedString::~WHashedString()
{
  // Explicit check if data is still valid. It can be invalid if this string has been moved.
  if (m_Data.IsValid())
  {
    // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
    m_Data.Value().m_iRefCount.Decrement();
  }
}
#endif

inline void WHashedString::operator=(const WHashedString& rhs)
{
  // first increase the other refcount, then decrease ours
  HashedType tmp = rhs.m_Data;

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Increment();

  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data = tmp;
}

W_FORCE_INLINE void WHashedString::operator=(WHashedString&& rhs)
{
#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data = rhs.m_Data;
  rhs.m_Data = HashedType();
}

template <size_t N>
W_FORCE_INLINE void WHashedString::Assign(const char (&string)[N])
{
#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(string, WHashingUtils::StringHash(string));

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

W_FORCE_INLINE void WHashedString::Assign(WStringView sString)
{
#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(sString, WHashingUtils::StringHash(sString));

#if W_ENABLED(W_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

inline bool WHashedString::operator==(const WHashedString& rhs) const
{
  return m_Data == rhs.m_Data;
}

inline bool WHashedString::operator==(const WTempHashedString& rhs) const
{
  return m_Data.Key() == rhs.m_uiHash;
}

inline bool WHashedString::operator!=(const WTempHashedString& rhs) const
{
  return m_Data.Key() != rhs.m_uiHash;
}

inline bool WHashedString::operator<(const WHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_Data.Key();
}

inline bool WHashedString::operator<(const WTempHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_uiHash;
}

W_ALWAYS_INLINE const WString& WHashedString::GetString() const
{
  return m_Data.Value().m_sString;
}

W_ALWAYS_INLINE const char* WHashedString::GetData() const
{
  return m_Data.Value().m_sString.GetData();
}

W_ALWAYS_INLINE WUInt64 WHashedString::GetHash() const
{
  return m_Data.Key();
}

template <size_t N>
W_FORCE_INLINE WHashedString WMakeHashedString(const char (&string)[N])
{
  WHashedString sResult;
  sResult.Assign(string);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WTempHashedString::WTempHashedString()
{
  constexpr WUInt64 uiEmptyHash = WHashingUtils::StringHash("");
  m_uiHash = uiEmptyHash;
}

template <size_t N>
constexpr W_ALWAYS_INLINE WTempHashedString::WTempHashedString(const char (&string)[N])
  : m_uiHash(WHashingUtils::StringHash<N>(string))
{
}

W_ALWAYS_INLINE WTempHashedString::WTempHashedString(WStringView sString)
{
  m_uiHash = WHashingUtils::StringHash(sString);
}

W_ALWAYS_INLINE WTempHashedString::WTempHashedString(const WTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

W_ALWAYS_INLINE WTempHashedString::WTempHashedString(const WHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

W_ALWAYS_INLINE WTempHashedString::WTempHashedString(WUInt64 uiHash)
{
  m_uiHash = uiHash;
}

template <size_t N>
W_ALWAYS_INLINE void WTempHashedString::operator=(const char (&string)[N])
{
  m_uiHash = WHashingUtils::StringHash<N>(string);
}

W_ALWAYS_INLINE void WTempHashedString::operator=(WStringView sString)
{
  m_uiHash = WHashingUtils::StringHash(sString);
}

W_ALWAYS_INLINE void WTempHashedString::operator=(const WTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

W_ALWAYS_INLINE void WTempHashedString::operator=(const WHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

W_ALWAYS_INLINE bool WTempHashedString::operator==(const WTempHashedString& rhs) const
{
  return m_uiHash == rhs.m_uiHash;
}

W_ALWAYS_INLINE bool WTempHashedString::operator<(const WTempHashedString& rhs) const
{
  return m_uiHash < rhs.m_uiHash;
}

W_ALWAYS_INLINE bool WTempHashedString::IsEmpty() const
{
  constexpr WUInt64 uiEmptyHash = WHashingUtils::StringHash("");
  return m_uiHash == uiEmptyHash;
}

W_ALWAYS_INLINE void WTempHashedString::Clear()
{
  *this = WTempHashedString();
}

W_ALWAYS_INLINE WUInt64 WTempHashedString::GetHash() const
{
  return m_uiHash;
}

//////////////////////////////////////////////////////////////////////////

template <>
struct WHashHelper<WHashedString>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WHashedString& value)
  {
    return WHashingUtils::StringHashTo32(value.GetHash());
  }

  W_ALWAYS_INLINE static WUInt32 Hash(const WTempHashedString& value)
  {
    return WHashingUtils::StringHashTo32(value.GetHash());
  }

  W_ALWAYS_INLINE static bool Equal(const WHashedString& a, const WHashedString& b) { return a == b; }

  W_ALWAYS_INLINE static bool Equal(const WHashedString& a, const WTempHashedString& b) { return a == b; }
};

template <>
struct WHashHelper<WTempHashedString>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const WTempHashedString& value)
  {
    return WHashingUtils::StringHashTo32(value.GetHash());
  }

  W_ALWAYS_INLINE static bool Equal(const WTempHashedString& a, const WTempHashedString& b) { return a == b; }
};
