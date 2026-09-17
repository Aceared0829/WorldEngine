#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringBuilder.h>

const char* WPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (WStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    WUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szPathStart).AssertSuccess();

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool WPathUtils::HasAnyExtension(WStringView sPath)
{
  return !GetFileExtension(sPath, true).IsEmpty();
}

bool WPathUtils::HasExtension(WStringView sPath, WStringView sExtension)
{
  sPath = GetFileNameAndExtension(sPath);
  WStringView fullExt = GetFileExtension(sPath, true);

  if (sExtension.IsEmpty() && fullExt.IsEmpty())
    return true;

  // if there is a single dot at the start of the extension, remove it
  if (sExtension.StartsWith("."))
    sExtension.ChopAwayFirstCharacterAscii();

  if (!fullExt.EndsWith_NoCase(sExtension))
    return false;

  // remove the checked extension
  sPath = WStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - sExtension.GetElementCount());

  // checked extension didn't start with a dot -> make sure there is one at the end of sPath
  if (!sPath.EndsWith("."))
    return false;

  // now make sure the rest isn't just the dot
  return sPath.GetElementCount() > 1;
}

WStringView WPathUtils::GetFileExtension(WStringView sPath, bool bFullExtension)
{
  // get rid of any path before the filename
  sPath = GetFileNameAndExtension(sPath);

  // ignore all dots that the file name may start with (".", "..", ".file", "..file", etc)
  // filename may be empty afterwards, which means no dot will be found -> no extension
  while (sPath.StartsWith("."))
    sPath.ChopAwayFirstCharacterAscii();

  const char* szDot;

  if (bFullExtension)
  {
    szDot = sPath.FindSubString(".");
  }
  else
  {
    szDot = sPath.FindLastSubString(".");
  }

  // no dot at all -> no extension
  if (szDot == nullptr)
    return WStringView();

  // dot at the very end of the string -> not an extension
  if (szDot + 1 == sPath.GetEndPointer())
    return WStringView();

  return WStringView(szDot + 1, sPath.GetEndPointer());
}

WStringView WPathUtils::GetFileNameAndExtension(WStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return WStringView(szSeparator + 1, sPath.GetEndPointer());
}

WStringView WPathUtils::GetFileName(WStringView sPath, bool bRemoveFullExtension)
{
  // reduce the problem to just the filename + extension
  sPath = GetFileNameAndExtension(sPath);

  return GetWithoutExtension(sPath, bRemoveFullExtension);
}

WStringView WPathUtils::GetWithoutExtension(WStringView sPath, bool bRemoveFullExtension)
{
  // TODO: unit test

  WStringView ext = GetFileExtension(sPath, bRemoveFullExtension);

  if (ext.IsEmpty())
    return sPath;

  return WStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - ext.GetElementCount() - 1);
}

WStringView WPathUtils::GetFileDirectory(WStringView sPath)
{
  auto it = rbegin(sPath);

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(it.GetCharacter()))
    return sPath;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return WStringView(nullptr);

  return WStringView(sPath.GetStartPointer(), szSeparator + 1);
}

const char WPathUtils::OsSpecificPathSeparator = W_PLATFORM_PATH_SEPARATOR;

bool WPathUtils::IsAbsolutePath(WStringView sPath)
{
  if (sPath.GetElementCount() < 1)
    return false;

  const char* szPath = sPath.GetStartPointer();

#if W_ENABLED(W_PLATFORM_WINDOWS)

  if (sPath.GetElementCount() < 2)
    return false;

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#else
  return (szPath[0] == '/');
#endif
}

bool WPathUtils::IsRelativePath(WStringView sPath)
{
  if (sPath.IsEmpty())
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (WPathUtils::IsPathSeparator(*sPath.GetStartPointer()))
    return false;

  return !IsAbsolutePath(sPath) && !IsRootedPath(sPath);
}

bool WPathUtils::IsRootedPath(WStringView sPath)
{
  return !sPath.IsEmpty() && *sPath.GetStartPointer() == ':';
}

void WPathUtils::GetRootedPathParts(WStringView sPath, WStringView& ref_sRoot, WStringView& ref_sRelPath)
{
  ref_sRoot = WStringView();
  ref_sRelPath = sPath;

  if (!IsRootedPath(sPath))
    return;

  const char* szStart = sPath.GetStartPointer();
  const char* szPathEnd = sPath.GetEndPointer();

  do
  {
    WUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd).AssertSuccess();

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  WUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    WUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  ref_sRoot = WStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = WStringView();
  }
  else
  {
    // skip path separator for the relative path
    WUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();
    ref_sRelPath = WStringView(szEnd, szPathEnd);
  }
}

WStringView WPathUtils::GetRootedPathRootName(WStringView sPath)
{
  WStringView root, relPath;
  GetRootedPathParts(sPath, root, relPath);
  return root;
}

bool WPathUtils::IsValidFilenameChar(WUInt32 uiCharacter)
{
  /// \test Not tested yet

  // Windows: https://msdn.microsoft.com/library/windows/desktop/aa365247(v=vs.85).aspx
  // Unix: https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words
  // Details can be more complicated (there might be reserved names depending on the filesystem), but in general all platforms behave like
  // this:
  static const WUInt32 forbiddenFilenameChars[] = {'<', '>', ':', '"', '|', '?', '*', '\\', '/', '\t', '\b', '\n', '\r', '\0'};

  for (int i = 0; i < W_ARRAY_SIZE(forbiddenFilenameChars); ++i)
  {
    if (forbiddenFilenameChars[i] == uiCharacter)
      return false;
  }

  return true;
}

bool WPathUtils::ContainsInvalidFilenameChars(WStringView sPath)
{
  /// \test Not tested yet

  WStringIterator it = sPath.GetIteratorFront();

  for (; it.IsValid(); ++it)
  {
    if (!IsValidFilenameChar(it.GetCharacter()))
      return true;
  }

  return false;
}

void WPathUtils::MakeValidFilename(WStringView sFilename, WUInt32 uiReplacementCharacter, WStringBuilder& out_sFilename)
{
  W_ASSERT_DEBUG(IsValidFilenameChar(uiReplacementCharacter), "Given replacement character is not allowed for filenames.");

  out_sFilename.Clear();

  for (auto it = sFilename.GetIteratorFront(); it.IsValid(); ++it)
  {
    WUInt32 currentChar = it.GetCharacter();

    if (IsValidFilenameChar(currentChar) == false)
      out_sFilename.Append(uiReplacementCharacter);
    else
      out_sFilename.Append(currentChar);
  }
}

void WPathUtils::NormalizeWindowsDriveLetter(WStringBuilder& ref_sAbsolutePath)
{
  if (ref_sAbsolutePath.GetElementCount() >= 2 && ref_sAbsolutePath.GetData()[1] == ':')
  {
    const WUInt32 uiChar = static_cast<WUInt8>(ref_sAbsolutePath.GetData()[0]);

    // Only ASCII can be a drive letter, which also guarantees that the replacement is a single byte.
    if (WUnicodeUtils::IsASCII(uiChar))
    {
      const WUInt32 uiUpper = WStringUtils::ToUpperChar(uiChar);

      const char szUpper[2] = {static_cast<char>(uiUpper), '\0'};
      ref_sAbsolutePath.ReplaceSubString(ref_sAbsolutePath.GetData(), ref_sAbsolutePath.GetData() + 1, szUpper);
    }
  }
}

bool WPathUtils::IsSubPath(WStringView sPrefixPath, WStringView sFullPath0)
{
  if (sPrefixPath.IsEmpty())
  {
    if (sFullPath0.IsAbsolutePath())
      return true;

    W_REPORT_FAILURE("Prefixpath is empty and checked path is not absolute.");
    return false;
  }

  WStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  WStringBuilder sFullPath = sFullPath0;
  sFullPath.MakeCleanPath();

  if (sFullPath.StartsWith(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetData()[tmp.GetElementCount()] == '/';
  }

  return false;
}

bool WPathUtils::IsSubPath_NoCase(WStringView sPrefixPath, WStringView sFullPath)
{
  WStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  if (sFullPath.StartsWith_NoCase(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetStartPointer()[tmp.GetElementCount()] == '/';
  }

  return false;
}
