#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

class WStringBuilder;
class WStreamReader;

/// A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the WStringBuilder class.
/// WHybridString has an internal array to store short strings without any memory allocations, it will dynamically
/// allocate additional memory, if that cache is insufficient. Thus a hybrid string will always take up a certain amount
/// of memory, which might be of concern when it is used as a member variable, in such cases you might want to use an
/// WHybridString with a very small internal array (1 would basically make it into a completely dynamic string).
/// On the other hand, creating WHybridString instances on the stack and working locally with them, is quite fast.
/// Prefer to use the typedef'd string types \a WString, \a WDynamicString, \a WString32 etc.
/// Most strings in an application are rather short, typically shorter than 20 characters.
/// Use \a WString, which is a typedef'd WHybridString to use a cache size that is sufficient for more than 90%
/// of all use cases.
template <WUInt16 Size>
struct WHybridStringBase : public WStringBase<WHybridStringBase<Size>>
{
protected:
  /// Creates an empty string.
  WHybridStringBase(WAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  WHybridStringBase(const WHybridStringBase& rhs, WAllocator* pAllocator); // [tested]

  /// Moves the data from \a rhs.
  WHybridStringBase(WHybridStringBase&& rhs, WAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  WHybridStringBase(const char* rhs, WAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  WHybridStringBase(const wchar_t* rhs, WAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  WHybridStringBase(const WStringView& rhs, WAllocator* pAllocator); // [tested]

  /// Copies the data from \a rhs.
  WHybridStringBase(const WStringBuilder& rhs, WAllocator* pAllocator); // [tested]

  /// Moves the data from \a rhs.
  WHybridStringBase(WStringBuilder&& rhs, WAllocator* pAllocator); // [tested]

  /// Destructor.
  ~WHybridStringBase(); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const WHybridStringBase& rhs); // [tested]

  /// Moves the data from \a rhs.
  void operator=(WHybridStringBase&& rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const char* rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const wchar_t* rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const WStringView& rhs); // [tested]

  /// Copies the data from \a rhs.
  void operator=(const WStringBuilder& rhs); // [tested]

  /// Moves the data from \a rhs.
  void operator=(WStringBuilder&& rhs); // [tested]

#if W_ENABLED(W_INTEROP_STL_STRINGS)
  /// Copies the data from \a rhs.
  WHybridStringBase(const std::string_view& rhs, WAllocator* pAllocator);

  /// Copies the data from \a rhs.
  WHybridStringBase(const std::string& rhs, WAllocator* pAllocator);

  /// Copies the data from \a rhs.
  void operator=(const std::string_view& rhs);

  /// Copies the data from \a rhs.
  void operator=(const std::string& rhs);
#endif

public:
  /// Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  WUInt32 GetElementCount() const; // [tested]

  /// Returns the number of characters in this string. Might be less than GetElementCount, if it contains Utf8
  /// multi-byte characters.
  ///
  /// \note This is a slow operation, as it has to run through the entire string to count the Unicode characters.
  /// Only call this once and use the result as long as the string doesn't change. Don't call this in a loop.
  WUInt32 GetCharacterCount() const; // [tested]

  /// Returns a view to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this view will only be valid as long as this WHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  WStringView GetSubString(WUInt32 uiFirstCharacter, WUInt32 uiNumCharacters) const; // [tested]

  /// Returns a view to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this WHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  WStringView GetFirst(WUInt32 uiNumCharacters) const; // [tested]

  /// Returns a view to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this WHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  WStringView GetLast(WUInt32 uiNumCharacters) const; // [tested]

  /// Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(WStreamReader& inout_stream);

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class WStringBuilder;

  WHybridArray<char, Size> m_Data;
};


/// \see WHybridStringBase
template <WUInt16 Size, typename AllocatorWrapper = WDefaultAllocatorWrapper>
struct WHybridString : public WHybridStringBase<Size>
{
public:
  WHybridString();
  WHybridString(WAllocator* pAllocator);

  WHybridString(const WHybridString<Size, AllocatorWrapper>& other);
  WHybridString(const WHybridStringBase<Size>& other);
  WHybridString(const char* rhs);
  WHybridString(const wchar_t* rhs);
  WHybridString(const WStringView& rhs);
  WHybridString(const WStringBuilder& rhs);
  WHybridString(WStringBuilder&& rhs);
  WHybridString(WHybridString<Size, AllocatorWrapper>&& other);
  WHybridString(WHybridStringBase<Size>&& other);

  void operator=(const WHybridString<Size, AllocatorWrapper>& rhs);
  void operator=(const WHybridStringBase<Size>& rhs);
  void operator=(const char* szString);
  void operator=(const wchar_t* pString);
  void operator=(const WStringView& rhs);
  void operator=(const WStringBuilder& rhs);
  void operator=(WStringBuilder&& rhs);
  void operator=(WHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(WHybridStringBase<Size>&& rhs);

#if W_ENABLED(W_INTEROP_STL_STRINGS)
  WHybridString(const std::string_view& rhs);
  WHybridString(const std::string& rhs);
  void operator=(const std::string_view& rhs);
  void operator=(const std::string& rhs);
#endif
};

/// String that uses the static allocator to prevent leak reports in RTTI attributes.
using WUntrackedString = WHybridString<32, WStaticsAllocatorWrapper>;

using WDynamicString = WHybridString<1>;
using WString = WHybridString<32>;
using WString16 = WHybridString<16>;
using WString24 = WHybridString<24>;
using WString32 = WHybridString<32>;
using WString48 = WHybridString<48>;
using WString64 = WHybridString<64>;
using WString128 = WHybridString<128>;
using WString256 = WHybridString<256>;

static_assert(WGetTypeClass<WString>::value == WTypeIsClass::value);

template <WUInt16 Size>
struct WCompareHelper<WHybridString<Size>>
{
  static W_ALWAYS_INLINE bool Less(WStringView lhs, WStringView rhs)
  {
    return lhs.Compare(rhs) < 0;
  }

  static W_ALWAYS_INLINE bool Equal(WStringView lhs, WStringView rhs)
  {
    return lhs.IsEqual(rhs);
  }
};

struct WCompareString_NoCase
{
  static W_ALWAYS_INLINE bool Less(WStringView lhs, WStringView rhs)
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  static W_ALWAYS_INLINE bool Equal(WStringView lhs, WStringView rhs)
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// Returns true if a is less than b
  static W_ALWAYS_INLINE bool Less(const char* a, const char* b) { return WStringUtils::Compare(a, b) < 0; }

  /// Returns true if a is equal to b
  static W_ALWAYS_INLINE bool Equal(const char* a, const char* b) { return WStringUtils::IsEqual(a, b); }
};

// For WFormatString
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WString& sArg);
W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WUntrackedString& sArg);

#include <Foundation/Strings/Implementation/String_inl.h>
