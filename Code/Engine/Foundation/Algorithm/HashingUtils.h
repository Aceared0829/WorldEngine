
#pragma once

#include <Foundation/Basics.h>

/// This class provides implementations of different hashing algorithms.
class W_FOUNDATION_DLL WHashingUtils
{
public:
  /// Calculates the CRC32 checksum of the given key.
  static WUInt32 CRC32Hash(const void* pKey, size_t uiSizeInBytes); // [tested]

  /// Calculates the 32bit murmur hash of the given key.
  static WUInt32 MurmurHash32(const void* pKey, size_t uiSizeInByte, WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit murmur hash of the given key.
  static WUInt64 MurmurHash64(const void* pKey, size_t uiSizeInByte, WUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit murmur hash of a string constant at compile time. Encoding does not matter here.
  template <size_t N>
  constexpr static WUInt32 MurmurHash32String(const char (&str)[N], WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 32bit murmur hash of a string pointer during runtime. Encoding does not matter here.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static WUInt32 MurmurHash32String(WStringView sStr, WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of the given key.
  static WUInt32 xxHash32(const void* pKey, size_t uiSizeInByte, WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of the given key.
  static WUInt64 xxHash64(const void* pKey, size_t uiSizeInByte, WUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of the given string literal at compile time.
  template <size_t N>
  constexpr static WUInt32 xxHash32String(const char (&str)[N], WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of the given string literal at compile time.
  template <size_t N>
  constexpr static WUInt64 xxHash64String(const char (&str)[N], WUInt64 uiSeed = 0); // [tested]

  /// Calculates the 32bit xxHash of a string pointer during runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static WUInt32 xxHash32String(WStringView sStr, WUInt32 uiSeed = 0); // [tested]

  /// Calculates the 64bit xxHash of a string pointer during runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static WUInt64 xxHash64String(WStringView sStr, WUInt64 uiSeed = 0); // [tested]

  /// Calculates the hash of the given string literal at compile time.
  template <size_t N>
  constexpr static WUInt64 StringHash(const char (&str)[N], WUInt64 uiSeed = 0); // [tested]

  /// Calculates the hash of a string pointer at runtime.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  static WUInt64 StringHash(WStringView sStr, WUInt64 uiSeed = 0); // [tested]

  /// Truncates a 64 bit string hash to 32 bit.
  ///
  /// This is necessary when a 64 bit string hash is used in a hash table (which only uses 32 bit indices).
  constexpr static WUInt32 StringHashTo32(WUInt64 uiHash);

  /// Combines two 32 bit hash values into one.
  constexpr static WUInt32 CombineHashValues32(WUInt32 ui0, WUInt32 ui1);
};

/// Helper struct to calculate the Hash of different types.
///
/// This struct can be used to provide a custom hash function for WHashTable. The default implementation uses the xxHash function.
template <typename T>
struct WHashHelper
{
  template <typename U>
  static WUInt32 Hash(const U& value);

  template <typename U>
  static bool Equal(const T& a, const U& b);
};

#include <Foundation/Algorithm/Implementation/HashingMurmur_inl.h>
#include <Foundation/Algorithm/Implementation/HashingUtils_inl.h>
#include <Foundation/Algorithm/Implementation/HashingXxHash_inl.h>
