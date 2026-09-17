#pragma once

#ifndef W_INCLUDING_BASICS_H
#  error "Please don't include StringView.h directly, but instead include Foundation/Basics.h"
#endif

#include <Foundation/Strings/StringUtils.h>

#include <Foundation/Strings/Implementation/StringIterator.h>

#include <type_traits>

#if W_ENABLED(W_INTEROP_STL_STRINGS)
#  include <string_view>
#endif

/// Base class which marks a class as containing string data
struct WThisIsAString
{
};

class WStringBuilder;

/// WStringView represent a read-only sub-string of a larger string, as it can store a dedicated string end position.
/// It derives from WStringBase and thus provides a large set of functions for search and comparisons.
///
/// Attention: WStringView does not store string data itself. It only stores pointers into memory. For example,
/// when you get an WStringView to an WStringBuilder, the WStringView instance will point to the exact same memory,
/// enabling you to iterate over it (read-only).
/// That means that an WStringView is only valid as long as its source data is not modified. Once you make any kind
/// of modification to the source data, you should not continue using the WStringView to that data anymore,
/// as it might now point into invalid memory.
class W_FOUNDATION_DLL WStringView : public WThisIsAString
{
public:
  W_DECLARE_POD_TYPE();

  using iterator = WStringIterator;
  using const_iterator = WStringIterator;
  using reverse_iterator = WStringReverseIterator;
  using const_reverse_iterator = WStringReverseIterator;

  /// Default constructor creates an invalid view.
  constexpr WStringView();

  /// Creates a string view starting at the given position, ending at the next '\0' terminator.
  WStringView(char* pStart);

  /// Creates a string view starting at the given position, ending at the next '\0' terminator.
  template <typename T>                                                                                           // T is always const char*
  constexpr WStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, int>::type* = 0); // [tested]

  /// Creates a string view from any class / struct which is implicitly convertible to const char *
  template <typename T>
  constexpr W_ALWAYS_INLINE WStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, int>::type* = 0); // [tested]

  /// Creates a string view for the range from pStart to pEnd.
  constexpr WStringView(const char* pStart, const char* pEnd); // [tested]

  /// Creates a string view for the range from pStart to pStart + uiLength.
  constexpr WStringView(const char* pStart, WUInt32 uiLength);

  /// Construct a string view from a string literal.
  template <size_t N>
  constexpr WStringView(const char (&str)[N]);

  /// Construct a string view from a fixed size buffer
  template <size_t N>
  constexpr WStringView(char (&str)[N]);

  /// Advances the start to the next character, unless the end of the range was reached.
  void operator++(); // [tested]

  /// Advances the start forwards by d characters. Does not move it beyond the range's end.
  void operator+=(WUInt32 d); // [tested]

  /// Returns the first pointed to character in Utf32 encoding.
  WUInt32 GetCharacter() const; // [tested]

  /// Returns true, if the current string pointed to is non empty.
  bool IsValid() const; // [tested]

  /// Returns the data as a zero-terminated string.
  ///
  /// The string will be copied to \a tempStorage and the pointer to that is returned.
  /// If you really need the raw pointer to the WStringView memory or are absolutely certain that the view points
  /// to a zero-terminated string, you can use GetStartPointer()
  const char* GetData(WStringBuilder& ref_sTempStorage) const; // [tested]

  /// Returns the number of bytes from the start position up to its end.
  ///
  /// \note Note that the element count (bytes) may be larger than the number of characters in that string, due to Utf8 encoding.
  WUInt32 GetElementCount() const { return m_uiElementCount; } // [tested]

  /// Allows to set the start position to a different value.
  ///
  /// Must be between the current start and end range.
  void SetStartPosition(const char* szCurPos); // [tested]

  /// Returns the start of the view range.
  /// \note Be careful to not use this and assume the view will be zero-terminated. Use GetData(WStringBuilder&) instead to be safe.
  const char* GetStartPointer() const { return m_pStart; } // [tested]

  /// Returns the end of the view range. This will point to the byte AFTER the last character.
  ///
  /// That means it might point to the '\0' terminator, UNLESS the view only represents a sub-string of a larger string.
  /// Accessing the value at 'GetEnd' has therefore no real use.
  const char* GetEndPointer() const { return m_pStart + m_uiElementCount; } // [tested]

  /// Returns whether the string is an empty string.
  bool IsEmpty() const; // [tested]

  /// Compares this string view with the other string view for equality.
  bool IsEqual(WStringView sOther) const;

  /// Compares this string view with the other string view for equality.
  bool IsEqual_NoCase(WStringView sOther) const;

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise.
  WInt32 Compare(WStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise.
  WInt32 CompareN(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise. Case insensitive.
  WInt32 Compare_NoCase(WStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise. Case insensitive.
  WInt32 CompareN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Returns true, if this string starts with the given string.
  bool StartsWith(WStringView sStartsWith) const; // [tested]

  /// Returns true, if this string starts with the given string. Case insensitive.
  bool StartsWith_NoCase(WStringView sStartsWith) const; // [tested]

  /// Returns true, if this string ends with the given string.
  bool EndsWith(WStringView sEndsWith) const; // [tested]

  /// Returns true, if this string ends with the given string. Case insensitive.
  bool EndsWith_NoCase(WStringView sEndsWith) const; // [tested]

  /// Computes the pointer to the n-th character in the string. This is a linear search from the start.
  const char* ComputeCharacterPosition(WUInt32 uiCharacterIndex) const;

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found.
  /// To find the next occurrence, use an WStringView which points to the next position and call FindSubString again.
  const char* FindSubString(WStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// To find the next occurrence, use an WStringView which points to the next position and call FindSubString again.
  const char* FindSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString(WStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString_NoCase(WStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr.
  const char* FindWholeWord(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr. Ignores case.
  const char* FindWholeWord_NoCase(const char* szSearchFor, WStringUtils::W_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]


  /// Shrinks the view range by uiShrinkCharsFront characters at the front and by uiShrinkCharsBack characters at the back.
  ///
  /// Thus reduces the range of the view to a smaller sub-string.
  /// The current position is clamped to the new start of the range.
  /// The new end position is clamped to the new start of the range.
  /// If more characters are removed from the range, than it actually contains, the view range will become 'empty'
  /// and its state will be set to invalid, however no error or assert will be triggered.
  void Shrink(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack); // [tested]

  /// Returns a sub-string that is shrunk at the start and front by the given amount of characters (not bytes!).
  WStringView GetShrunk(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack = 0) const; // [tested]

  /// Returns a sub-string starting at a given character (not byte offset!) and including a number of characters (not bytes).
  ///
  /// If this is a Utf-8 string, the correct number of bytes are skipped to reach the given character.
  /// If you instead want to construct a sub-string from byte offsets, use the WStringView constructor that takes a start pointer like so:
  ///   WStringView subString(this->GetStartPointer() + byteOffset, byteCount);
  WStringView GetSubString(WUInt32 uiFirstCharacter, WUInt32 uiNumCharacters) const; // [tested]

  /// Identical to 'Shrink(1, 0)' in functionality, but slightly more efficient.
  void ChopAwayFirstCharacterUtf8(); // [tested]

  /// Similar to ChopAwayFirstCharacterUtf8(), but assumes that the first character is ASCII and thus exactly one byte in length.
  /// Asserts that this is the case.
  /// More efficient than ChopAwayFirstCharacterUtf8(), if it is known that the first character is ASCII.
  void ChopAwayFirstCharacterAscii(); // [tested]

  /// Removes all characters from the start and end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimChars = " \f\n\r\t\v"); // [tested]

  /// Removes all characters from the start and/or end that appear in the given strings by adjusting the begin and end of the view.
  void Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd); // [tested]

  /// If the string starts with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordStart(WStringView sWord); // [tested]

  /// If the string ends with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordEnd(WStringView sWord); // [tested]

  /// Fills the given container with WStringView's which represent each found substring.
  /// If bReturnEmptyStrings is true, even empty strings between separators are returned.
  /// Output must be a container that stores WStringView's and provides the functions 'Clear' and 'Append'.
  /// szSeparator1 to szSeparator6 are strings which act as separators and indicate where to split the string.
  /// This string itself will not be modified.
  template <typename Container>
  void Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 = nullptr, const char* szSeparator3 = nullptr, const char* szSeparator4 = nullptr, const char* szSeparator5 = nullptr, const char* szSeparator6 = nullptr) const; // [tested]

  /// Returns an iterator to this string, which points to the very first character.
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  iterator GetIteratorFront() const;

  /// Returns an iterator to this string, which points to the very last character (NOT the end).
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  reverse_iterator GetIteratorBack() const;

  // ******* Path Functions ********

  /// Checks whether the given path has any file extension
  bool HasAnyExtension() const; // [tested]

  /// Checks whether the given path ends with the given extension. szExtension may start with a '.', but doesn't have to.
  ///
  /// The check is case insensitive.
  bool HasExtension(WStringView sExtension) const; // [tested]

  /// Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will return "c".
  /// If bFullExtension is true, a file named "file.a.b.c" will return "a.b.c".
  WStringView GetFileExtension(bool bFullExtension = false) const; // [tested]

  /// Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  WStringView GetFileName() const; // [tested]

  /// Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  WStringView GetFileNameAndExtension() const; // [tested]

  /// Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  WStringView GetFileDirectory() const; // [tested]

  /// Returns true, if the given path represents an absolute path on the current OS.
  bool IsAbsolutePath() const; // [tested]

  /// Returns true, if the given path represents a relative path on the current OS.
  bool IsRelativePath() const; // [tested]

  /// Returns true, if the given path represents a 'rooted' path. See WFileSystem for details.
  bool IsRootedPath() const; // [tested]

  /// Extracts the root name from a rooted path
  ///
  /// ":MyRoot" -> "MyRoot"
  /// ":MyRoot\folder" -> "MyRoot"
  /// ":\MyRoot\folder" -> "MyRoot"
  /// ":/MyRoot\folder" -> "MyRoot"
  /// Returns an empty string, if the path is not rooted.
  WStringView GetRootedPathRootName() const; // [tested]

#if W_ENABLED(W_INTEROP_STL_STRINGS)
  /// Makes the WStringView reference the same memory as the const std::string_view&.
  WStringView(const std::string_view& rhs);

  /// Makes the WStringView reference the same memory as the const std::string_view&.
  WStringView(const std::string& rhs);

  /// Returns a std::string_view to this string.
  operator std::string_view() const;

  /// Returns a std::string_view to this string.
  std::string_view GetAsStdView() const;
#endif

private:
  const char* m_pStart = nullptr;
  WUInt32 m_uiElementCount = 0;
};

/// String literal suffix to create a WStringView.
///
/// Example:
/// "Hello World"
constexpr WStringView operator"" _wsv(const char* pString, size_t uiLen);

W_ALWAYS_INLINE typename WStringView::iterator begin(WStringView sContainer)
{
  return typename WStringView::iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetStartPointer());
}

W_ALWAYS_INLINE typename WStringView::const_iterator cbegin(WStringView sContainer)
{
  return typename WStringView::const_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetStartPointer());
}

W_ALWAYS_INLINE typename WStringView::iterator end(WStringView sContainer)
{
  return typename WStringView::iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

W_ALWAYS_INLINE typename WStringView::const_iterator cend(WStringView sContainer)
{
  return typename WStringView::const_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}


W_ALWAYS_INLINE typename WStringView::reverse_iterator rbegin(WStringView sContainer)
{
  return typename WStringView::reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

W_ALWAYS_INLINE typename WStringView::const_reverse_iterator crbegin(WStringView sContainer)
{
  return typename WStringView::const_reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), sContainer.GetEndPointer());
}

W_ALWAYS_INLINE typename WStringView::reverse_iterator rend(WStringView sContainer)
{
  return typename WStringView::reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), nullptr);
}

W_ALWAYS_INLINE typename WStringView::const_reverse_iterator crend(WStringView sContainer)
{
  return typename WStringView::const_reverse_iterator(sContainer.GetStartPointer(), sContainer.GetEndPointer(), nullptr);
}

#include <Foundation/Strings/Implementation/StringView_inl.h>
