#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace WTokenParseUtils;

WMap<WString, WTokenizedFileCache::FileData>::ConstIterator WTokenizedFileCache::Lookup(const WString& sFileName) const
{
  W_LOCK(m_Mutex);
  auto it = m_Cache.Find(sFileName);
  return it;
}

void WTokenizedFileCache::Remove(const WString& sFileName)
{
  W_LOCK(m_Mutex);
  m_Cache.Remove(sFileName);
}

void WTokenizedFileCache::Clear()
{
  W_LOCK(m_Mutex);
  m_Cache.Clear();
}

void WTokenizedFileCache::SkipWhitespace(WDeque<WToken>& Tokens, WUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken].m_iType == WTokenType::BlockComment || Tokens[uiCurToken].m_iType == WTokenType::LineComment || Tokens[uiCurToken].m_iType == WTokenType::Newline || Tokens[uiCurToken].m_iType == WTokenType::Whitespace))
    ++uiCurToken;
}

const WTokenizer* WTokenizedFileCache::Tokenize(const WString& sFileName, WArrayPtr<const WUInt8> fileContent, const WTimestamp& fileTimeStamp, WLogInterface* pLog)
{
  W_LOCK(m_Mutex);

  bool bExisted = false;
  auto it = m_Cache.FindOrAdd(sFileName, &bExisted);
  if (bExisted)
  {
    return &it.Value().m_Tokens;
  }

  auto& data = it.Value();

  data.m_Timestamp = fileTimeStamp;
  WTokenizer* pTokenizer = &data.m_Tokens;
  pTokenizer->Tokenize(fileContent, pLog);

  WDeque<WToken>& Tokens = pTokenizer->GetTokens();

  WHashedString sFile;
  sFile.Assign(sFileName);

  WInt32 iLineOffset = 0;

  for (WUInt32 i = 0; i + 1 < Tokens.GetCount(); ++i)
  {
    const WUInt32 uiCurLine = Tokens[i].m_uiLine;

    Tokens[i].m_File = sFile;
    Tokens[i].m_uiLine += iLineOffset;

    if (Tokens[i].m_iType == WTokenType::NonIdentifier && Tokens[i].m_DataView.IsEqual("#"))
    {
      WUInt32 uiNext = i + 1;

      SkipWhitespace(Tokens, uiNext);

      if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == WTokenType::Identifier && Tokens[uiNext].m_DataView.IsEqual("line"))
      {
        ++uiNext;
        SkipWhitespace(Tokens, uiNext);

        if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == WTokenType::Integer)
        {
          WInt32 iNextLine = 0;

          const WString sNumber = Tokens[uiNext].m_DataView;
          if (WConversionUtils::StringToInt(sNumber, iNextLine).Succeeded())
          {
            iLineOffset = (iNextLine - uiCurLine) - 1;

            ++uiNext;
            SkipWhitespace(Tokens, uiNext);

            if (uiNext < Tokens.GetCount())
            {
              if (Tokens[uiNext].m_iType == WTokenType::String1)
              {
                WStringBuilder sFileName2 = Tokens[uiNext].m_DataView;
                sFileName2.Shrink(1, 1); // remove surrounding "

                sFile.Assign(sFileName2);
              }
            }
          }
        }
      }
    }
  }

  return pTokenizer;
}


void WPreprocessor::SetLogInterface(WLogInterface* pLog)
{
  m_pLog = pLog;
}

void WPreprocessor::SetFileOpenFunction(FileOpenCB openAbsFileCB)
{
  m_FileOpenCallback = openAbsFileCB;
}

void WPreprocessor::SetFileLocatorFunction(FileLocatorCB locateAbsFileCB)
{
  m_FileLocatorCallback = locateAbsFileCB;
}

WResult WPreprocessor::DefaultFileLocator(WStringView sCurAbsoluteFile, WStringView sIncludeFile, WPreprocessor::IncludeType incType, WStringBuilder& out_sAbsoluteFilePath)
{
  WStringBuilder& s = out_sAbsoluteFilePath;

  if (incType == WPreprocessor::RelativeInclude)
  {
    s = sCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(sIncludeFile);
    s.MakeCleanPath();
  }
  else
  {
    s = sIncludeFile;
    s.MakeCleanPath();
  }

  return W_SUCCESS;
}

WResult WPreprocessor::DefaultFileOpen(WStringView sAbsoluteFile, WDynamicArray<WUInt8>& ref_fileContent, WTimestamp& out_fileModification)
{
  WFileReader r;
  if (r.Open(sAbsoluteFile).Failed())
    return W_FAILURE;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  WFileStats stats;
  if (WFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
    out_fileModification = stats.m_LastModificationTime;
#endif

  WUInt8 Temp[4096];

  while (WUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    ref_fileContent.PushBackRange(WArrayPtr<WUInt8>(Temp, (WUInt32)uiRead));
  }

  return W_SUCCESS;
}

WResult WPreprocessor::OpenFile(WStringView sFile, const WTokenizer** pTokenizer)
{
  W_ASSERT_DEV(m_FileOpenCallback.IsValid(), "OpenFile callback has not been set");
  W_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_pUsedFileCache->Lookup(sFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value().m_Tokens;
    return W_SUCCESS;
  }

  WTimestamp stamp;

  WDynamicArray<WUInt8> Content;
  if (m_FileOpenCallback(sFile, Content, stamp).Failed())
  {
    WLog::Error(m_pLog, "Could not open file '{0}'", sFile);
    return W_FAILURE;
  }

  WArrayPtr<const WUInt8> ContentView = Content;

  // the file open callback gives us raw data for the opened file
  // the tokenizer doesn't like the Utf8 BOM, so skip it here, if we detect it
  if (ContentView.GetCount() >= 3) // length of a BOM
  {
    const char* dataStart = reinterpret_cast<const char*>(ContentView.GetPtr());

    if (WUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      ContentView = WArrayPtr<const WUInt8>((const WUInt8*)dataStart, Content.GetCount() - 3);
    }
  }

  *pTokenizer = m_pUsedFileCache->Tokenize(sFile, ContentView, stamp, m_pLog);

  return W_SUCCESS;
}


WResult WPreprocessor::HandleInclude(const TokenStream& Tokens0, WUInt32 uiCurToken, WUInt32 uiDirectiveToken, TokenStream& TokenOutput)
{
  W_IGNORE_UNUSED(uiDirectiveToken);
  W_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  TokenStream Tokens;
  if (Expand(Tokens0, Tokens).Failed())
    return W_FAILURE;

  SkipWhitespace(Tokens, uiCurToken);

  WStringBuilder sPath;

  IncludeType IncType = IncludeType::GlobalInclude;


  WUInt32 uiAccepted;
  if (Accept(Tokens, uiCurToken, WTokenType::String1, &uiAccepted))
  {
    IncType = IncludeType::RelativeInclude;
    sPath = Tokens[uiAccepted]->m_DataView;
    sPath.Shrink(1, 1); // remove " at start and end
  }
  else
  {
    // in global include paths (ie. <bla/blub.h>) we need to handle line comments special
    // because a path with two slashes will be a comment token, although it could be a valid path
    // so we concatenate just everything and then make sure it ends with a >

    if (Expect(Tokens, uiCurToken, "<", &uiAccepted).Failed())
      return W_FAILURE;

    TokenStream PathTokens;

    while (uiCurToken < Tokens.GetCount())
    {
      if (Tokens[uiCurToken]->m_iType == WTokenType::Newline)
      {
        break;
      }

      PathTokens.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
    }

    CombineTokensToString(PathTokens, 0, sPath, false);

    // remove all whitespace at the end (this could be part of a comment, so not tokenized as whitespace)
    while (sPath.EndsWith(" ") || sPath.EndsWith("\t"))
      sPath.Shrink(0, 1);

    // there must always be a > at the end, although it could be a separate token or part of a comment
    // so we check the string, instead of the tokens
    if (sPath.EndsWith(">"))
      sPath.Shrink(0, 1);
    else
    {
      PP_LOG(Error, "Invalid include path '{0}'", Tokens[uiAccepted], sPath);
      return W_FAILURE;
    }
  }

  if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
  {
    PP_LOG0(Error, "Expected end-of-line", Tokens[uiCurToken]);
    return W_FAILURE;
  }

  W_ASSERT_DEV(!m_CurrentFileStack.IsEmpty(), "Implementation error.");

  WStringBuilder sOtherFile;

  if (m_FileLocatorCallback(m_CurrentFileStack.PeekBack().m_sFileName.GetData(), sPath, IncType, sOtherFile).Failed())
  {
    PP_LOG(Error, "#include file '{0}' could not be located", Tokens[uiAccepted], sPath);
    return W_FAILURE;
  }

  const WTempHashedString sOtherFileHashed(sOtherFile);

  // if this has been included before, and contains a #pragma once, do not include it again
  if (m_PragmaOnce.Find(sOtherFileHashed).IsValid())
    return W_SUCCESS;

  if (m_bImplicitPragmaOnce)
  {
    // don't include it again next time
    m_PragmaOnce.Insert(sOtherFileHashed);
  }

  if (ProcessFile(sOtherFile, TokenOutput, uiCurToken < Tokens.GetCount() ? Tokens[uiCurToken] : nullptr).Failed())
    return W_FAILURE;

  if (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken]->m_iType == WTokenType::Newline || Tokens[uiCurToken]->m_iType == WTokenType::EndOfFile))
    TokenOutput.PushBack(Tokens[uiCurToken]);

  return W_SUCCESS;
}
