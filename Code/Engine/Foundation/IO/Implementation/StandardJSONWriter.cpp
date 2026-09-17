#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

WStandardJSONWriter::JSONState::JSONState()
{
  m_State = Invalid;
  m_bRequireComma = false;
  m_bValueWasWritten = false;
}

WStandardJSONWriter::CommaWriter::CommaWriter(WStandardJSONWriter* pWriter)
{
  const WStandardJSONWriter::State state = pWriter->m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV(state == WStandardJSONWriter::Array || state == WStandardJSONWriter::NamedArray || state == WStandardJSONWriter::Variable,
    "Values can only be written inside BeginVariable() / EndVariable() and BeginArray() / EndArray().");

  m_pWriter = pWriter;

  if (m_pWriter->m_StateStack.PeekBack().m_bRequireComma)
  {
    // we are writing the comma now, so it is not required anymore
    m_pWriter->m_StateStack.PeekBack().m_bRequireComma = false;

    if (m_pWriter->m_StateStack.PeekBack().m_State == WStandardJSONWriter::Array ||
        m_pWriter->m_StateStack.PeekBack().m_State == WStandardJSONWriter::NamedArray)
    {
      if (pWriter->m_WhitespaceMode >= WJSONWriter::WhitespaceMode::NewlinesOnly)
      {
        if (pWriter->m_ArrayMode == WJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(",");
        else
          m_pWriter->OutputString(",\n");
      }
      else
      {
        if (pWriter->m_ArrayMode == WJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(", ");
        else
        {
          m_pWriter->OutputString(",\n");
          m_pWriter->OutputIndentation();
        }
      }
    }
    else
    {
      if (pWriter->m_WhitespaceMode >= WJSONWriter::WhitespaceMode::None)
        m_pWriter->OutputString(",");
      else
        m_pWriter->OutputString(",\n");

      m_pWriter->OutputIndentation();
    }
  }
}

WStandardJSONWriter::CommaWriter::~CommaWriter()
{
  m_pWriter->m_StateStack.PeekBack().m_bRequireComma = true;
  m_pWriter->m_StateStack.PeekBack().m_bValueWasWritten = true;
}

WStandardJSONWriter::WStandardJSONWriter()
{
  m_iIndentation = 0;
  m_pOutput = nullptr;
  JSONState s;
  s.m_State = WStandardJSONWriter::Empty;
  m_StateStack.PushBack(s);
}

WStandardJSONWriter::~WStandardJSONWriter()
{
  if (!HadWriteError())
  {
    W_ASSERT_DEV(m_StateStack.PeekBack().m_State == WStandardJSONWriter::Empty, "The JSON stream must be closed properly.");
  }
}

void WStandardJSONWriter::SetOutputStream(WStreamWriter* pOutput)
{
  m_pOutput = pOutput;
}

void WStandardJSONWriter::OutputString(WStringView s)
{
  W_ASSERT_DEBUG(m_pOutput != nullptr, "No output stream has been set yet.");

  if (m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).Failed())
  {
    SetWriteErrorState();
  }
}

void WStandardJSONWriter::OutputEscapedString(WStringView s)
{
  WStringBuilder sEscaped = s;
  sEscaped.ReplaceAll("\\", "\\\\");
  // sEscaped.ReplaceAll("/", "\\/"); // this is not necessary to escape
  sEscaped.ReplaceAll("\"", "\\\"");
  sEscaped.ReplaceAll("\b", "\\b");
  sEscaped.ReplaceAll("\r", "\\r");
  sEscaped.ReplaceAll("\f", "\\f");
  sEscaped.ReplaceAll("\n", "\\n");
  sEscaped.ReplaceAll("\t", "\\t");

  // Everything below 0x20 that has no named escape has to be written as \uXXXX - JSON forbids raw
  // control characters in a string, and parsers reject them rather than skipping them. Text coming out
  // of a log or a user-entered string does contain them.
  //
  // Done last, and by rebuilding rather than with ReplaceAll, because the backslashes inserted here
  // must not go through the escaping above a second time. Control characters are single bytes in UTF-8
  // (continuation bytes are >= 0x80), so scanning bytes cannot cut a multi-byte character in half.
  bool bNeedsUnicodeEscape = false;

  for (WUInt32 i = 0; i < sEscaped.GetElementCount(); ++i)
  {
    if (static_cast<WUInt8>(sEscaped.GetData()[i]) < 0x20)
    {
      bNeedsUnicodeEscape = true;
      break;
    }
  }

  if (bNeedsUnicodeEscape)
  {
    WStringBuilder sResult;
    sResult.Reserve(sEscaped.GetElementCount());

    const char* szCur = sEscaped.GetData();
    const char* szEnd = szCur + sEscaped.GetElementCount();
    const char* szChunkStart = szCur;

    for (; szCur < szEnd; ++szCur)
    {
      const WUInt8 uiByte = static_cast<WUInt8>(*szCur);

      if (uiByte >= 0x20)
        continue;

      sResult.Append(WStringView(szChunkStart, szCur));

      char szEscape[8];
      WStringUtils::snprintf(szEscape, W_ARRAY_SIZE(szEscape), "\\u%04x", uiByte);
      sResult.Append(szEscape);

      szChunkStart = szCur + 1;
    }

    sResult.Append(WStringView(szChunkStart, szEnd));
    sEscaped = sResult;
  }

  OutputString("\"");
  OutputString(sEscaped);
  OutputString("\"");
}

void WStandardJSONWriter::OutputIndentation()
{
  if (m_WhitespaceMode >= WhitespaceMode::NoIndentation)
    return;

  WInt32 iIndentation = m_iIndentation * 2;

  if (m_WhitespaceMode == WhitespaceMode::LessIndentation)
    iIndentation = m_iIndentation;

  WStringBuilder s;
  s.SetPrintf("%*s", iIndentation, "");

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteBool(bool value)
{
  CommaWriter cw(this);

  if (value)
    OutputString("true");
  else
    OutputString("false");
}

void WStandardJSONWriter::WriteInt32(WInt32 value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteUInt32(WUInt32 value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteInt64(WInt64 value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteUInt64(WUInt64 value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  WStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void WStandardJSONWriter::WriteString(WStringView value)
{
  CommaWriter cw(this);

  OutputEscapedString(value);
}

void WStandardJSONWriter::WriteNULL()
{
  CommaWriter cw(this);

  OutputString("null");
}

void WStandardJSONWriter::WriteTime(WTime value)
{
  WriteDouble(value.GetSeconds());
}

void WStandardJSONWriter::WriteFloatComponents(const float* pValues, WUInt32 uiCount, WStringView sComponentNames)
{
  W_ASSERT_DEBUG(uiCount <= sComponentNames.GetElementCount(), "Not enough component names for {} components.", uiCount);

  BeginObject();

  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    const char* szName = sComponentNames.GetStartPointer() + i;
    AddVariableFloat(WStringView(szName, szName + 1), pValues[i]);
  }

  EndObject();
}

void WStandardJSONWriter::WriteColor(const WColor& value)
{
  // The linear/gamma distinction is not part of the output, so a reader has to know which one it is
  // getting from the context - same as for the component types below, which do not record their type
  // either.
  WriteFloatComponents(&value.r, 4, "rgba");
}

void WStandardJSONWriter::WriteColorGamma(const WColorGammaUB& value)
{
  BeginObject();
  AddVariableUInt32("r", value.r);
  AddVariableUInt32("g", value.g);
  AddVariableUInt32("b", value.b);
  AddVariableUInt32("a", value.a);
  EndObject();
}

void WStandardJSONWriter::WriteVec2(const WVec2& value)
{
  WriteFloatComponents(&value.x, 2, "xyzw");
}

void WStandardJSONWriter::WriteVec3(const WVec3& value)
{
  WriteFloatComponents(&value.x, 3, "xyzw");
}

void WStandardJSONWriter::WriteVec4(const WVec4& value)
{
  WriteFloatComponents(&value.x, 4, "xyzw");
}

void WStandardJSONWriter::WriteVec2I32(const WVec2I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  EndObject();
}

void WStandardJSONWriter::WriteVec3I32(const WVec3I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  AddVariableInt32("z", value.z);
  EndObject();
}

void WStandardJSONWriter::WriteVec4I32(const WVec4I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  AddVariableInt32("z", value.z);
  AddVariableInt32("w", value.w);
  EndObject();
}

void WStandardJSONWriter::WriteQuat(const WQuat& value)
{
  WriteFloatComponents(&value.x, 4, "xyzw");
}

void WStandardJSONWriter::WriteMatrix(const float* pValues, WUInt32 uiRowsAndColumns)
{
  BeginArray();

  for (WUInt32 uiRow = 0; uiRow < uiRowsAndColumns; ++uiRow)
  {
    BeginArray();

    for (WUInt32 uiColumn = 0; uiColumn < uiRowsAndColumns; ++uiColumn)
    {
      WriteFloat(pValues[uiRow * uiRowsAndColumns + uiColumn]);
    }

    EndArray();
  }

  EndArray();
}

void WStandardJSONWriter::WriteMat3(const WMat3& value)
{
  float f[9];
  value.GetAsArray(f, WMatrixLayout::RowMajor);

  WriteMatrix(f, 3);
}

void WStandardJSONWriter::WriteMat4(const WMat4& value)
{
  float f[16];
  value.GetAsArray(f, WMatrixLayout::RowMajor);

  WriteMatrix(f, 4);
}

void WStandardJSONWriter::WriteUuid(const WUuid& value)
{
  WStringBuilder s;
  WConversionUtils::ToString(value, s);

  WriteString(s);
}

void WStandardJSONWriter::WriteAngle(WAngle value)
{
  WriteFloat(value.GetDegree());
}

void WStandardJSONWriter::WriteDataBuffer(const WDataBuffer& value)
{
  // Hex, because Foundation has no base64 encoder. This doubles the size of the data, so consider
  // writing a reference to the data instead of the data itself.
  WStringBuilder sHex;

  for (WUInt8 uiByte : value)
  {
    sHex.AppendFormat("{}", WArgU(uiByte, 2, true, 16));
  }

  WriteString(sHex);
}

void WStandardJSONWriter::BeginVariable(WStringView sName)
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV(state == WStandardJSONWriter::Empty || state == WStandardJSONWriter::Object || state == WStandardJSONWriter::NamedObject,
    "Variables can only be written inside objects.");

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  OutputEscapedString(sName);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString(":");
  else
    OutputString(" : ");

  JSONState s;
  s.m_State = WStandardJSONWriter::Variable;
  m_StateStack.PushBack(s);
}

void WStandardJSONWriter::EndVariable()
{
  W_ASSERT_DEV(m_StateStack.PeekBack().m_State == WStandardJSONWriter::Variable, "EndVariable() must be called in sync with BeginVariable().");
  W_ASSERT_DEV(m_StateStack.PeekBack().m_bValueWasWritten, "EndVariable() cannot be called without writing any value in between.");

  End();
}

void WStandardJSONWriter::BeginArray(WStringView sName)
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV((state == WStandardJSONWriter::Empty) ||
                  ((state == WStandardJSONWriter::Object || state == WStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == WStandardJSONWriter::Array || state == WStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == WStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin arrays when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous arrays, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString(",");
    else
      OutputString(", ");
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("[");
  else
    OutputString("[ ");

  JSONState s;
  s.m_State = (sName == nullptr) ? WStandardJSONWriter::Array : WStandardJSONWriter::NamedArray;
  m_StateStack.PushBack(s);
  ++m_iIndentation;
}

void WStandardJSONWriter::EndArray()
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV(
    state == WStandardJSONWriter::Array || state == WStandardJSONWriter::NamedArray, "EndArray() must be called in sync with BeginArray().");


  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == WStandardJSONWriter::NamedArray)
    EndVariable();
}

void WStandardJSONWriter::BeginObject(WStringView sName)
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV((state == WStandardJSONWriter::Empty) ||
                  ((state == WStandardJSONWriter::Object || state == WStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == WStandardJSONWriter::Array || state == WStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == WStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin objects when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous objects, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  if (m_WhitespaceMode >= WJSONWriter::WhitespaceMode::None)
    OutputString("{");
  else
    OutputString("{\n");

  JSONState s;
  s.m_State = (sName == nullptr) ? WStandardJSONWriter::Object : WStandardJSONWriter::NamedObject;
  m_StateStack.PushBack(s);
  ++m_iIndentation;

  OutputIndentation();
}

void WStandardJSONWriter::WriteRawJson(WStringView sJson)
{
  // places the separating comma and marks the value as written, exactly as for any other value
  CommaWriter cw(this);

  // not OutputEscapedString(), because the text is JSON already and must not be quoted or escaped
  OutputString(sJson);
}

void WStandardJSONWriter::AddVariableRawJson(WStringView sName, WStringView sJson)
{
  BeginVariable(sName);
  WriteRawJson(sJson);
  EndVariable();
}

void WStandardJSONWriter::EndAll()
{
  // Works top down through whatever is open, using the public functions so that commas, indentation
  // and the NamedObject/NamedArray cascade into EndVariable() all behave as during normal writing.
  while (m_StateStack.GetCount() > 1)
  {
    switch (m_StateStack.PeekBack().m_State)
    {
      case WStandardJSONWriter::Variable:
        // EndVariable() asserts unless something was written for it, and an object member without a
        // value is not representable, so the unfinished variable becomes null.
        if (!m_StateStack.PeekBack().m_bValueWasWritten)
          WriteNULL();

        EndVariable();
        break;

      case WStandardJSONWriter::Object:
      case WStandardJSONWriter::NamedObject:
        EndObject();
        break;

      case WStandardJSONWriter::Array:
      case WStandardJSONWriter::NamedArray:
        EndArray();
        break;

      default:
        // Nothing else can be on the stack above the initial Empty state. Bailing out rather than
        // looping forever, since this runs on an error path where an assert would be unhelpful.
        return;
    }
  }
}

void WStandardJSONWriter::EndObject()
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  W_IGNORE_UNUSED(state);
  W_ASSERT_DEV(
    state == WStandardJSONWriter::Object || state == WStandardJSONWriter::NamedObject, "EndObject() must be called in sync with BeginObject().");

  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == WStandardJSONWriter::NamedObject)
    EndVariable();
}

void WStandardJSONWriter::End()
{
  const WStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;

  if (m_StateStack.PeekBack().m_State == WStandardJSONWriter::Array || m_StateStack.PeekBack().m_State == WStandardJSONWriter::NamedArray)
  {
    --m_iIndentation;

    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("]");
    else
      OutputString(" ]");
  }


  m_StateStack.PopBack();
  m_StateStack.PeekBack().m_bRequireComma = true;

  if (state == WStandardJSONWriter::Object || state == WStandardJSONWriter::NamedObject)
  {
    --m_iIndentation;

    if (m_WhitespaceMode < WJSONWriter::WhitespaceMode::None)
      OutputString("\n");

    OutputIndentation();
    OutputString("}");
  }
}


void WStandardJSONWriter::WriteBinaryData(WStringView sDataType, const void* pData, WUInt32 uiBytes, WStringView sValueString)
{
  CommaWriter cw(this);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{\"$t\":\"");
  else
    OutputString("{ \"$t\" : \"");

  OutputString(sDataType);

  if (!sValueString.IsEmpty())
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("\",\"$v\":\"");
    else
      OutputString("\", \"$v\" : \"");

    OutputString(sValueString);
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\",\"$b\":\"0x");
  else
    OutputString("\", \"$b\" : \"0x");

  WStringBuilder s;

  WUInt8* pBytes = (WUInt8*)pData;

  for (WUInt32 i = 0; i < uiBytes; ++i)
  {
    s.SetFormat("{0}", WArgU((WUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    OutputString(s.GetData());
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\"}");
  else
    OutputString("\" }");
}
