#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  using TokenMatch = WTokenParseUtils::TokenMatch;

  void CompareResults(const WDynamicArray<TokenMatch>& expected, WTokenizer& inout_tokenizer, bool bIgnoreWhitespace)
  {
    auto& tokens = inout_tokenizer.GetTokens();

    const WUInt32 expectedCount = expected.GetCount();
    const WUInt32 tokenCount = tokens.GetCount();

    WUInt32 expectedIndex = 0, tokenIndex = 0;
    while (expectedIndex < expectedCount && tokenIndex < tokenCount)
    {
      auto& token = tokens[tokenIndex];
      if (bIgnoreWhitespace && (token.m_iType == WTokenType::Whitespace || token.m_iType == WTokenType::Newline))
      {
        tokenIndex++;
        continue;
      }

      auto& e = expected[expectedIndex];

      if (!W_TEST_BOOL_MSG(e.m_Type == token.m_iType, "Token with index %u does not match in type, expected %d actual %d", expectedIndex, e.m_Type, token.m_iType))
      {
        return;
      }

      if (!W_TEST_BOOL_MSG(e.m_sToken == token.m_DataView, "Token with index %u does not match, expected '%.*s' actual '%.*s'", expectedIndex, e.m_sToken.GetElementCount(), e.m_sToken.GetStartPointer(), token.m_DataView.GetElementCount(), token.m_DataView.GetStartPointer()))
      {
        return;
      }
      tokenIndex++;
      expectedIndex++;
    }

    // Skip remaining whitespace and newlines
    if (bIgnoreWhitespace)
    {
      while (tokenIndex < tokenCount)
      {
        auto& token = tokens[tokenIndex];
        if (token.m_iType != WTokenType::Whitespace && token.m_iType != WTokenType::Newline)
        {
          break;
        }
        tokenIndex++;
      }
    }

    if (W_TEST_BOOL_MSG(tokenIndex == tokenCount - 1, "Not all tokens have been consumed"))
    {
      W_TEST_BOOL_MSG(tokens[tokenIndex].m_iType == WTokenType::EndOfFile, "Last token must be end of file token");
    }

    W_TEST_BOOL_MSG(expectedIndex == expectedCount, "Not all expected values have been consumed");
  }
} // namespace

W_CREATE_SIMPLE_TEST(CodeUtils, Tokenizer)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Token Types")
  {
    const char* stringLiteral = R"(
float f=10.3f + 100'000.0;
int i=100'000*12345;
// line comment
/*
block comment
*/
char c='f';
const char* bla =  "blup";
)";
    WTokenizer tokenizer(WFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(WMakeArrayPtr(reinterpret_cast<const WUInt8*>(stringLiteral), WStringUtils::GetStringElementCount(stringLiteral)), WLog::GetThreadLocalLogSystem(), false);

    W_TEST_BOOL(tokenizer.GetTokenizedData().IsEmpty());

    WDynamicArray<TokenMatch> expectedResult;
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::Identifier, "float"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Identifier, "f"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::Float, "10.3f"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::NonIdentifier, "+"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Float, "100'000.0"});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::Identifier, "int"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Identifier, "i"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::Integer, "100'000"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({WTokenType::Integer, "12345"});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::LineComment, "// line comment"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::BlockComment, "/*\nblock comment\n*/"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::Identifier, "char"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Identifier, "c"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::String2, "'f'"});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    expectedResult.PushBack({WTokenType::Identifier, "const"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Identifier, "char"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::Identifier, "bla"});
    expectedResult.PushBack({WTokenType::Whitespace, " "});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::Whitespace, "  "});
    expectedResult.PushBack({WTokenType::String1, "\"blup\""});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({WTokenType::Newline, "\n"});

    CompareResults(expectedResult, tokenizer, false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Raw string literal")
  {
    const char* stringLiteral = R"token(
const char* test = R"(
eins
zwei)";
const char* test2 = R"foo(
vier,
fuenf
)foo";
)token";

    WTokenizer tokenizer(WFoundation::GetDefaultAllocator());
    tokenizer.Tokenize(WMakeArrayPtr(reinterpret_cast<const WUInt8*>(stringLiteral), WStringUtils::GetStringElementCount(stringLiteral)), WLog::GetThreadLocalLogSystem());

    WDynamicArray<TokenMatch> expectedResult;
    expectedResult.PushBack({WTokenType::Identifier, "const"});
    expectedResult.PushBack({WTokenType::Identifier, "char"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({WTokenType::Identifier, "test"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::RawString1Prefix, "R\"("});
    expectedResult.PushBack({WTokenType::RawString1, "\neins\nzwei"});
    expectedResult.PushBack({WTokenType::RawString1Postfix, ")\""});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});
    expectedResult.PushBack({WTokenType::Identifier, "const"});
    expectedResult.PushBack({WTokenType::Identifier, "char"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "*"});
    expectedResult.PushBack({WTokenType::Identifier, "test2"});
    expectedResult.PushBack({WTokenType::NonIdentifier, "="});
    expectedResult.PushBack({WTokenType::RawString1Prefix, "R\"foo("});
    expectedResult.PushBack({WTokenType::RawString1, "\nvier,\nfuenf\n"});
    expectedResult.PushBack({WTokenType::RawString1Postfix, ")foo\""});
    expectedResult.PushBack({WTokenType::NonIdentifier, ";"});

    CompareResults(expectedResult, tokenizer, true);
  }
}
