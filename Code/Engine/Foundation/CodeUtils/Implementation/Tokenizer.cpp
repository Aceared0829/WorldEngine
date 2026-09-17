#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Memory/CommonAllocators.h>

const char* WTokenType::EnumNames[WTokenType::ENUM_COUNT] = {
  "Unknown",
  "Whitespace",
  "Identifier",
  "NonIdentifier",
  "Newline",
  "LineComment",
  "BlockComment",
  "String1",
  "String2",
  "Integer",
  "Float",
  "RawString1",
  "RawString1Prefix",
  "RawString1Postfix",
  "EndOfFile"};

namespace
{
  // This allocator is used to get rid of some of the memory allocation tracking
  // that would otherwise occur for allocations made by the tokenizer.
  thread_local WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::Nothing> s_ClassAllocator("WTokenizer", WFoundation::GetDefaultAllocator());
} // namespace


WTokenizer::WTokenizer(WAllocator* pAllocator)
  : m_Tokens(pAllocator != nullptr ? pAllocator : &s_ClassAllocator)
  , m_Data(pAllocator != nullptr ? pAllocator : &s_ClassAllocator)
{
}

WTokenizer::~WTokenizer() = default;

void WTokenizer::NextChar()
{
  m_uiCurChar = m_uiNextChar;
  m_szCurCharStart = m_szNextCharStart;
  ++m_uiCurColumn;

  if (m_uiCurChar == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }

  if (!m_sIterator.IsValid() || m_sIterator.IsEmpty())
  {
    m_szNextCharStart = m_sIterator.GetEndPointer();
    m_uiNextChar = '\0';
    return;
  }

  m_uiNextChar = m_sIterator.GetCharacter();
  m_szNextCharStart = m_sIterator.GetStartPointer();

  ++m_sIterator;
}

void WTokenizer::AddToken()
{
  const char* szEnd = m_szCurCharStart;

  WToken t;
  t.m_uiLine = m_uiLastLine;
  t.m_uiColumn = m_uiLastColumn;
  t.m_iType = m_CurMode;
  t.m_DataView = WStringView(m_szTokenStart, szEnd);

  m_uiLastLine = m_uiCurLine;
  m_uiLastColumn = m_uiCurColumn;

  m_Tokens.PushBack(t);

  m_szTokenStart = szEnd;

  m_CurMode = WTokenType::Unknown;
}

void WTokenizer::Tokenize(WArrayPtr<const WUInt8> data, WLogInterface* pLog, bool bCopyData)
{
  if (bCopyData)
  {
    m_Data = data;
    data = m_Data;
  }
  else
  {
    m_Data.Clear();
  }

  if (data.GetCount() >= 3)
  {
    const char* dataStart = reinterpret_cast<const char*>(data.GetPtr());

    if (WUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      WLog::Error(pLog, "Data to tokenize contains a Utf-8 BOM.");

      // although the tokenizer should get data without a BOM, it's easy enough to work around that here
      // that's what the tokenizer does in other error cases as well - complain, but continue
      data = WArrayPtr<const WUInt8>((const WUInt8*)dataStart, data.GetCount() - 3);
    }
  }

  m_Tokens.Clear();
  m_pLog = pLog;

  {
    m_CurMode = WTokenType::Unknown;
    m_uiCurLine = 1;
    m_uiCurColumn = WInvalidIndex;
    m_uiCurChar = '\0';
    m_uiNextChar = '\0';
    m_uiLastLine = 1;
    m_uiLastColumn = 1;

    m_szCurCharStart = nullptr;
    m_szNextCharStart = nullptr;
    m_szTokenStart = nullptr;
  }

  m_sIterator = {};
  if (!data.IsEmpty())
  {
    m_sIterator = WStringView((const char*)&data[0], (const char*)&data[0] + data.GetCount());
  }

  if (!m_sIterator.IsValid() || m_sIterator.IsEmpty())
  {
    WToken t;
    t.m_uiLine = 1;
    t.m_iType = WTokenType::EndOfFile;
    m_Tokens.PushBack(t);
    return;
  }

  NextChar();
  NextChar();

  m_szTokenStart = m_szCurCharStart;

  while (m_szTokenStart != nullptr && m_szTokenStart != m_sIterator.GetEndPointer())
  {
    switch (m_CurMode)
    {
      case WTokenType::Unknown:
        HandleUnknown();
        break;

      case WTokenType::String1:
        HandleString('\"');
        break;

      case WTokenType::RawString1:
        HandleRawString();
        break;

      case WTokenType::String2:
        HandleString('\'');
        break;

      case WTokenType::Integer:
      case WTokenType::Float:
        HandleNumber();
        break;

      case WTokenType::LineComment:
        HandleLineComment();
        break;

      case WTokenType::BlockComment:
        HandleBlockComment();
        break;

      case WTokenType::Whitespace:
        HandleWhitespace();
        break;

      case WTokenType::Identifier:
        HandleIdentifier();
        break;

      case WTokenType::NonIdentifier:
        HandleNonIdentifier();
        break;

      case WTokenType::RawString1Prefix:
      case WTokenType::RawString1Postfix:
      case WTokenType::Newline:
      case WTokenType::EndOfFile:
      case WTokenType::ENUM_COUNT:
        break;
    }
  }

  WToken t;
  t.m_uiLine = m_uiCurLine;
  t.m_iType = WTokenType::EndOfFile;
  m_Tokens.PushBack(t);
}

void WTokenizer::HandleUnknown()
{
  m_szTokenStart = m_szCurCharStart;

  if ((m_uiCurChar == '/') && (m_uiNextChar == '/'))
  {
    m_CurMode = WTokenType::LineComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_bHashSignIsLineComment && (m_uiCurChar == '#'))
  {
    m_CurMode = WTokenType::LineComment;
    NextChar();
    return;
  }

  if ((m_uiCurChar == '/') && (m_uiNextChar == '*'))
  {
    m_CurMode = WTokenType::BlockComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\"')
  {
    m_CurMode = WTokenType::String1;
    NextChar();
    return;
  }

  if (m_uiCurChar == 'R' && m_uiNextChar == '\"')
  {
    m_CurMode = WTokenType::RawString1;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\'')
  {
    m_CurMode = WTokenType::String2;
    NextChar();
    return;
  }

  if ((m_uiCurChar == ' ') || (m_uiCurChar == '\t'))
  {
    m_CurMode = WTokenType::Whitespace;
    NextChar();
    return;
  }

  if (WStringUtils::IsDecimalDigit(m_uiCurChar) || (m_uiCurChar == '.' && WStringUtils::IsDecimalDigit(m_uiNextChar)))
  {
    m_CurMode = m_uiCurChar == '.' ? WTokenType::Float : WTokenType::Integer;
    // Do not advance to next char here since we need the first character in HandleNumber
    return;
  }

  if (!WStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
  {
    m_CurMode = WTokenType::Identifier;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\n')
  {
    m_CurMode = WTokenType::Newline;
    NextChar();
    AddToken();
    return;
  }

  if ((m_uiCurChar == '\r') && (m_uiNextChar == '\n'))
  {
    NextChar();
    NextChar();
    m_CurMode = WTokenType::Newline;
    AddToken();
    return;
  }

  // else
  m_CurMode = WTokenType::NonIdentifier;
  NextChar();
}

void WTokenizer::HandleString(char terminator)
{
  while (m_uiCurChar != '\0')
  {
    // Escaped quote \"
    if ((m_uiCurChar == '\\') && (m_uiNextChar == WUInt32(terminator)))
    {
      // skip this one
      NextChar();
      NextChar();
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\n'))
    {
      AddToken();

      // skip this one entirely
      NextChar();
      NextChar();

      m_CurMode = terminator == '\"' ? WTokenType::String1 : WTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\r'))
    {
      // this might be a 3 character sequence of \\ \r \n -> skip them all
      AddToken();

      // skip \\ and \r
      NextChar();
      NextChar();

      // skip \n
      if (m_uiCurChar == '\n')
        NextChar();

      m_CurMode = terminator == '\"' ? WTokenType::String1 : WTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped backslash
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\\'))
    {
      // Skip
      NextChar();
      NextChar();
    }
    // not-escaped line break in string
    else if (m_uiCurChar == '\n')
    {
      WLog::Error(m_pLog, "Unescaped Newline in string line {0} column {1}", m_uiCurLine, m_uiCurColumn);
      // NextChar(); // not sure whether to include the newline in the string or not
      AddToken();
      return;
    }
    // end of string
    else if (m_uiCurChar == WUInt32(terminator))
    {
      NextChar();
      AddToken();
      return;
    }
    else
    {
      NextChar();
    }
  }

  WLog::Error(m_pLog, "String not closed at end of file");
  AddToken();
}

void WTokenizer::HandleRawString()
{
  const char* markerStart = m_szCurCharStart;
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar == '(')
    {
      m_sRawStringMarker = WStringView(markerStart, m_szCurCharStart);
      NextChar(); // consume '('
      break;
    }
    NextChar();
  }
  if (m_uiCurChar == '\0')
  {
    WLog::Error(m_pLog, "Failed to find '(' for raw string before end of file");
    AddToken();
    return;
  }

  m_CurMode = WTokenType::RawString1Prefix;
  AddToken();

  m_CurMode = WTokenType::RawString1;

  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar == ')')
    {
      if (m_sRawStringMarker.GetElementCount() == 0 && m_uiNextChar == '\"')
      {
        AddToken();
        NextChar();
        NextChar();
        m_CurMode = WTokenType::RawString1Postfix;
        AddToken();
        return;
      }
      else if (m_szCurCharStart + m_sRawStringMarker.GetElementCount() + 2 <= m_sIterator.GetEndPointer())
      {
        if (WStringUtils::CompareN(m_szCurCharStart + 1, m_sRawStringMarker.GetStartPointer(), m_sRawStringMarker.GetElementCount()) == 0 &&
            m_szCurCharStart[m_sRawStringMarker.GetElementCount() + 1] == '\"')
        {
          AddToken();
          for (WUInt32 i = 0; i < m_sRawStringMarker.GetElementCount() + 2; ++i) // consume )marker"
          {
            NextChar();
          }
          m_CurMode = WTokenType::RawString1Postfix;
          AddToken();
          return;
        }
      }
      NextChar();
    }
    else
    {
      NextChar();
    }
  }

  WLog::Error(m_pLog, "Raw string not closed at end of file");
  AddToken();
}

void WTokenizer::HandleNumber()
{
  if (m_uiCurChar == '0' && (m_uiNextChar == 'x' || m_uiNextChar == 'X'))
  {
    NextChar();
    NextChar();

    WUInt32 uiDigitsRead = 0;
    while (WStringUtils::IsHexDigit(m_uiCurChar))
    {
      NextChar();
      ++uiDigitsRead;
    }

    if (uiDigitsRead < 1)
    {
      WLog::Error(m_pLog, "Invalid hex literal");
    }
  }
  else
  {
    NextChar();

    while (WStringUtils::IsDecimalDigit(m_uiCurChar) || m_uiCurChar == '\'') // integer literal: 100'000
    {
      NextChar();
    }

    if (m_CurMode != WTokenType::Float && (m_uiCurChar == '.' || m_uiCurChar == 'e' || m_uiCurChar == 'E'))
    {
      m_CurMode = WTokenType::Float;
      bool bAllowExponent = true;

      if (m_uiCurChar == '.')
      {
        NextChar();

        WUInt32 uiDigitsRead = 0;
        while (WStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        bAllowExponent = uiDigitsRead > 0;
      }

      if ((m_uiCurChar == 'e' || m_uiCurChar == 'E') && bAllowExponent)
      {
        NextChar();
        if (m_uiCurChar == '+' || m_uiCurChar == '-')
        {
          NextChar();
        }

        WUInt32 uiDigitsRead = 0;
        while (WStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        if (uiDigitsRead < 1)
        {
          WLog::Error(m_pLog, "Invalid float literal");
        }
      }

      if (m_uiCurChar == 'f') // skip float suffix
      {
        NextChar();
      }
    }
  }

  AddToken();
}

void WTokenizer::HandleLineComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '\r') || (m_uiCurChar == '\n'))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // comment at end of file
  AddToken();
}

void WTokenizer::HandleBlockComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '*') && (m_uiNextChar == '/'))
    {
      NextChar();
      NextChar();
      AddToken();
      return;
    }

    NextChar();
  }

  WLog::Error(m_pLog, "Block comment not closed at end of file.");
  AddToken();
}

void WTokenizer::HandleWhitespace()
{
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar != ' ' && m_uiCurChar != '\t')
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // whitespace at end of file
  AddToken();
}

void WTokenizer::HandleIdentifier()
{
  while (m_uiCurChar != '\0')
  {
    if (WStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // identifier at end of file
  AddToken();
}

void WTokenizer::HandleNonIdentifier()
{
  AddToken();
}

void WTokenizer::GetAllTokens(WDynamicArray<const WToken*>& ref_tokens) const
{
  ref_tokens.Clear();
  ref_tokens.Reserve(m_Tokens.GetCount());

  for (const WToken& curToken : m_Tokens)
  {
    ref_tokens.PushBack(&curToken);
  }
}

void WTokenizer::GetAllLines(WDynamicArray<const WToken*>& ref_tokens) const
{
  ref_tokens.Clear();
  ref_tokens.Reserve(m_Tokens.GetCount());

  for (const WToken& curToken : m_Tokens)
  {
    if (curToken.m_iType != WTokenType::Newline)
    {
      ref_tokens.PushBack(&curToken);
    }
  }
}

WResult WTokenizer::GetNextLine(WUInt32& out_uiFirstToken, WDynamicArray<WToken*>& out_tokens)
{
  out_tokens.Clear();

  WTempHybridArray<const WToken*, 32> Tokens0;
  WResult r = GetNextLine(out_uiFirstToken, Tokens0);

  out_tokens.SetCountUninitialized(Tokens0.GetCount());
  for (WUInt32 i = 0; i < Tokens0.GetCount(); ++i)
    out_tokens[i] = const_cast<WToken*>(Tokens0[i]); // soo evil !

  return r;
}

WResult WTokenizer::GetNextLine(WUInt32& out_uiFirstToken, WDynamicArray<const WToken*>& out_tokens) const
{
  out_tokens.Clear();

  const WUInt32 uiMaxTokens = m_Tokens.GetCount() - 1;

  while (out_uiFirstToken < uiMaxTokens)
  {
    const WToken& tCur = m_Tokens[out_uiFirstToken];

    // found a backslash
    if (tCur.m_iType == WTokenType::NonIdentifier && tCur.m_DataView == "\\")
    {
      const WToken& tNext = m_Tokens[out_uiFirstToken + 1];

      // and a newline!
      if (tNext.m_iType == WTokenType::Newline)
      {
        /// \todo Theoretically, if the line ends with an identifier, and the next directly starts with one again,
        // we would need to merge the two into one identifier name, because the \ \n combo means it is not a
        // real line break
        // for now we ignore this and assume there is a 'whitespace' between such identifiers

        // we could maybe at least output a warning, if we detect it
        if (out_uiFirstToken > 0 && m_Tokens[out_uiFirstToken - 1].m_iType == WTokenType::Identifier && out_uiFirstToken + 2 < uiMaxTokens && m_Tokens[out_uiFirstToken + 2].m_iType == WTokenType::Identifier)
        {
          WStringBuilder s1 = m_Tokens[out_uiFirstToken - 1].m_DataView;
          WStringBuilder s2 = m_Tokens[out_uiFirstToken + 2].m_DataView;
          WLog::Warning("Line {0}: The \\ at the line end is in the middle of an identifier name ('{1}' and '{2}'). However, merging identifier "
                         "names is currently not supported.",
            m_Tokens[out_uiFirstToken].m_uiLine, s1, s2);
        }

        // ignore this
        out_uiFirstToken += 2;
        continue;
      }
    }

    out_tokens.PushBack(&tCur);

    if (m_Tokens[out_uiFirstToken].m_iType == WTokenType::Newline)
    {
      ++out_uiFirstToken;
      return W_SUCCESS;
    }

    ++out_uiFirstToken;
  }

  if (out_tokens.IsEmpty())
    return W_FAILURE;

  return W_SUCCESS;
}
