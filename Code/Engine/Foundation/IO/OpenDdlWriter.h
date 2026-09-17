#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

// TODO
// Write primitives in HEX (esp. float)

/// Writes OpenDDL documents to a stream with configurable formatting
///
/// OpenDDL (Open Data Description Language) is a text format for describing structured data.
/// This writer generates OpenDDL text from programmatically created object hierarchies and primitive data.
/// Supports various output modes including compact/verbose formatting, different type name styles,
/// and precise floating-point representation. Objects and primitive lists can be nested arbitrarily.
/// Call SetOutputStream() first, then use BeginObject/EndObject and BeginPrimitiveList/EndPrimitiveList pairs.
class W_FOUNDATION_DLL WOpenDdlWriter
{
public:
  /// Controls how primitive type names are written in the output
  enum class TypeStringMode
  {
    Compliant,            ///< All primitive types are written as the OpenDDL standard defines them (very verbose)
    ShortenedUnsignedInt, ///< unsigned_intX is shortened to uintX
    Shortest              ///< All primitive type names are shortened to one or two characters: i1, i2, i3, i4, u1, u2, u3, u4, b, s, f, d (int, uint, bool,
                          ///< string, float, double)
  };

  /// Controls how floating-point values are formatted in the output
  enum class FloatPrecisionMode
  {
    Readable, ///< Float values are printed as readable numbers. Precision might get lost though.
    Exact,    ///< Float values are printed as HEX, representing the exact binary data.
  };

  /// Constructor
  WOpenDdlWriter();

  virtual ~WOpenDdlWriter() = default;

  /// All output is written to this binary stream.
  void SetOutputStream(WStreamWriter* pOutput) { m_pOutput = pOutput; } // [tested]

  /// Configures how much whitespace is output.
  void SetCompactMode(bool bCompact) { m_bCompactMode = bCompact; } // [tested]

  /// Configures how verbose the type strings are going to be written.
  void SetPrimitiveTypeStringMode(TypeStringMode mode) { m_TypeStringMode = mode; }

  /// Configures how float values are output.
  void SetFloatPrecisionMode(FloatPrecisionMode mode) { m_FloatPrecisionMode = mode; }

  /// Returns how float values are output.
  FloatPrecisionMode GetFloatPrecisionMode() const { return m_FloatPrecisionMode; }

  /// Sets the base indentation level for output formatting
  ///
  /// Negative values are allowed to delay indentation until deeper nesting levels.
  /// For example, setting -2 means indentation only starts at nesting level 3.
  void SetIndentation(WInt8 iIndentation) { m_iIndentation = iIndentation; }

  /// Begins outputting an object.
  void BeginObject(WStringView sType, WStringView sName = {}, bool bGlobalName = false, bool bSingleLine = false); // [tested]

  /// Ends outputting an object.
  void EndObject(); // [tested]

  /// Begins outputting a list of primitives of the given type.
  void BeginPrimitiveList(WOpenDdlPrimitiveType type, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Ends outputting the list of primitives.
  void EndPrimitiveList(); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteBool(const bool* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt8(const WInt8* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt16(const WInt16* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt32(const WInt32* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt64(const WInt64* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt8(const WUInt8* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt16(const WUInt16* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt32(const WUInt32* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt64(const WUInt64* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteFloat(const float* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteDouble(const double* pValues, WUInt32 uiCount = 1); // [tested]

  /// Writes a single string to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteString(const WStringView& sString); // [tested]

  /// Writes binary data as a hexadecimal string to the primitive list
  ///
  /// Converts the binary data to a hex string representation and writes it as a string primitive.
  /// Useful for embedding binary data within OpenDDL text format.
  void WriteBinaryAsString(const void* pData, WUInt32 uiBytes);


protected:
  enum State
  {
    Invalid = -5,
    Empty = -4,
    ObjectSingleLine = -3,
    ObjectMultiLine = -2,
    ObjectStart = -1,
    PrimitivesBool = 0, // same values as in WOpenDdlPrimitiveType to enable casting
    PrimitivesInt8,
    PrimitivesInt16,
    PrimitivesInt32,
    PrimitivesInt64,
    PrimitivesUInt8,
    PrimitivesUInt16,
    PrimitivesUInt32,
    PrimitivesUInt64,
    PrimitivesFloat,
    PrimitivesDouble,
    PrimitivesString,
  };

  struct DdlState
  {
    State m_State = State::Empty;
    bool m_bPrimitivesWritten = false;
  };

  W_ALWAYS_INLINE void OutputString(WStringView s) { m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).IgnoreResult(); }
  W_ALWAYS_INLINE void OutputString(WStringView s, WUInt32 uiElementCount) { m_pOutput->WriteBytes(s.GetStartPointer(), uiElementCount).IgnoreResult(); }
  void OutputEscapedString(const WStringView& string);
  void OutputIndentation();
  void OutputPrimitiveTypeNameCompliant(WOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShort(WOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShortest(WOpenDdlPrimitiveType type);
  void WritePrimitiveType(WOpenDdlWriter::State exp);
  void OutputObjectName(WStringView sName, bool bGlobalName);
  void WriteBinaryAsHex(const void* pData, WUInt32 uiBytes);
  void OutputObjectBeginning();

  WInt32 m_iIndentation = 0;
  bool m_bCompactMode = false;
  TypeStringMode m_TypeStringMode = TypeStringMode::ShortenedUnsignedInt;
  FloatPrecisionMode m_FloatPrecisionMode = FloatPrecisionMode::Exact;
  WStreamWriter* m_pOutput = nullptr;
  WStringBuilder m_sTemp;

  WHybridArray<DdlState, 16> m_StateStack;
};
