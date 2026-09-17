#include <Foundation/Strings/Implementation/StringBase.h>

namespace WInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl
  {
    static WUInt32 Hash(const T& value);
  };

  template <typename T>
  struct HashHelperImpl<T, true>
  {
    W_ALWAYS_INLINE static WUInt32 Hash(WStringView sString)
    {
      return WHashingUtils::StringHashTo32(WHashingUtils::StringHash(sString));
    }
  };

  template <typename T, bool isString>
  W_ALWAYS_INLINE WUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    static_assert(isString, "WHashHelper is not implemented for the given type.");
    return 0;
  }
} // namespace WInternal

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUInt32 WHashHelper<T>::Hash(const U& value)
{
  return WInternal::HashHelperImpl<T, W_IS_DERIVED_FROM_STATIC(WThisIsAString, T)>::Hash(value);
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE bool WHashHelper<T>::Equal(const T& a, const U& b)
{
  return a == b;
}



template <>
struct WHashHelper<WUInt32>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WUInt32 value)
  {
    // Knuth: multiplication by the golden ratio will minimize gaps in the hash space.
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }

  W_ALWAYS_INLINE static bool Equal(WUInt32 a, WUInt32 b) { return a == b; }
};

template <>
struct WHashHelper<WInt32>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WInt32 value) { return WHashHelper<WUInt32>::Hash(WUInt32(value)); }

  W_ALWAYS_INLINE static bool Equal(WInt32 a, WInt32 b) { return a == b; }
};

template <>
struct WHashHelper<WUInt64>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WUInt64 value)
  {
    // boost::hash_combine.
    WUInt32 a = WUInt32(value >> 32);
    WUInt32 b = WUInt32(value);
    return a ^ (b + 0x9e3779b9 + (a << 6) + (b >> 2));
  }

  W_ALWAYS_INLINE static bool Equal(WUInt64 a, WUInt64 b) { return a == b; }
};

template <>
struct WHashHelper<WInt64>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WInt64 value) { return WHashHelper<WUInt64>::Hash(WUInt64(value)); }

  W_ALWAYS_INLINE static bool Equal(WInt64 a, WInt64 b) { return a == b; }
};

template <>
struct WHashHelper<const char*>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const char* szValue)
  {
    return WHashingUtils::StringHashTo32(WHashingUtils::StringHash(szValue));
  }

  W_ALWAYS_INLINE static bool Equal(const char* a, const char* b) { return WStringUtils::IsEqual(a, b); }
};

template <>
struct WHashHelper<WStringView>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WStringView sValue)
  {
    return WHashingUtils::StringHashTo32(WHashingUtils::StringHash(sValue));
  }

  W_ALWAYS_INLINE static bool Equal(WStringView a, WStringView b) { return a == b; }
};

template <typename T>
struct WHashHelper<T*>
{
  W_ALWAYS_INLINE static WUInt32 Hash(T* value)
  {
#if W_ENABLED(W_PLATFORM_64BIT)
    return WHashHelper<WUInt64>::Hash(reinterpret_cast<WUInt64>(value) >> 4);
#else
    return WHashHelper<WUInt32>::Hash(reinterpret_cast<WUInt32>(value) >> 4);
#endif
  }

  W_ALWAYS_INLINE static bool Equal(T* a, T* b)
  {
    return a == b;
  }
};

template <size_t N>
constexpr W_ALWAYS_INLINE WUInt64 WHashingUtils::StringHash(const char (&str)[N], WUInt64 uiSeed)
{
  return xxHash64String(str, uiSeed);
}

W_ALWAYS_INLINE WUInt64 WHashingUtils::StringHash(WStringView sStr, WUInt64 uiSeed)
{
  return xxHash64String(sStr, uiSeed);
}

constexpr W_ALWAYS_INLINE WUInt32 WHashingUtils::StringHashTo32(WUInt64 uiHash)
{
  // just throw away the upper bits
  return static_cast<WUInt32>(uiHash);
}

constexpr W_ALWAYS_INLINE WUInt32 WHashingUtils::CombineHashValues32(WUInt32 ui0, WUInt32 ui1)
{
  // See boost::hash_combine
  return ui0 ^ (ui1 + 0x9e3779b9 + (ui0 << 6) + (ui1 >> 2));
}
