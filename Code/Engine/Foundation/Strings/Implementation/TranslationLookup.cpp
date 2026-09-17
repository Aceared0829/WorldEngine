#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

bool WTranslator::s_bHighlightUntranslated = false;
WHybridArray<WTranslator*, 4> WTranslator::s_AllTranslators;

WTranslator::WTranslator()
{
  s_AllTranslators.PushBack(this);
}

WTranslator::~WTranslator()
{
  s_AllTranslators.RemoveAndSwap(this);
}

void WTranslator::Reset() {}

void WTranslator::Reload() {}

void WTranslator::ReloadAllTranslators()
{
  W_LOG_BLOCK("ReloadAllTranslators");

  for (WTranslator* pTranslator : s_AllTranslators)
  {
    pTranslator->Reload();
  }
}

void WTranslator::HighlightUntranslated(bool bHighlight)
{
  if (s_bHighlightUntranslated == bHighlight)
    return;

  s_bHighlightUntranslated = bHighlight;

  ReloadAllTranslators();
}

//////////////////////////////////////////////////////////////////////////

WHybridArray<WUniquePtr<WTranslator>, 16> WTranslationLookup::s_Translators;

void WTranslationLookup::AddTranslator(WUniquePtr<WTranslator> pTranslator)
{
  s_Translators.PushBack(std::move(pTranslator));
}


WStringView WTranslationLookup::Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  for (WUInt32 i = s_Translators.GetCount(); i > 0; --i)
  {
    WStringView sResult = s_Translators[i - 1]->Translate(sString, uiStringHash, usage);

    if (!sResult.IsEmpty())
      return sResult;
  }

  if (usage != WTranslationUsage::Default)
    return {};

  return sString;
}


void WTranslationLookup::Clear()
{
  s_Translators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void WTranslatorFromFiles::AddTranslationFilesFromFolder(const char* szFolder)
{
  W_LOG_BLOCK("AddTranslationFilesFromFolder", szFolder);

  if (!m_Folders.Contains(szFolder))
  {
    m_Folders.PushBack(szFolder);
  }

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)
  WStringBuilder startPath;
  if (WFileSystem::ResolvePath(szFolder, &startPath, nullptr).Failed())
    return;

  WStringBuilder fullpath;

  WFileSystemIterator it;
  it.StartSearch(startPath, WFileSystemIteratorFlags::ReportFilesRecursive);


  while (it.IsValid())
  {
    fullpath = it.GetCurrentPath();
    fullpath.AppendPath(it.GetStats().m_sName);

    LoadTranslationFile(fullpath);

    it.Next();
  }

#endif
}

WStringView WTranslatorFromFiles::Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  return WTranslatorStorage::Translate(sString, uiStringHash, usage);
}

void WTranslatorFromFiles::Reload()
{
  WTranslatorStorage::Reload();

  for (const auto& sFolder : m_Folders)
  {
    AddTranslationFilesFromFolder(sFolder);
  }
}

void WTranslatorFromFiles::LoadTranslationFile(const char* szFullPath)
{
  W_LOG_BLOCK("LoadTranslationFile", szFullPath);

  WLog::Dev("Loading Localization File '{0}'", szFullPath);

  WFileReader file;
  if (file.Open(szFullPath).Failed())
  {
    WLog::Warning("Failed to open localization file '{0}'", szFullPath);
    return;
  }

  WStringBuilder sContent;
  sContent.ReadAll(file);

  WDeque<WStringView> Lines;
  sContent.Split(false, Lines, "\n");

  WTempHybridArray<WStringView, 4> entries;

  WStringBuilder sLine, sKey, sValue, sTooltip, sHelpUrl;
  for (const auto& line : Lines)
  {
    sLine = line;
    sLine.Trim(" \t\r\n");

    if (sLine.IsEmpty() || sLine.StartsWith("#"))
      continue;

    entries.Clear();
    sLine.Split(true, entries, ";");

    if (entries.GetCount() <= 1)
    {
      WLog::Error("Invalid line in translation file: '{0}'", sLine);
      continue;
    }

    sKey = entries[0];
    sValue = entries[1];

    sTooltip.Clear();
    sHelpUrl.Clear();

    if (entries.GetCount() >= 3)
      sTooltip = entries[2];
    if (entries.GetCount() >= 4)
      sHelpUrl = entries[3];

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");
    sTooltip.Trim(" \t\r\n");
    sHelpUrl.Trim(" \t\r\n");

    sTooltip.ReplaceAll("\\n", "\n");

    if (GetHighlightUntranslated())
    {
      sValue.Prepend("# ");
      sValue.Append(" (@", sKey, ")");
    }

    StoreTranslation(sValue, WHashingUtils::StringHash(sKey), WTranslationUsage::Default);
    StoreTranslation(sTooltip, WHashingUtils::StringHash(sKey), WTranslationUsage::Tooltip);
    StoreTranslation(sHelpUrl, WHashingUtils::StringHash(sKey), WTranslationUsage::HelpURL);
  }
}

//////////////////////////////////////////////////////////////////////////

void WTranslatorStorage::StoreTranslation(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  m_Translations[(WUInt32)usage][uiStringHash] = sString;
}

WStringView WTranslatorStorage::Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  W_IGNORE_UNUSED(sString);

  auto it = m_Translations[(WUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return {};
}

void WTranslatorStorage::Reset()
{
  for (WUInt32 i = 0; i < (WUInt32)WTranslationUsage::ENUM_COUNT; ++i)
  {
    m_Translations[i].Clear();
  }
}

void WTranslatorStorage::Reload()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////

bool WTranslatorLogMissing::s_bActive = true;

WStringView WTranslatorLogMissing::Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  if (!WTranslatorLogMissing::s_bActive && !GetHighlightUntranslated())
    return {};

  if (usage != WTranslationUsage::Default)
    return {};

  WStringView sResult = WTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (sResult.IsEmpty())
  {
    WLog::Warning("Missing translation: {0};", sString);

    StoreTranslation(sString, uiStringHash, usage);
  }

  return {};
}

WStringView WTranslatorMakeMoreReadable::Translate(WStringView sString, WUInt64 uiStringHash, WTranslationUsage usage)
{
  if (usage != WTranslationUsage::Default)
    return {};

  WStringView sResult = WTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (!sResult.IsEmpty())
    return sResult;

  WStringBuilder result;
  WStringBuilder tmp = sString;
  tmp.Trim(" _-");

  if (tmp.TrimWordStart("W"))
  {
    WStringView sComponent = "Component";
    if (tmp.EndsWith(sComponent) && tmp.GetElementCount() > sComponent.GetElementCount())
    {
      tmp.Shrink(0, sComponent.GetElementCount());
    }
  }

  auto IsUpper = [](WUInt32 c)
  { return c == WStringUtils::ToUpperChar(c); };
  auto IsNumber = [](WUInt32 c)
  { return c >= '0' && c <= '9'; };

  WUInt32 uiPrev = ' ';
  WUInt32 uiCur = ' ';
  WUInt32 uiNext = ' ';

  bool bContinue = true;

  for (auto it = tmp.GetIteratorFront(); bContinue; ++it)
  {
    uiPrev = uiCur;
    uiCur = uiNext;

    if (it.IsValid())
    {
      uiNext = it.GetCharacter();
    }
    else
    {
      uiNext = ' ';
      bContinue = false;
    }

    if (uiCur == '_')
      uiCur = ' ';

    if (uiCur == ':')
    {
      result.Clear();
      continue;
    }

    if (uiPrev != '[' && uiCur != ']' && IsNumber(uiPrev) != IsNumber(uiCur))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (IsNumber(uiPrev) && IsNumber(uiCur))
    {
      result.Append(uiCur);
      continue;
    }

    if (IsUpper(uiPrev) && IsUpper(uiCur) && !IsUpper(uiNext))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (!IsUpper(uiCur) && IsUpper(uiNext))
    {
      result.Append(uiCur);
      result.Append(" ");
      continue;
    }

    result.Append(uiCur);
  }

  result.Trim(" ");
  while (result.ReplaceAll("  ", " ") > 0)
  {
    // remove double whitespaces
  }

  if (GetHighlightUntranslated())
  {
    result.Append(" (@", sString, ")");
  }

  StoreTranslation(result, uiStringHash, usage);

  return WTranslatorStorage::Translate(sString, uiStringHash, usage);
}
