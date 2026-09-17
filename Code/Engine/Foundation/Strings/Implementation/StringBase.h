#pragma once

#include <Foundation/Strings/StringView.h>

namespace WInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl;
}

/// Base class for strings, which implements all read-only string functions.
template <typename Derived>
struct WStringBase : public WThisIsAString
{
public:
  using iterator = WStringIterator;
  using const_iterator = WStringIterator;
  using reverse_iterator = WStringReverseIterator;
  using const_reverse_iterator = WStringReverseIterator;

  /// Returns whether the string is an empty string.
  bool IsEmpty() const; // [tested]

  /// Returns true, if this string starts with the given string.
  bool StartsWith(WStringView sStartsWith) const; // [tested]

  /// Returns true, if this string starts with the given string. Case insensitive.
  bool StartsWith_NoCase(WStringView sStartsWith) const; // [tested]

  /// Returns true, if this string ends with the given string.
  bool EndsWith(WStringView sEndsWith) const; // [tested]

  /// Returns true, if this string ends with the given string. Case insensitive.
  bool EndsWith_NoCase(WStringView sEndsWith) const; // [tested]

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

  /// Compares this string with the other string for equality.
  bool IsEqual(WStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other string for equality.
  bool IsEqual_NoCase(WStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN_NoCase(WStringView sOther, WUInt32 uiCharsToCompare) const; // [tested]

  /// Computes the pointer to the n-th character in the string. This is a linear search from the start.
  const char* ComputeCharacterPosition(WUInt32 uiCharacterIndex) const;

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

  /// Returns a string view to this string's data.
  operator WStringView() const; // [tested]

  /// Returns a string view to this string's data.
  WStringView GetView() const; // [tested]

  /// Returns a pointer to the internal Utf8 string.
  W_ALWAYS_INLINE operator const char*() const { return InternalGetData(); }

  /// Fills the given container with WStringView's which represent each found substring.
  /// If bReturnEmptyStrings is true, even empty strings between separators are returned.
  /// Output must be a container that stores WStringView's and provides the functions 'Clear' and 'Append'.
  /// szSeparator1 to szSeparator6 are strings which act as separators and indicate where to split the string.
  /// This string itself will not be modified.
  template <typename Container>
  void Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 = nullptr, const char* szSeparator3 = nullptr, const char* szSeparator4 = nullptr, const char* szSeparator5 = nullptr, const char* szSeparator6 = nullptr) const; // [tested]

  /// Checks whether the given path has any file extension
  bool HasAnyExtension() const; // [tested]

  /// Checks whether the given path ends with the given extension. szExtension should start with a '.' for performance reasons, but
  /// it will work without a '.' too.
  bool HasExtension(WStringView sExtension) const; // [tested]

  /// Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
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
  /// Returns a std::string_view to this string.
  W_ALWAYS_INLINE std::string_view GetAsStdView() const
  {
    return std::string_view(InternalGetData(), static_cast<size_t>(InternalGetElementCount()));
  }
  /// Returns a std::string copy of this string.
  W_ALWAYS_INLINE std::string GetAsStdString() const
  {
    return std::string(GetAsStdView());
  }

  /// Returns a std::string_view to this string.
  W_ALWAYS_INLINE operator std::string_view() const
  {
    return GetAsStdView();
  }

  /// Returns a std::string copy of this string.
  W_ALWAYS_INLINE operator std::string() const
  {
    return std::string(GetAsStdView());
  }
#endif

private:
  const char* InternalGetData() const;
  const char* InternalGetDataEnd() const;
  WUInt32 InternalGetElementCount() const;

  template <typename Derived2>
  friend typename WStringBase<Derived2>::iterator begin(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::const_iterator cbegin(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::iterator end(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::const_iterator cend(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::reverse_iterator rbegin(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::const_reverse_iterator crbegin(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::reverse_iterator rend(const WStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename WStringBase<Derived2>::const_reverse_iterator crend(const WStringBase<Derived2>& container);
};


template <typename Derived>
typename WStringBase<Derived>::iterator begin(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetData());
}

template <typename Derived>
typename WStringBase<Derived>::const_iterator cbegin(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::const_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetData());
}

template <typename Derived>
typename WStringBase<Derived>::iterator end(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename WStringBase<Derived>::const_iterator cend(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::const_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}


template <typename Derived>
typename WStringBase<Derived>::reverse_iterator rbegin(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename WStringBase<Derived>::const_reverse_iterator crbegin(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::const_reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename WStringBase<Derived>::reverse_iterator rend(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), nullptr);
}

template <typename Derived>
typename WStringBase<Derived>::const_reverse_iterator crend(const WStringBase<Derived>& container)
{
  return typename WStringBase<Derived>::const_reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), nullptr);
}

#include <Foundation/Strings/Implementation/StringBase_inl.h>
