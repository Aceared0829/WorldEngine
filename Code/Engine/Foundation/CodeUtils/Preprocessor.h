#pragma once

#include <Foundation/Basics.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Timestamp.h>

/// This object caches files in a tokenized state. It can be shared among WPreprocessor instances to improve performance when
/// they access the same files.
class W_FOUNDATION_DLL WTokenizedFileCache
{
public:
  struct FileData
  {
    WTokenizer m_Tokens;
    WTimestamp m_Timestamp;
  };

  /// Checks whether \a sFileName is already in the cache, returns an iterator to it. If the iterator is invalid, the file is not cached yet.
  WMap<WString, FileData>::ConstIterator Lookup(const WString& sFileName) const;

  /// Removes the cached content for \a sFileName from the cache. Should be used when the file content has changed and needs to be re-read.
  void Remove(const WString& sFileName);

  /// Removes all files from the cache to ensure that they will be re-read.
  void Clear();

  /// Stores \a FileContent for the file \a sFileName as the new cached data.
  ///
  //// The file content is tokenized first and all #line directives are evaluated, to update the line number and file origin for each token.
  /// Any errors are written to the given log.
  const WTokenizer* Tokenize(const WString& sFileName, WArrayPtr<const WUInt8> fileContent, const WTimestamp& fileTimeStamp, WLogInterface* pLog);

private:
  void SkipWhitespace(WDeque<WToken>& Tokens, WUInt32& uiCurToken);

  mutable WMutex m_Mutex;
  WMap<WString, FileData> m_Cache;
};

/// WPreprocessor implements a standard C preprocessor. It can be used to pre-process files to get the output after macro expansion and #ifdef
/// handling.
///
/// For a detailed documentation about the C preprocessor, see https://gcc.gnu.org/onlinedocs/cpp/
///
/// This class implements all standard features:
///   * object and function macros
///   * Full evaluation of #if, #ifdef etc. including mathematical operations such as #if A > 42
///   * Parameter stringification
///   * Parameter concatenation
///   * __LINE__ and __FILE__ macros
///   * Fully correct #line evaluation for error output
///   * Correct handling of __VA_ARGS__
///   * #include handling
///   * #pragma once
///   * #warning and #error for custom failure messages
class W_FOUNDATION_DLL WPreprocessor
{
public:
  /// Describes the type of #include that was encountered during preprocessing
  enum IncludeType
  {
    MainFile,        ///< This is used for the very first access to the main source file
    RelativeInclude, ///< An #include "file" has been encountered
    GlobalInclude    ///< An #include <file> has been encountered
  };

  /// This type of callback is used to read an #include file. \a sAbsoluteFile is the path that the FileLocatorCB reported, the result needs
  /// to be stored in \a FileContent.
  using FileOpenCB = WDelegate<WResult(WStringView, WDynamicArray<WUInt8>&, WTimestamp&)>;

  /// This type of callback is used to retrieve the absolute path of the \a sIncludeFile when #included inside \a sCurAbsoluteFile.
  ///
  /// Note that you should ensure that \a out_sAbsoluteFilePath is always identical (including casing and path slashes) when it is supposed to point
  /// to the same file, as this exact name is used for file lookup (and therefore also file caching).
  /// If it is not identical, file caching will not work, and on different OSes the file may be found or not.
  using FileLocatorCB = WDelegate<WResult(WStringView, WStringView, IncludeType, WStringBuilder&)>;

  /// Every time an unknown command (e.g. '#version') is encountered, this callback is used to determine whether the command shall be passed
  /// through.
  ///
  /// If the callback returns false, an error is generated and parsing fails. The callback thus acts as a whitelist for all commands that shall be
  /// passed through.
  using PassThroughUnknownCmdCB = WDelegate<bool(WStringView)>;

  using MacroParameters = WDeque<WTokenParseUtils::TokenStream>;

  /// The event data that the processor broadcasts
  ///
  /// Please note that m_pToken contains a lot of interesting information, such as
  /// the current file and line number and of course the current piece of text.
  struct ProcessingEvent
  {
    /// The event types that the processor broadcasts
    enum EventType
    {
      BeginExpansion,  ///< A macro is now going to be expanded
      EndExpansion,    ///< A macro is finished being expanded
      Error,           ///< An error was encountered
      Warning,         ///< A warning has been output.
      CheckDefined,    ///< A 'defined(X)' is being evaluated
      CheckIfdef,      ///< A '#ifdef X' is being evaluated
      CheckIfndef,     ///< A '#ifndef X' is being evaluated
      EvaluateUnknown, ///< Inside an #if an unknown identifier has been encountered, it will be evaluated as zero
      Define,          ///< A #define X has been stored
      Redefine,        ///< A #define for an already existing macro name (also logged as a warning)
    };

    EventType m_Type = EventType::Error;

    const WToken* m_pToken = nullptr;
    WStringView m_sInfo;
  };

  /// Broadcasts events during the processing. This can be used to create detailed callstacks when an error is encountered.
  /// It also broadcasts errors and warnings with more detailed information than the log interface allows.
  WEvent<const ProcessingEvent&> m_ProcessingEvents;

  WPreprocessor();

  /// All error output is sent to the given WLogInterface.
  ///
  /// Note that when the preprocessor encounters any error, it will stop immediately and usually no output is generated.
  /// However, there are also a few cases where only a warning is generated, in this case preprocessing will continue without problems.
  ///
  /// Additionally errors and warnings are also broadcast through m_ProcessingEvents. So if you want to output more detailed information,
  /// that method should be preferred, because the events carry more information about the current file and line number etc.
  void SetLogInterface(WLogInterface* pLog);

  /// Allows to specify a custom cache object that should be used for storing the tokenized result of files.
  ///
  /// This allows to share one cache across multiple instances of WPreprocessor and across time. E.g. it makes it possible
  /// to prevent having to read and tokenize include files that are referenced often.
  void SetCustomFileCache(WTokenizedFileCache* pFileCache = nullptr);

  /// If set to true, all #pragma commands are passed through to the output, otherwise they are removed.
  void SetPassThroughPragma(bool bPassThrough) { m_bPassThroughPragma = bPassThrough; }

  /// If set to true, all #line commands are passed through to the output, otherwise they are removed.
  void SetPassThroughLine(bool bPassThrough) { m_bPassThroughLine = bPassThrough; }

  /// If set to true, all files are treated as if they contain a '#pragma once' directive, ie they will never be #included twice.
  void SetImplicitPragmaOnce(bool bEnable) { m_bImplicitPragmaOnce = bEnable; }

  /// Sets the callback that is used to determine whether an unknown command is passed through or triggers an error.
  void SetPassThroughUnknownCmdsCB(PassThroughUnknownCmdCB callback) { m_PassThroughUnknownCmdCB = callback; }

  /// Sets the callback that is needed to read input data.
  ///
  /// The default file open function will just try to open files via WFileReader.
  void SetFileOpenFunction(FileOpenCB openAbsFileCB);

  /// Sets the callback that is needed to locate an input file
  ///
  /// The default file locator will assume that the main source file and all files #included in angle brackets can be opened without modification.
  /// Files #included in "" will be appended as relative paths to the path of the file they appeared in.
  void SetFileLocatorFunction(FileLocatorCB locateAbsFileCB);

  /// Adds a #define to the preprocessor, even before any file is processed.
  ///
  /// This allows to have global macros that are always defined for all processed files, such as the current platform etc.
  /// \a sDefinition must be in the form of the text that follows a #define statement. So to define the macro "WIN32", just
  /// pass that string. You can define any macro that could also be defined in the source files.
  ///
  /// If the definition is invalid, W_FAILURE is returned. Also the preprocessor might end up in an invalid state, so using it any
  /// further might fail (including crashing).
  WResult AddCustomDefine(WStringView sDefinition);

  /// Processes the given file and returns the result as a stream of tokens.
  ///
  /// This function is useful when you want to further process the output afterwards and thus need it in a tokenized form anyway.
  WResult Process(WStringView sMainFile, WTokenParseUtils::TokenStream& ref_tokenOutput);

  /// Processes the given file and returns the result as a string.
  ///
  /// This function creates a string from the tokenized result. If \a bKeepComments is true, all block and line comments
  /// are included in the output string, otherwise they are removed.
  WResult Process(WStringView sMainFile, WStringBuilder& ref_sOutput, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);


private:
  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
    }

    WHashedString m_sVirtualFileName;
    WHashedString m_sFileName;
    WInt32 m_iCurrentLine;
    WInt32 m_iExpandDepth;
  };

  enum IfDefActivity
  {
    IsActive,
    IsInactive,
    WasActive,
  };

  struct CustomDefine
  {
    WHybridArray<WUInt8, 64> m_Content;
    WTokenizer m_Tokenized;
  };

  // This class-local allocator is used to get rid of some of the memory allocation
  // tracking that would otherwise occur for allocations made by the preprocessor.
  // If changing its position in the class, make sure it always comes before all
  // other members that depend on it to ensure deallocations in those members
  // happen before the allocator get destroyed.
  WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::Nothing> m_ClassAllocator;

  bool m_bPassThroughPragma = false;
  bool m_bPassThroughLine = false;
  bool m_bImplicitPragmaOnce = false; // all files will be treated as if they contain a #pragma once directive
  PassThroughUnknownCmdCB m_PassThroughUnknownCmdCB;

  // this file cache is used as long as the user does not provide his own
  WTokenizedFileCache m_InternalFileCache;

  // pointer to the file cache that is in use
  WTokenizedFileCache* m_pUsedFileCache;

  WDeque<FileData> m_CurrentFileStack;

  WLogInterface* m_pLog;

  WDeque<CustomDefine> m_CustomDefines;

  struct IfDefState
  {
    IfDefState(IfDefActivity activeState = IfDefActivity::IsActive)
      : m_ActiveState(activeState)

    {
    }

    IfDefActivity m_ActiveState;
    bool m_bIsInElseClause = false;
  };

  WDeque<IfDefState> m_IfdefActiveStack;

  WResult ProcessFile(WStringView sFile, WTokenParseUtils::TokenStream& TokenOutput, const WToken* pCurParentToken);
  WResult ProcessCmd(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& TokenOutput);

public:
  static WResult DefaultFileLocator(WStringView sCurAbsoluteFile, WStringView sIncludeFile, WPreprocessor::IncludeType incType, WStringBuilder& out_sAbsoluteFilePath);
  static WResult DefaultFileOpen(WStringView sAbsoluteFile, WDynamicArray<WUInt8>& ref_fileContent, WTimestamp& out_fileModification);

private: // *** File Handling ***
  WResult OpenFile(WStringView sFile, const WTokenizer** pTokenizer);

  FileOpenCB m_FileOpenCallback;
  FileLocatorCB m_FileLocatorCallback;
  WSet<WTempHashedString> m_PragmaOnce;

private: // *** Macro Definition ***
  bool RemoveDefine(WStringView sName);
  WResult HandleDefine(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken);

  struct MacroDefinition
  {
    const WToken* m_MacroIdentifier = nullptr;
    bool m_bIsFunction = false;
    bool m_bCurrentlyExpanding = false;
    bool m_bHasVarArgs = false;
    WUInt32 m_uiNumParameters = WInvalidIndex;
    WTokenParseUtils::TokenStream m_Replacement;
  };

  WResult StoreDefine(const WToken* pMacroNameToken, const WTokenParseUtils::TokenStream* pReplacementTokens, WUInt32 uiFirstReplacementToken, WInt32 iNumParameters, bool bUsesVarArgs);
  WResult ExtractParameterName(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WString& sIdentifierName);

  WMap<WString256, MacroDefinition> m_Macros;

  static constexpr WInt32 s_iMacroParameter0 = WTokenType::ENUM_COUNT + 2;
  static WString s_ParamNames[32];
  WToken m_ParameterTokens[32];

private: // *** #if condition parsing ***
  WResult EvaluateCondition(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseCondition(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseFactor(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionMul(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionOr(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionAnd(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionPlus(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionShift(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionBitOr(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionBitAnd(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);
  WResult ParseExpressionBitXor(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult);


private: // *** Parsing ***
  WResult CopyTokensAndEvaluateDefined(const WTokenParseUtils::TokenStream& Source, WUInt32 uiFirstSourceToken, WTokenParseUtils::TokenStream& Destination);
  void CopyTokensReplaceParams(const WTokenParseUtils::TokenStream& Source, WUInt32 uiFirstSourceToken, WTokenParseUtils::TokenStream& Destination, const WArrayPtr<WString>& parameters);

  WResult Expect(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WStringView sToken, WUInt32* pAccepted = nullptr);
  WResult Expect(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WTokenType::Enum Type, WUInt32* pAccepted = nullptr);
  WResult Expect(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WStringView sToken1, WStringView sToken2, WUInt32* pAccepted = nullptr);
  WResult ExpectEndOfLine(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken);

private: // *** Macro Expansion ***
  WResult Expand(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& Output);
  WResult ExpandOnce(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& Output);
  WResult ExpandObjectMacro(MacroDefinition& Macro, WTokenParseUtils::TokenStream& Output, const WToken* pMacroToken);
  WResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, WTokenParseUtils::TokenStream& Output, const WToken* pMacroToken);
  WResult ExpandMacroParam(const WToken& MacroToken, WUInt32 uiParam, WTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, WTokenParseUtils::TokenStream& Output);
  WToken* AddCustomToken(const WToken* pPrevious, const WStringView& sNewText);
  void OutputNotExpandableMacro(MacroDefinition& Macro, WTokenParseUtils::TokenStream& Output);
  WResult ExtractAllMacroParameters(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WDeque<WTokenParseUtils::TokenStream>& AllParameters);
  WResult ExtractParameterValue(const WTokenParseUtils::TokenStream& Tokens, WUInt32& uiCurToken, WTokenParseUtils::TokenStream& ParamTokens);

  WResult InsertParameters(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  WResult InsertStringifiedParameters(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  WResult ConcatenateParameters(const WTokenParseUtils::TokenStream& Tokens, WTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void MergeTokens(const WToken* pFirst, const WToken* pSecond, WTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  struct CustomToken
  {
    WToken m_Token;
    WString m_sIdentifierString;
  };

  enum TokenFlags : WUInt32
  {
    NoFurtherExpansion = W_BIT(0),
  };

  WToken m_TokenFile;
  WToken m_TokenLine;
  const WToken* m_pTokenOpenParenthesis;
  const WToken* m_pTokenClosedParenthesis;
  const WToken* m_pTokenComma;

  WDeque<const MacroParameters*> m_MacroParamStack;
  WDeque<const MacroParameters*> m_MacroParamStackExpanded;
  WDeque<CustomToken> m_CustomTokens;

private: // *** Other ***
  static void StringifyTokens(const WTokenParseUtils::TokenStream& Tokens, WStringBuilder& sResult, bool bSurroundWithQuotes);
  WToken* CreateStringifiedParameter(WUInt32 uiParam, const WToken* pParamToken, const MacroDefinition& Macro);

  WResult HandleErrorDirective(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleWarningDirective(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleUndef(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);

  WResult HandleEndif(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleElif(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleIf(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleElse(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken);
  WResult HandleIfdef(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken, bool bIsIfdef);
  WResult HandleInclude(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken, WTokenParseUtils::TokenStream& TokenOutput);
  WResult HandleLine(const WTokenParseUtils::TokenStream& Tokens, WUInt32 uiCurToken, WUInt32 uiDirectiveToken, WTokenParseUtils::TokenStream& TokenOutput);
};

#define PP_LOG0(Type, FormatStr, ErrorToken)                                                                                                        \
  {                                                                                                                                                 \
    ProcessingEvent pe;                                                                                                                             \
    pe.m_Type = ProcessingEvent::Type;                                                                                                              \
    pe.m_pToken = ErrorToken;                                                                                                                       \
    pe.m_sInfo = FormatStr;                                                                                                                         \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0)                                                                                 \
    {                                                                                                                                               \
      const_cast<WToken*>(pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                   \
      const_cast<WToken*>(pe.m_pToken)->m_File = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                                 \
    }                                                                                                                                               \
    m_ProcessingEvents.Broadcast(pe);                                                                                                               \
    WLog::Type(m_pLog, "File '{0}', Line {1} ({2}): " FormatStr, pe.m_pToken->m_File.GetString(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn); \
  }

#define PP_LOG(Type, FormatStr, ErrorToken, ...)                                                                                                       \
  {                                                                                                                                                    \
    ProcessingEvent _pe;                                                                                                                               \
    _pe.m_Type = ProcessingEvent::Type;                                                                                                                \
    _pe.m_pToken = ErrorToken;                                                                                                                         \
    if (_pe.m_pToken->m_uiLine == 0 && _pe.m_pToken->m_uiColumn == 0)                                                                                  \
    {                                                                                                                                                  \
      const_cast<WToken*>(_pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                     \
      const_cast<WToken*>(_pe.m_pToken)->m_File = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                                   \
    }                                                                                                                                                  \
    WStringBuilder sInfo;                                                                                                                             \
    sInfo.SetFormat(FormatStr, ##__VA_ARGS__);                                                                                                         \
    _pe.m_sInfo = sInfo;                                                                                                                               \
    m_ProcessingEvents.Broadcast(_pe);                                                                                                                 \
    WLog::Type(m_pLog, "File '{0}', Line {1} ({2}): {3}", _pe.m_pToken->m_File.GetString(), _pe.m_pToken->m_uiLine, _pe.m_pToken->m_uiColumn, sInfo); \
  }
