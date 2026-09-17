#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>

/// A reference to a file or folder inside a data directory.
///
/// Allows quick access to various sub-parts of the path as well as the data dir index.
/// To construct a WDataDirPath, the list of absolute data directory root directories must be supplied in order to determine whether the path is inside a data directory and in which. After calling the constructor, IsValid() should be called to determine if the file is inside a data directory.
/// The various sub-parts look like this with "Testing Chambers" being the data directory in this example:
///
///  GetAbsolutePath() == "C:/WorldEngine/Data/Samples/Testing Chambers/Objects/Barrel.WPrefab"
///  GetDataDir() == "C:/WorldEngine/Data/Samples/Testing Chambers"
///  GetDataDirParentRelativePath() == "Testing Chambers/Objects/Barrel.WPrefab"
///  GetDataDirRelativePath() == "Objects/Barrel.WPrefab"
class W_TOOLSFOUNDATION_DLL WDataDirPath
{
public:
  /// \name Constructor
  ///@{

  /// Default ctor, creates an invalid data directory path.
  WDataDirPath();
  /// Tries to create a new data directory path from an absolute path. Check IsValid afterwards to confirm this path is inside a data directory.
  /// \param sAbsPath Absolute path to the file or folder. Must be normalized. Must not end with "/".
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if the data directory index is known.
  WDataDirPath(WStringView sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex = 0);
  /// Overload for WStringBuilder to fix ambiguity between implicit conversions.
  WDataDirPath(const WStringBuilder& sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex = 0);
  /// Move constructor overload for the absolute path.
  WDataDirPath(WString&& sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex = 0);


  ///@}
  /// \name Misc
  ///@{

  /// Returns the same path this instance was created with. Calling this function is always valid.
  const WString& GetAbsolutePath() const;
  /// Returns whether this path is inside a data directory. If not, none of the Get* functions except for GetAbsolutePath are allowed to be called.
  bool IsValid() const;
  /// Same as the default constructor. Creates an empty, invalid path.
  void Clear();

  ///@}
  /// \name Operators
  ///@{

  operator WStringView() const;
  bool operator==(WStringView rhs) const;
  bool operator!=(WStringView rhs) const;

  ///@}
  /// \name Data directory access. Only allowed to be called if IsValid() is true.
  ///@{

  /// Returns a relative path including the data directory the path belongs to, e.g. "Testing Chambers/Objects/Barrel.WPrefab".
  WStringView GetDataDirParentRelativePath() const;
  /// Returns a path relative to the data directory the path belongs to, e.g. "Objects/Barrel.WPrefab".
  WStringView GetDataDirRelativePath() const;
  /// Returns absolute path to the data directory this path belongs to, e.g.  "C:/WorldEngine/Data/Samples/Testing Chambers".
  WStringView GetDataDir() const;
  /// Returns the index of the data directory the path belongs to.
  WUInt8 GetDataDirIndex() const;

  ///@}
  /// \name Data directory update
  ///@{

  /// If a WDataDirPath is de-serialized, it might not be correct anymore and its data directory reference must be updated. It could potentially no longer be part of any data directory at all and become invalid so after calling this function, IsValid will match the return value of this function. On failure, the invalid data directory paths should then be destroyed.
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if nothing has changed.
  /// \return Returns whether the data directory path is valid, i.e. it is still under one of the dataDirRoots.
  bool UpdateDataDirInfos(WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex = 0) const;

  ///@}
  /// \name Serialization
  ///@{

  WStreamWriter& Write(WStreamWriter& inout_stream) const;
  WStreamReader& Read(WStreamReader& inout_stream);

  ///@}

private:
  WString m_sAbsolutePath;
  mutable WUInt16 m_uiDataDirParent = 0;
  mutable WUInt8 m_uiDataDirLength = 0;
  mutable WUInt8 m_uiDataDirIndex = 0;
};

WStreamWriter& operator<<(WStreamWriter& inout_stream, const WDataDirPath& value);
WStreamReader& operator>>(WStreamReader& inout_stream, WDataDirPath& out_value);

/// Comparator that first sort case-insensitive and then case-sensitive if necessary for a unique ordering.
///
/// Use this comparator when sorting e.g. files on disk like they would appear in a windows explorer.
/// This comparator is using WStringView instead of WDataDirPath as all string and WDataDirPath can be implicitly converted to WStringView.
struct W_TOOLSFOUNDATION_DLL WCompareDataDirPath
{
  static inline bool Less(WStringView lhs, WStringView rhs);
  static inline bool Equal(WStringView lhs, WStringView rhs);
};

#include <ToolsFoundation/FileSystem/Implementation/DataDirPath_inl.h>
