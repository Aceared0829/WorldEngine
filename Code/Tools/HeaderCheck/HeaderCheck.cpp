#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/LinearAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>


namespace
{
  W_ALWAYS_INLINE void SkipWhitespace(WToken& ref_token, WUInt32& i, const WDeque<WToken>& tokens)
  {
    while (ref_token.m_iType == WTokenType::Whitespace)
    {
      ref_token = tokens[++i];
    }
  }

  W_ALWAYS_INLINE void SkipLine(WToken& ref_token, WUInt32& i, const WDeque<WToken>& tokens)
  {
    while (ref_token.m_iType != WTokenType::Newline && ref_token.m_iType != WTokenType::EndOfFile)
    {
      ref_token = tokens[++i];
    }
  }
} // namespace

class WHeaderCheckApp : public WApplication
{
private:
  WString m_sSearchDir;
  WString m_sProjectName;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  WUniquePtr<WLinearAllocator<WAllocatorTrackingMode::Nothing>> m_pLinearAllocator;
  WDynamicArray<WString> m_IncludeDirectories;

  struct IgnoreInfo
  {
    WHashSet<WString> m_byName;
  };

  IgnoreInfo m_IgnoreTarget;
  IgnoreInfo m_IgnoreSource;

public:
  using SUPER = WApplication;

  WHeaderCheckApp()
    : WApplication("HeaderCheck")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const WLoggingEventData& eventData)
  {
    WHeaderCheckApp* app = (WHeaderCheckApp*)WApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case WLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case WLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case WLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  WResult ParseArray(const WVariant& value, WHashSet<WString>& ref_dst)
  {
    if (!value.CanConvertTo<WVariantArray>())
    {
      WLog::Error("Expected array");
      return W_FAILURE;
    }
    auto a = value.Get<WVariantArray>();
    const auto arraySize = a.GetCount();
    for (WUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<WString>())
      {
        WLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return W_FAILURE;
      }
      WStringBuilder file = el.Get<WString>();
      file.ToLower();
      ref_dst.Insert(file);
    }
    return W_SUCCESS;
  }

  WResult ParseIgnoreFile(const WStringView sIgnoreFilePath)
  {
    WJSONReader jsonReader;
    jsonReader.SetLogInterface(WLog::GetThreadLocalLogSystem());

    WFileReader reader;
    if (reader.Open(sIgnoreFilePath).Failed())
    {
      WLog::Error("Failed to open ignore file {0}", sIgnoreFilePath);
      return W_FAILURE;
    }

    if (jsonReader.Parse(reader).Failed())
      return W_FAILURE;

    const WStringView includeTarget = "includeTarget";
    const WStringView includeSource = "includeSource";
    const WStringView byName = "byName";

    if (jsonReader.GetTopLevelElementType() != WJSONReader::ElementType::Dictionary)
    {
      WLog::Error("Ignore file {0} does not start with a json object", sIgnoreFilePath);
      return W_FAILURE;
    }

    auto topLevel = jsonReader.GetTopLevelObject();
    for (auto it = topLevel.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Key() == includeTarget || it.Key() == includeSource)
      {
        IgnoreInfo& info = (it.Key() == includeTarget) ? m_IgnoreTarget : m_IgnoreSource;
        auto inner = it.Value().Get<WVariantDictionary>();
        for (auto it2 = inner.GetIterator(); it2.IsValid(); it2.Next())
        {
          if (it2.Key() == byName)
          {
            if (ParseArray(it2.Value(), info.m_byName).Failed())
            {
              WLog::Error("Failed to parse value of '{0}.{1}'.", it.Key(), it2.Key());
              return W_FAILURE;
            }
          }
          else
          {
            WLog::Error("Unknown field of '{0}.{1}'", it.Key(), it2.Key());
            return W_FAILURE;
          }
        }
      }
      else
      {
        WLog::Error("Unknown json member in root object '{0}'", it.Key().GetView());
        return W_FAILURE;
      }
    }
    return W_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
    WGlobalLog::AddLogWriter(LogInspector);

    constexpr WUInt32 uiInitalAllocatorSize = 1024 * 1024;
    m_pLinearAllocator = W_DEFAULT_NEW(WLinearAllocator<WAllocatorTrackingMode::Nothing>, "Temp Allocator", WFoundation::GetAlignedAllocator(), uiInitalAllocatorSize);

    if (GetArgumentCount() < 2)
      WLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    WFileSystem::AddDataDirectory("", "App", ":", WDataDirUsage::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    WStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    auto shortInclude = WStringView("-i");
    auto longInclude = WStringView("--includeDir");
    auto shortIgnoreFile = WStringView("-f");
    auto longIgnoreFile = WStringView("--ignoreFile");
    for (WUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = WStringView(GetArgument(argi));
      if (arg == shortInclude || arg == longInclude)
      {
        if (numArgs <= argi + 1)
        {
          WLog::Error("Missing path for {0}", arg);
          return;
        }
        WStringBuilder includeDir = GetArgument(argi + 1);
        if (includeDir == shortInclude || includeDir == longInclude || includeDir == shortIgnoreFile || includeDir == longIgnoreFile)
        {
          WLog::Error("Missing path for {0} found {1} instead", arg, includeDir.GetView());
          return;
        }
        argi++;
        includeDir.MakeCleanPath();
        m_IncludeDirectories.PushBack(includeDir);
      }
      else if (arg == shortIgnoreFile || arg == longIgnoreFile)
      {
        if (numArgs <= argi + 1)
        {
          WLog::Error("Missing path for {0}", arg);
          return;
        }
        WStringBuilder ignoreFile = GetArgument(argi + 1);
        if (ignoreFile == shortInclude || ignoreFile == longInclude || ignoreFile == shortIgnoreFile || ignoreFile == longIgnoreFile)
        {
          WLog::Error("Missing path for {0} found {1} instead", arg, ignoreFile.GetView());
          return;
        }
        argi++;
        ignoreFile.MakeCleanPath();
        if (ParseIgnoreFile(ignoreFile.GetView()).Failed())
          return;
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          WLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir?");
        }
      }
    }

    if (!WPathUtils::IsAbsolutePath(sSearchDir.GetData()))
      WLog::Error("The given path is not absolute: '{0}'", sSearchDir);

    m_sSearchDir = sSearchDir;

    auto projectStart = m_sSearchDir.GetView().FindLastSubString("/");
    if (projectStart == nullptr)
    {
      WLog::Error("Failed to parse project name from search path {0}", sSearchDir);
      return;
    }
    WStringBuilder projectName = WStringView(projectStart + 1, m_sSearchDir.GetView().GetEndPointer());
    projectName.ToUpper();
    m_sProjectName = projectName;

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      WLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    m_pLinearAllocator = nullptr;

    WGlobalLog::RemoveLogWriter(LogInspector);
    WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
  }

  WResult ReadEntireFile(WStringView sFile, WStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    WFileReader File;
    if (File.Open(sFile) == W_FAILURE)
    {
      WLog::Error("Could not open for reading: '{0}'", sFile);
      return W_FAILURE;
    }

    WDynamicArray<WUInt8> FileContent;

    WUInt8 Temp[4024];
    WUInt64 uiRead = File.ReadBytes(Temp, W_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(WArrayPtr<WUInt8>(Temp, (WUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, W_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!WUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      WLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                   "an editor that does not save the file in Utf8 encoding.",
        sFile);
      return W_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    return W_SUCCESS;
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const WUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // get a directory iterator for the search directory
    WFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), WFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      WStringBuilder currentFile, sExt;

      // while there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // build the absolute path to the current file
        currentFile = it.GetCurrentPath();
        currentFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = currentFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h"))
        {
          WLog::Info("Checking: {}", currentFile);

          W_LOG_BLOCK("Header", &currentFile.GetData()[uiSearchDirLength]);
          CheckHeaderFile(currentFile);
          m_pLinearAllocator->Reset();
        }
      }
    }
    else
      WLog::Error("Could not search the directory '{0}'", m_sSearchDir);
  }

  void CheckInclude(const WStringBuilder& sCurrentFile, const WStringBuilder& sIncludePath, WUInt32 uiLine)
  {
    WStringBuilder absIncludePath(m_pLinearAllocator.Borrow());
    bool includeOutside = true;
    if (sIncludePath.IsAbsolutePath())
    {
      for (auto& includeDir : m_IncludeDirectories)
      {
        if (sIncludePath.StartsWith(includeDir))
        {
          includeOutside = false;
          break;
        }
      }
    }
    else
    {
      bool includeFound = false;
      if (sIncludePath.StartsWith("ThirdParty"))
      {
        includeOutside = true;
      }
      else
      {
        for (auto& includeDir : m_IncludeDirectories)
        {
          absIncludePath = includeDir;
          absIncludePath.AppendPath(sIncludePath);
          if (WOSFile::ExistsFile(absIncludePath))
          {
            includeOutside = false;
            break;
          }
        }
      }
    }

    if (includeOutside)
    {
      WStringBuilder includeFileLower = sIncludePath.GetFileNameAndExtension();
      includeFileLower.ToLower();
      WStringBuilder currentFileLower = sCurrentFile.GetFileNameAndExtension();
      currentFileLower.ToLower();

      // ignore includes ending in "_Platform.h", they redirect to platform specific W headers
      const bool ignore = m_IgnoreTarget.m_byName.Contains(includeFileLower) || m_IgnoreSource.m_byName.Contains(currentFileLower) || includeFileLower.EndsWith_NoCase("_Platform.h");

      if (!ignore)
      {
        WLog::Error("Including '{0}' in {1}:{2} leaks underlying implementation details. Including system or thirdparty headers in public W header "
                     "files is not allowed. Please use an interface, factory or pimpl pattern to hide the implementation and avoid the include. See "
                     "the Documentation Chapter 'General->Header Files' for details.",
          sIncludePath.GetView(), sCurrentFile.GetView(), uiLine);
      }
    }
  }

  void CheckHeaderFile(const WStringBuilder& sCurrentFile)
  {
    WStringBuilder fileContents(m_pLinearAllocator.Borrow());
    ReadEntireFile(sCurrentFile.GetData(), fileContents).IgnoreResult();

    auto fileDir = sCurrentFile.GetFileDirectory();

    WStringBuilder internalMacroToken(m_pLinearAllocator.Borrow());
    internalMacroToken.Append("W_", m_sProjectName, "_INTERNAL_HEADER");
    auto internalMacroTokenView = internalMacroToken.GetView();

    WTokenizer tokenizer(m_pLinearAllocator.Borrow());
    auto dataView = fileContents.GetView();
    tokenizer.Tokenize(WArrayPtr<const WUInt8>(reinterpret_cast<const WUInt8*>(dataView.GetStartPointer()), dataView.GetElementCount()), WLog::GetThreadLocalLogSystem());

    WStringView hash("#");
    WStringView include("include");
    WStringView openAngleBracket("<");
    WStringView closeAngleBracket(">");

    bool isInternalHeader = false;
    auto tokens = tokenizer.GetTokens();
    const auto numTokens = tokens.GetCount();
    for (WUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == WTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == WTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == WTokenType::Whitespace);

        if (curToken.m_iType == WTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == WTokenType::Whitespace);

          if (curToken.m_iType == WTokenType::String1)
          {
            // #include "bla"
            WStringBuilder absIncludePath(m_pLinearAllocator.Borrow());
            WStringBuilder relativePath(m_pLinearAllocator.Borrow());
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!WOSFile::ExistsFile(absIncludePath))
            {
              WLog::Error("The file '{0}' does not exist. Includes relative to the global include directories should use the #include "
                           "<path/to/file.h> syntax.",
                absIncludePath);
            }
            else if (!isInternalHeader)
            {
              CheckInclude(sCurrentFile, absIncludePath, includeToken.m_uiLine);
            }
          }
          else if (curToken.m_iType == WTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == WTokenType::Newline)
              {
                WLog::Error("Non-terminated '<' in #include {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != WTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != WTokenType::Newline);
            }
            else if (!isInternalHeader)
            {
              WStringBuilder includePath(m_pLinearAllocator.Borrow());
              includePath = WStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              CheckInclude(sCurrentFile, includePath, startToken.m_uiLine);
            }
          }
          else
          {
            // error
            WLog::Error("Can not parse #include statement in {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != WTokenType::Newline && curToken.m_iType != WTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        if (curToken.m_iType == WTokenType::Identifier && curToken.m_DataView == internalMacroTokenView)
        {
          isInternalHeader = true;
        }
        else
        {
          while (curToken.m_iType != WTokenType::Newline && curToken.m_iType != WTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
    }
  }

  virtual void Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
    {
      QuitApplication();
      return;
    }

    IterateOverFiles();
    QuitApplication();
  }
};

W_APPLICATION_ENTRY_POINT(WHeaderCheckApp);
