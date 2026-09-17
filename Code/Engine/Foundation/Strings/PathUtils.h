#pragma once

#include <Foundation/Strings/StringView.h>
#include <Foundation/Strings/UnicodeUtils.h>

class WStringBuilder;

/// Contains Helper functions to work with paths.
///
/// Only functions that require read-only access to a string are provided here
/// All functions that require to modify the path are provided by WStringBuilder.
/// Many functions return WStringView's, which will always be strict sub-strings of their input data.
/// That allows that these functions can work without any additional memory allocations.
class W_FOUNDATION_DLL WPathUtils
{
public:
  /// The path separator used by this operating system.
  static const char OsSpecificPathSeparator;

  /// Returns whether c is any known path separator.
  static bool IsPathSeparator(WUInt32 c); // [tested]

  /// Checks if a given character is allowed in a filename (not path!)
  static bool IsValidFilenameChar(WUInt32 uiCharacter);

  /// Checks every character in the string with IsValidFilenameChar()
  ///
  /// This is a basic check, only because each character passes the test, it does not guarantee that the full string is a valid path.
  static bool ContainsInvalidFilenameChars(WStringView sPath);

  /// Searches for the previous path separator before szStartSearchAt. Will return nullptr if it reaches szPathStart before finding
  /// any separator.
  static const char* FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt); // [tested]

  /// Checks whether the given path has any file extension
  static bool HasAnyExtension(WStringView sPath); // [tested]

  /// Checks whether the path ends with the given file extension.
  /// szExtension may or may not start with a dot.
  /// The check is case insensitive.
  ///
  ///   HasExtension("file.txt", "txt") -> true
  ///   HasExtension("file.txt", ".txt") -> true
  ///   HasExtension("file.a.b", ".b") -> true
  ///   HasExtension("file.a.b", "a.b") -> true
  ///   HasExtension("file.a.b", ".a.b") -> true
  ///   HasExtension("file.a.b", "file.a.b") -> false
  static bool HasExtension(WStringView sPath, WStringView sExtension); // [tested]

  /// Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension. The dot (.) is not included.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will return "c".
  /// If bFullExtension is true, a file named "file.a.b.c" will return "a.b.c".
  static WStringView GetFileExtension(WStringView sPath, bool bFullExtension = false); // [tested]

  /// Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  static WStringView GetFileName(WStringView sPath, bool bRemoveFullExtension = false); // [tested]

  /// Returns the path, excluding the file extension.
  static WStringView GetWithoutExtension(WStringView sPath, bool bRemoveFullExtension = false);

  /// Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  static WStringView GetFileNameAndExtension(WStringView sPath); // [tested]

  /// Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  static WStringView GetFileDirectory(WStringView sPath); // [tested]

  /// Returns true, if the given path represents an absolute path on the current OS.
  static bool IsAbsolutePath(WStringView sPath); // [tested]

  /// Returns true, if the given path represents a relative path on the current OS.
  static bool IsRelativePath(WStringView sPath); // [tested]

  /// A rooted path starts with a colon and then names a file-system data directory. Rooted paths are used as 'absolute' paths within
  /// the WFileSystem.
  static bool IsRootedPath(WStringView sPath); // [tested]

  /// Splits the passed path into its root portion and the relative path
  ///
  /// ":MyRoot\file.txt" -> root = "MyRoot", relPath="file.txt"
  /// ":MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// ":\MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// ":/MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// If the path is not rooted, then root will be an empty string and relPath is set to the full input path.
  static void GetRootedPathParts(WStringView sPath, WStringView& ref_sRoot, WStringView& ref_sRelPath); // [tested]

  /// Special case of GetRootedPathParts that returns the root of the input path and discards the relative path
  static WStringView GetRootedPathRootName(WStringView sPath); // [tested]

  /// Creates a valid filename (not path!) using the given string by replacing all disallowed characters.
  ///
  /// Note that path separators in the given string will be replaced as well!
  /// Asserts that replacementCharacter is an allowed character.
  /// \see IsValidFilenameChar()
  static void MakeValidFilename(WStringView sFilename, WUInt32 uiReplacementCharacter, WStringBuilder& out_sFilename);

  /// Makes the drive letter of an absolute Windows path upper case.
  ///
  /// Checks whether this is a Windows-specific absolute path. Does nothing if not.
  static void NormalizeWindowsDriveLetter(WStringBuilder& ref_sAbsolutePath); // [tested]

  /// Checks whether \a sFullPath starts with \a sPrefixPath.
  static bool IsSubPath(WStringView sPrefixPath, WStringView sFullPath); // [tested]
  /// Checks whether \a sFullPath starts with \a sPrefixPath. Case insensitive.
  static bool IsSubPath_NoCase(WStringView sPrefixPath, WStringView sFullPath); // [tested]
};

#include <Foundation/Strings/Implementation/PathUtils_inl.h>
