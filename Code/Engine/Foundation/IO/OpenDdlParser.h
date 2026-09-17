#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>

class WLogInterface;

/// The primitive data types that OpenDDL supports
enum class WOpenDdlPrimitiveType
{
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  // Half, // Currently not supported
  Float,
  Double,
  String,
  // Ref, // Currently not supported
  // Type // Currently not supported
  Custom
};

/// Low-level streaming parser for OpenDDL documents
///
/// This abstract base class provides incremental parsing of OpenDDL (Open Data Description Language) format.
/// Unlike WOpenDdlReader which builds a complete in-memory tree, this parser operates in streaming mode,
/// calling virtual functions as elements are encountered. This allows processing large documents with minimal
/// memory usage and enables selective parsing where only certain parts are processed.
/// Derived classes must override the virtual On* methods to handle parsed elements.
class W_FOUNDATION_DLL WOpenDdlParser
{
public:
  WOpenDdlParser();
  virtual ~WOpenDdlParser() = default;

  /// Whether an error occurred during parsing that resulted in cancellation of further parsing.
  bool HadFatalParsingError() const { return m_bHadFatalParsingError; } // [tested]

protected:
  /// Sets an WLogInterface through which errors and warnings are reported.
  void SetLogInterface(WLogInterface* pLog) { m_pLogInterface = pLog; }

  /// Sets the internal cache size for batching primitive data callbacks
  ///
  /// Data is returned in larger chunks to reduce the number of function calls. The cache size determines
  /// the maximum chunk size per primitive type. Default cache size is 4 KB, allowing up to 1000 integers
  /// or 500 doubles per chunk. Increasing the cache size only helps when the input data contains large
  /// primitive arrays, otherwise it provides no benefit.
  void SetCacheSize(WUInt32 uiSizeInKB);

  /// Configures the parser to read from the given stream. This can only be called once on a parser instance.
  void SetInputStream(WStreamReader& stream, WUInt32 uiFirstLineOffset = 0); // [tested]

  /// Parses the next portion of the document and triggers appropriate callbacks
  ///
  /// Returns false when the end of the document has been reached or a fatal parsing error occurred.
  /// Use this for incremental parsing where you want to control when parsing happens.
  bool ContinueParsing(); // [tested]

  /// Calls ContinueParsing() in a loop until that returns false.
  WResult ParseAll(); // [tested]

  /// Skips the rest of the currently open object. No OnEndObject() call will be done for this object either.
  void SkipRestOfObject();

  /// Can be used to prevent parsing the rest of the document.
  void StopParsing();

  /// Outputs that a parsing error was detected (via OnParsingError) and stops further parsing, if bFatal is set to true.
  void ParsingError(WStringView sMessage, bool bFatal);

  WLogInterface* m_pLogInterface;

protected:
  /// Called when something unexpected is encountered in the document.
  ///
  /// The error message describes what was expected and what was encountered.
  /// If bFatal is true, the error has left the parser in an unrecoverable state and thus it will not continue parsing.
  /// In that case client code will need to clean up it's open state, as no further callbacks will be called.
  /// If bFatal is false, the document is not entirely valid, but the parser is still able to continue.
  virtual void OnParsingError(WStringView sMessage, bool bFatal, WUInt32 uiLine, WUInt32 uiColumn)
  {
    W_IGNORE_UNUSED(sMessage);
    W_IGNORE_UNUSED(bFatal);
    W_IGNORE_UNUSED(uiLine);
    W_IGNORE_UNUSED(uiColumn);
  }

  /// Called when a new object is encountered.
  virtual void OnBeginObject(WStringView sType, WStringView sName, bool bGlobalName) = 0;

  /// Called when the end of an object is encountered.
  virtual void OnEndObject() = 0;

  /// Called when a new primitive object is encountered.
  virtual void OnBeginPrimitiveList(WOpenDdlPrimitiveType type, WStringView sName, bool bGlobalName) = 0;

  /// Called when the end of a primitive object is encountered.
  virtual void OnEndPrimitiveList() = 0;

  /// \todo Currently not supported
  // virtual void OnBeginPrimitiveArrayList(WOpenDdlPrimitiveType type, WUInt32 uiGroupSize) = 0;
  // virtual void OnEndPrimitiveArrayList() = 0;

  /// Called when boolean primitive data is available
  ///
  /// Multiple values may be reported at once for efficiency. bThisIsAll indicates if this is the final batch
  /// for the current primitive list. Implementations should accumulate data if bThisIsAll is false.
  virtual void OnPrimitiveBool(WUInt32 count, const bool* pData, bool bThisIsAll) = 0;

  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt8(WUInt32 count, const WInt8* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt16(WUInt32 count, const WInt16* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt32(WUInt32 count, const WInt32* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt64(WUInt32 count, const WInt64* pData, bool bThisIsAll) = 0;

  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt8(WUInt32 count, const WUInt8* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt16(WUInt32 count, const WUInt16* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt32(WUInt32 count, const WUInt32* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt64(WUInt32 count, const WUInt64* pData, bool bThisIsAll) = 0;

  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveFloat(WUInt32 count, const float* pData, bool bThisIsAll) = 0;
  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveDouble(WUInt32 count, const double* pData, bool bThisIsAll) = 0;

  /// Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveString(WUInt32 count, const WStringView* pData, bool bThisIsAll) = 0;

private:
  enum State
  {
    Finished,
    Idle,
    ReadingBool,
    ReadingInt8,
    ReadingInt16,
    ReadingInt32,
    ReadingInt64,
    ReadingUInt8,
    ReadingUInt16,
    ReadingUInt32,
    ReadingUInt64,
    ReadingFloat,
    ReadingDouble,
    ReadingString,
  };

  struct DdlState
  {
    DdlState()
      : m_State(Idle)
    {
    }
    DdlState(State s)
      : m_State(s)
    {
    }

    State m_State;
  };

  void ReadNextByte();
  bool ReadCharacter();
  bool ReadCharacterSkipComments();
  void SkipWhitespace();
  void ContinueIdle();
  void ReadIdentifier(WUInt8* szString, WUInt32& count);
  void ReadString();
  void ReadWord();
  WUInt64 ReadDecimalLiteral();
  void PurgeCachedPrimitives(bool bThisIsAll);
  bool ContinuePrimitiveList();
  void ContinueString();
  void SkipString();
  void ContinueBool();
  void ContinueInt();
  void ContinueFloat();

  void ReadDecimalFloat();
  void ReadHexString();

  WHybridArray<DdlState, 32> m_StateStack;
  WStreamReader* m_pInput;
  WDynamicArray<WUInt8> m_Cache;

  static constexpr WUInt32 s_uiMaxIdentifierLength = 64;

  WUInt8 m_uiCurByte;
  WUInt8 m_uiNextByte;
  WUInt32 m_uiCurLine;
  WUInt32 m_uiCurColumn;
  bool m_bSkippingMode;
  bool m_bHadFatalParsingError;
  WUInt8 m_szIdentifierType[s_uiMaxIdentifierLength];
  WUInt8 m_szIdentifierName[s_uiMaxIdentifierLength];
  WDynamicArray<WUInt8> m_TempString;
  WUInt32 m_uiTempStringLength;

  WUInt32 m_uiNumCachedPrimitives;
  bool* m_pBoolCache;
  WInt8* m_pInt8Cache;
  WInt16* m_pInt16Cache;
  WInt32* m_pInt32Cache;
  WInt64* m_pInt64Cache;
  WUInt8* m_pUInt8Cache;
  WUInt16* m_pUInt16Cache;
  WUInt32* m_pUInt32Cache;
  WUInt64* m_pUInt64Cache;
  float* m_pFloatCache;
  double* m_pDoubleCache;
};
