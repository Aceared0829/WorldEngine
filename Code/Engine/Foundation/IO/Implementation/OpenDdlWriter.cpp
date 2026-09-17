#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

void WOpenDdlWriter::OutputEscapedString(const WStringView& string)
{
  m_sTemp = string;
  m_sTemp.ReplaceAll("\\", "\\\\");
  m_sTemp.ReplaceAll("\"", "\\\"");
  m_sTemp.ReplaceAll("\b", "\\b");
  m_sTemp.ReplaceAll("\r", "\\r");
  m_sTemp.ReplaceAll("\f", "\\f");
  m_sTemp.ReplaceAll("\n", "\\n");
  m_sTemp.ReplaceAll("\t", "\\t");

  OutputString("\"", 1);
  OutputString(m_sTemp.GetData());
  OutputString("\"", 1);
}

void WOpenDdlWriter::OutputIndentation()
{
  if (m_bCompactMode)
    return;

  WInt32 iIndentation = m_iIndentation;

  // I need my space!
  const char* szIndentation = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

  while (iIndentation >= 16)
  {
    OutputString(szIndentation, 16);
    iIndentation -= 16;
  }

  if (iIndentation > 0)
  {
    OutputString(szIndentation, iIndentation);
  }
}

void WOpenDdlWriter::OutputPrimitiveTypeNameCompliant(WOpenDdlPrimitiveType type)
{
  switch (type)
  {
    case WOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case WOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case WOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case WOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case WOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case WOpenDdlPrimitiveType::UInt8:
      OutputString("unsigned_int8", 13);
      break;
    case WOpenDdlPrimitiveType::UInt16:
      OutputString("unsigned_int16", 14);
      break;
    case WOpenDdlPrimitiveType::UInt32:
      OutputString("unsigned_int32", 14);
      break;
    case WOpenDdlPrimitiveType::UInt64:
      OutputString("unsigned_int64", 14);
      break;
    case WOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case WOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case WOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      W_REPORT_FAILURE("Unknown DDL primitive type {0}", (WUInt32)type);
      break;
  }
}
void WOpenDdlWriter::OutputPrimitiveTypeNameShort(WOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write uint8 etc. instead of unsigned_int

  switch (type)
  {
    case WOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case WOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case WOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case WOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case WOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case WOpenDdlPrimitiveType::UInt8:
      OutputString("uint8", 5);
      break;
    case WOpenDdlPrimitiveType::UInt16:
      OutputString("uint16", 6);
      break;
    case WOpenDdlPrimitiveType::UInt32:
      OutputString("uint32", 6);
      break;
    case WOpenDdlPrimitiveType::UInt64:
      OutputString("uint64", 6);
      break;
    case WOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case WOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case WOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      W_REPORT_FAILURE("Unknown DDL primitive type {0}", (WUInt32)type);
      break;
  }
}

void WOpenDdlWriter::OutputPrimitiveTypeNameShortest(WOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write super short type strings

  switch (type)
  {
    case WOpenDdlPrimitiveType::Bool:
      OutputString("b", 1);
      break;
    case WOpenDdlPrimitiveType::Int8:
      OutputString("i1", 2);
      break;
    case WOpenDdlPrimitiveType::Int16:
      OutputString("i2", 2);
      break;
    case WOpenDdlPrimitiveType::Int32:
      OutputString("i3", 2);
      break;
    case WOpenDdlPrimitiveType::Int64:
      OutputString("i4", 2);
      break;
    case WOpenDdlPrimitiveType::UInt8:
      OutputString("u1", 2);
      break;
    case WOpenDdlPrimitiveType::UInt16:
      OutputString("u2", 2);
      break;
    case WOpenDdlPrimitiveType::UInt32:
      OutputString("u3", 2);
      break;
    case WOpenDdlPrimitiveType::UInt64:
      OutputString("u4", 2);
      break;
    case WOpenDdlPrimitiveType::Float:
      OutputString("f", 1);
      break;
    case WOpenDdlPrimitiveType::Double:
      OutputString("d", 1);
      break;
    case WOpenDdlPrimitiveType::String:
      OutputString("s", 1);
      break;

    default:
      W_REPORT_FAILURE("Unknown DDL primitive type {0}", (WUInt32)type);
      break;
  }
}

WOpenDdlWriter::WOpenDdlWriter()
{
  static_assert((int)WOpenDdlWriter::State::PrimitivesBool == (int)WOpenDdlPrimitiveType::Bool);
  static_assert((int)WOpenDdlWriter::State::PrimitivesInt8 == (int)WOpenDdlPrimitiveType::Int8);
  static_assert((int)WOpenDdlWriter::State::PrimitivesInt16 == (int)WOpenDdlPrimitiveType::Int16);
  static_assert((int)WOpenDdlWriter::State::PrimitivesInt32 == (int)WOpenDdlPrimitiveType::Int32);
  static_assert((int)WOpenDdlWriter::State::PrimitivesInt64 == (int)WOpenDdlPrimitiveType::Int64);
  static_assert((int)WOpenDdlWriter::State::PrimitivesUInt8 == (int)WOpenDdlPrimitiveType::UInt8);
  static_assert((int)WOpenDdlWriter::State::PrimitivesUInt16 == (int)WOpenDdlPrimitiveType::UInt16);
  static_assert((int)WOpenDdlWriter::State::PrimitivesUInt32 == (int)WOpenDdlPrimitiveType::UInt32);
  static_assert((int)WOpenDdlWriter::State::PrimitivesUInt64 == (int)WOpenDdlPrimitiveType::UInt64);
  static_assert((int)WOpenDdlWriter::State::PrimitivesFloat == (int)WOpenDdlPrimitiveType::Float);
  static_assert((int)WOpenDdlWriter::State::PrimitivesString == (int)WOpenDdlPrimitiveType::String);

  m_StateStack.ExpandAndGetRef().m_State = State::Invalid;
  m_StateStack.ExpandAndGetRef().m_State = State::Empty;
}

// All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
// LessIndentation,  ///< Saves some space by using less space for indentation
// NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
// NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
// None,             ///< No whitespace, not even newlines, is output. This should be used when DDL is used for data exchange, but probably not read
// by humans.

void WOpenDdlWriter::BeginObject(WStringView sType, WStringView sName /*= {}*/, bool bGlobalName /*= false*/, bool bSingleLine /*= false*/)
{
  {
    const auto state = m_StateStack.PeekBack().m_State;
    W_IGNORE_UNUSED(state);
    W_ASSERT_DEBUG(state == State::Empty || state == State::ObjectMultiLine || state == State::ObjectStart,
      "DDL Writer is in a state where no further objects may be created");
  }

  OutputObjectBeginning();

  {
    const auto state = m_StateStack.PeekBack().m_State;
    W_IGNORE_UNUSED(state);
    W_ASSERT_DEBUG(state != State::ObjectSingleLine, "Cannot put an object into another single-line object");
    W_ASSERT_DEBUG(state != State::ObjectStart, "Object beginning should have been written");
  }

  OutputIndentation();
  OutputString(sType);

  OutputObjectName(sName, bGlobalName);

  if (bSingleLine)
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectSingleLine;
  }
  else
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectMultiLine;
  }

  m_StateStack.ExpandAndGetRef().m_State = State::ObjectStart;
}


void WOpenDdlWriter::OutputObjectBeginning()
{
  if (m_StateStack.PeekBack().m_State != State::ObjectStart)
    return;

  m_StateStack.PopBack();

  const auto state = m_StateStack.PeekBack().m_State;

  if (state == State::ObjectSingleLine)
  {
    // if (m_bCompactMode)
    OutputString("{", 1); // more compact
    // else
    // OutputString(" { ", 3);
  }
  else if (state == State::ObjectMultiLine)
  {
    if (m_bCompactMode)
    {
      OutputString("{", 1);
    }
    else
    {
      OutputString("\n", 1);
      OutputIndentation();
      OutputString("{\n", 2);
    }
  }

  m_iIndentation++;
}

bool IsDdlIdentifierCharacter(WUInt32 uiByte);

void WOpenDdlWriter::OutputObjectName(WStringView sName, bool bGlobalName)
{
  if (!sName.IsEmpty())
  {
    // W_ASSERT_DEBUG(WStringUtils::FindSubString(szName, " ") == nullptr, "Spaces are not allowed in DDL object names: '{0}'", szName);


    /// \test This code path is untested
    bool bEscape = false;
    for (auto nameIt = sName.GetIteratorFront(); nameIt.IsValid(); ++nameIt)
    {
      if (!IsDdlIdentifierCharacter(nameIt.GetCharacter()))
      {
        bEscape = true;
        break;
      }
    }

    if (m_bCompactMode)
    {
      // even remove the whitespace between type and name

      if (bGlobalName)
        OutputString("$", 1);
      else
        OutputString("%", 1);
    }
    else
    {
      if (bGlobalName)
        OutputString(" $", 2);
      else
        OutputString(" %", 2);
    }

    if (bEscape)
      OutputString("\'", 1);

    OutputString(sName);

    if (bEscape)
      OutputString("\'", 1);
  }
}

void WOpenDdlWriter::EndObject()
{
  const auto state = m_StateStack.PeekBack().m_State;
  W_ASSERT_DEBUG(state == State::ObjectSingleLine || state == State::ObjectMultiLine || state == State::ObjectStart, "No object is open");

  if (state == State::ObjectStart)
  {
    // object is empty

    OutputString("{}\n", 3);
    m_StateStack.PopBack();

    const auto newState = m_StateStack.PeekBack().m_State;
    W_IGNORE_UNUSED(newState);
    W_ASSERT_DEBUG(newState == State::ObjectSingleLine || newState == State::ObjectMultiLine, "No object is open");
  }
  else
  {
    m_iIndentation--;

    if (m_bCompactMode)
      OutputString("}", 1);
    else
    {
      if (state == State::ObjectMultiLine)
      {
        OutputIndentation();
        OutputString("}\n", 2);
      }
      else
      {
        // OutputString(" }\n", 3);
        OutputString("}\n", 2); // more compact
      }
    }
  }

  m_StateStack.PopBack();
}

void WOpenDdlWriter::BeginPrimitiveList(WOpenDdlPrimitiveType type, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  OutputObjectBeginning();

  const auto state = m_StateStack.PeekBack().m_State;
  W_ASSERT_DEBUG(state == State::Empty || state == State::ObjectSingleLine || state == State::ObjectMultiLine,
    "DDL Writer is in a state where no primitive list may be created");

  if (state == State::ObjectMultiLine)
  {
    OutputIndentation();
  }

  if (m_TypeStringMode == TypeStringMode::Shortest)
    OutputPrimitiveTypeNameShortest(type);
  else if (m_TypeStringMode == TypeStringMode::ShortenedUnsignedInt)
    OutputPrimitiveTypeNameShort(type);
  else
    OutputPrimitiveTypeNameCompliant(type);

  OutputObjectName(sName, bGlobalName);

  // more compact
  // if (m_bCompactMode)
  OutputString("{", 1);
  // else
  // OutputString(" {", 2);

  m_StateStack.ExpandAndGetRef().m_State = static_cast<State>(type);
}

void WOpenDdlWriter::EndPrimitiveList()
{
  const auto state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEBUG(state >= State::PrimitivesBool && state <= State::PrimitivesString, "No primitive list is open");

  m_StateStack.PopBack();

  if (m_bCompactMode)
    OutputString("}", 1);
  else
  {
    if (m_StateStack.PeekBack().m_State == State::ObjectSingleLine)
      OutputString("}", 1);
    else
      OutputString("}\n", 2);
  }
}

void WOpenDdlWriter::WritePrimitiveType(WOpenDdlWriter::State exp)
{
  W_IGNORE_UNUSED(exp);

  auto& state = m_StateStack.PeekBack();
  W_ASSERT_DEBUG(state.m_State == exp, "Cannot write thie primitive type without have the correct primitive list open");

  if (state.m_bPrimitivesWritten)
  {
    // already wrote some primitives, so append a comma
    OutputString(",", 1);
  }

  state.m_bPrimitivesWritten = true;
}


void WOpenDdlWriter::WriteBinaryAsHex(const void* pData, WUInt32 uiBytes)
{
  char tmp[4];

  WUInt8* pBytes = (WUInt8*)pData;

  for (WUInt32 i = 0; i < uiBytes; ++i)
  {
    WStringUtils::snprintf(tmp, 4, "%02X", (WUInt32)*pBytes);
    ++pBytes;

    OutputString(tmp, 2);
  }
}

void WOpenDdlWriter::WriteBool(const bool* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesBool);

  if (m_bCompactMode || m_TypeStringMode == TypeStringMode::Shortest)
  {
    // Extension to OpenDDL: We write only '1' or '0' in compact mode

    if (pValues[0])
      OutputString("1", 1);
    else
      OutputString("0", 1);

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",1", 2);
      else
        OutputString(",0", 2);
    }
  }
  else
  {
    if (pValues[0])
      OutputString("true", 4);
    else
      OutputString("false", 5);

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",true", 5);
      else
        OutputString(",false", 6);
    }
  }
}

void WOpenDdlWriter::WriteInt8(const WInt8* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt8);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteInt16(const WInt16* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt16);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteInt32(const WInt32* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt32);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteInt64(const WInt64* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt64);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}


void WOpenDdlWriter::WriteUInt8(const WUInt8* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt8);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteUInt16(const WUInt16* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt16);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteUInt32(const WUInt32* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt32);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteUInt64(const WUInt64* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt64);

  m_sTemp.SetFormat("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (WUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.SetFormat(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void WOpenDdlWriter::WriteFloat(const float* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesFloat);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.SetFormat("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.SetFormat(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 4);
    }

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 4);
      }
    }
  }
}

void WOpenDdlWriter::WriteDouble(const double* pValues, WUInt32 uiCount /*= 1*/)
{
  W_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  W_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesDouble);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.SetFormat("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.SetFormat(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 8);
    }

    for (WUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 8);
      }
    }
  }
}

void WOpenDdlWriter::WriteString(const WStringView& sString)
{
  WritePrimitiveType(State::PrimitivesString);

  OutputEscapedString(sString);
}

void WOpenDdlWriter::WriteBinaryAsString(const void* pData, WUInt32 uiBytes)
{
  /// \test WOpenDdlWriter::WriteBinaryAsString

  WritePrimitiveType(State::PrimitivesString);

  OutputString("\"", 1);
  WriteBinaryAsHex(pData, uiBytes);
  OutputString("\"", 1);
}
