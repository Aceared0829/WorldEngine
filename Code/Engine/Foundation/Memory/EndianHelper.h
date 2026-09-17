#pragma once

#include <Foundation/Basics.h>

/// Collection of helper methods when working with endianess "problems"
struct W_FOUNDATION_DLL WEndianHelper
{

  /// Returns true if called on a big endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines W_PLATFORM_LITTLE_ENDIAN, W_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsBigEndian()
  {
    const int i = 1;
    return (*(char*)&i) == 0;
  }

  /// Returns true if called on a little endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines W_PLATFORM_LITTLE_ENDIAN, W_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsLittleEndian() { return !IsBigEndian(); }

  /// Switches endianess of the given array of words (16 bit values).
  static inline void SwitchWords(WUInt16* pWords, WUInt32 uiCount) // [tested]
  {
    for (WUInt32 i = 0; i < uiCount; i++)
      pWords[i] = Switch(pWords[i]);
  }

  /// Switches endianess of the given array of double words (32 bit values).
  static inline void SwitchDWords(WUInt32* pDWords, WUInt32 uiCount) // [tested]
  {
    for (WUInt32 i = 0; i < uiCount; i++)
      pDWords[i] = Switch(pDWords[i]);
  }

  /// Switches endianess of the given array of quad words (64 bit values).
  static inline void SwitchQWords(WUInt64* pQWords, WUInt32 uiCount) // [tested]
  {
    for (WUInt32 i = 0; i < uiCount; i++)
      pQWords[i] = Switch(pQWords[i]);
  }

  /// Returns a single switched word (16 bit value).
  static W_ALWAYS_INLINE WUInt16 Switch(WUInt16 uiWord) // [tested]
  {
    return (((uiWord & 0xFF) << 8) | ((uiWord >> 8) & 0xFF));
  }

  /// Returns a single switched double word (32 bit value).
  static W_ALWAYS_INLINE WUInt32 Switch(WUInt32 uiDWord) // [tested]
  {
    return (((uiDWord & 0xFF) << 24) | (((uiDWord >> 8) & 0xFF) << 16) | (((uiDWord >> 16) & 0xFF) << 8) | ((uiDWord >> 24) & 0xFF));
  }

  /// Returns a single switched quad word (64 bit value).
  static W_ALWAYS_INLINE WUInt64 Switch(WUInt64 uiQWord) // [tested]
  {
    return (((uiQWord & 0xFF) << 56) | ((uiQWord & 0xFF00) << 40) | ((uiQWord & 0xFF0000) << 24) | ((uiQWord & 0xFF000000) << 8) |
            ((uiQWord & 0xFF00000000) >> 8) | ((uiQWord & 0xFF0000000000) >> 24) | ((uiQWord & 0xFF000000000000) >> 40) |
            ((uiQWord & 0xFF00000000000000) >> 56));
  }

  /// Switches a value in place (template accepts pointers for 2, 4 & 8 byte data types)
  template <typename T>
  static void SwitchInPlace(T* pValue) // [tested]
  {
    static_assert(
      (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8), "Switch in place only works for type equivalents of WUInt16, WUInt32, WUInt64!");

    if (sizeof(T) == 2)
    {
      struct TAnd16BitUnion
      {
        union
        {
          WUInt16 BitValue;
          T TValue;
        };
      };

      TAnd16BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 4)
    {
      struct TAnd32BitUnion
      {
        union
        {
          WUInt32 BitValue;
          T TValue;
        };
      };

      TAnd32BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 8)
    {
      struct TAnd64BitUnion
      {
        union
        {
          WUInt64 BitValue;
          T TValue;
        };
      };

      TAnd64BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
  }

#if W_ENABLED(W_PLATFORM_LITTLE_ENDIAN)

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt16* /*pWords*/, WUInt32 /*uiCount*/)
  {
  }

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt16* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt32* /*pDWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt32* /*pDWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt64* /*pQWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt64* /*pQWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt16* pWords, WUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt16* pWords, WUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt32* pDWords, WUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt32* pDWords, WUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt64* pQWords, WUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt64* pQWords, WUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

#elif W_ENABLED(W_PLATFORM_BIG_ENDIAN)

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt16* pWords, WUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt16* pWords, WUInt32 uiCount) { SwitchWords(pWords, uiCount); }

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt32* pDWords, WUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt32* pDWords, WUInt32 uiCount) { SwitchDWords(pDWords, uiCount); }

  static W_ALWAYS_INLINE void LittleEndianToNative(WUInt64* pQWords, WUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static W_ALWAYS_INLINE void NativeToLittleEndian(WUInt64* pQWords, WUInt32 uiCount) { SwitchQWords(pQWords, uiCount); }

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt16* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt16* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt32* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt32* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void BigEndianToNative(WUInt64* /*pWords*/, WUInt32 /*uiCount*/) {}

  static W_ALWAYS_INLINE void NativeToBigEndian(WUInt64* /*pWords*/, WUInt32 /*uiCount*/) {}

#endif


  /// Switches a given struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, WUInt16)
  ///  - d for a member of 4 bytes (DWORD, WUInt32)
  ///  - q for a member of 8 bytes (DWORD, WUInt64)
  static void SwitchStruct(void* pDataPointer, const char* szFormat);

  /// Templated helper method for SwitchStruct
  template <typename T>
  static void SwitchStruct(T* pDataPointer, const char* szFormat) // [tested]
  {
    SwitchStruct(static_cast<void*>(pDataPointer), szFormat);
  }

  /// Switches a given set of struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, WUInt16)
  ///  - d for a member of 4 bytes (DWORD, WUInt32)
  ///  - q for a member of 8 bytes (DWORD, WUInt64)
  static void SwitchStructs(void* pDataPointer, const char* szFormat, WUInt32 uiStride, WUInt32 uiCount); // [tested]

  /// Templated helper method for SwitchStructs
  template <typename T>
  static void SwitchStructs(T* pDataPointer, const char* szFormat, WUInt32 uiCount) // [tested]
  {
    SwitchStructs(static_cast<void*>(pDataPointer), szFormat, sizeof(T), uiCount);
  }
};
