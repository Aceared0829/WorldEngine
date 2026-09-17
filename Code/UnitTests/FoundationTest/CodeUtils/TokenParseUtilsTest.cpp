#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

W_CREATE_SIMPLE_TEST(CodeUtils, TokenParseUtils)
{
  const char* stringLiteral = R"(
// Some comment
/* A block comment
Some block
*/
Identifier
)";

  WTokenizer tokenizer(WFoundation::GetDefaultAllocator());
  tokenizer.Tokenize(WMakeArrayPtr(reinterpret_cast<const WUInt8*>(stringLiteral), WStringUtils::GetStringElementCount(stringLiteral)), WLog::GetThreadLocalLogSystem(), false);

  WTokenParseUtils::TokenStream tokens;
  tokenizer.GetAllTokens(tokens);

  W_TEST_BLOCK(WTestBlock::Enabled, "SkipWhitespace / IsEndOfLine")
  {
    WUInt32 uiCurToken = 0;
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    W_TEST_BOOL(!WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    WTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    W_TEST_INT(uiCurToken, 2);
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    W_TEST_BOOL(!WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    WTokenParseUtils::SkipWhitespace(tokens, uiCurToken);
    W_TEST_INT(uiCurToken, 4);
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    W_TEST_BOOL(!WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    W_TEST_BOOL(!WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, true));
    W_TEST_INT(tokens[uiCurToken]->m_iType, WTokenType::Identifier);
    uiCurToken++;
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    uiCurToken++;
    W_TEST_INT(tokens[uiCurToken]->m_iType, WTokenType::EndOfFile);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SkipWhitespaceAndNewline")
  {
    WUInt32 uiCurToken = 0;
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    WTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    W_TEST_INT(uiCurToken, 5);
    W_TEST_INT(tokens[uiCurToken]->m_iType, WTokenType::Identifier);
    uiCurToken++;
    W_TEST_BOOL(WTokenParseUtils::IsEndOfLine(tokens, uiCurToken, false));
    WTokenParseUtils::SkipWhitespaceAndNewline(tokens, uiCurToken);
    W_TEST_INT(tokens[uiCurToken]->m_iType, WTokenType::EndOfFile);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CopyRelevantTokens")
  {
    WUInt32 uiCurToken = 0;
    WTokenParseUtils::TokenStream relevantTokens;
    WTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, true);

    W_TEST_INT(relevantTokens.GetCount(), 5);
    for (WUInt32 i = 0; i < relevantTokens.GetCount(); ++i)
    {
      if (i == 3)
      {
        W_TEST_INT(relevantTokens[i]->m_iType, WTokenType::Identifier);
      }
      else
      {
        W_TEST_INT(relevantTokens[i]->m_iType, WTokenType::Newline);
      }
    }

    relevantTokens.Clear();
    WTokenParseUtils::CopyRelevantTokens(tokens, uiCurToken, relevantTokens, false);
    W_TEST_INT(relevantTokens.GetCount(), 1);
    W_TEST_INT(relevantTokens[0]->m_iType, WTokenType::Identifier);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Accept")
  {
    WUInt32 uiCurToken = 0;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, "\n"_wsv, nullptr));
    W_TEST_INT(uiCurToken, 1);
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, WTokenType::Newline, nullptr));
    W_TEST_INT(uiCurToken, 3);
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, "\n"_wsv, nullptr));
    W_TEST_INT(uiCurToken, 5);

    WUInt32 uiIdentifierToken = 0;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, WTokenType::Identifier, &uiIdentifierToken));
    W_TEST_INT(uiIdentifierToken, 5);
    W_TEST_INT(uiCurToken, 6);

    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, WTokenType::Newline, nullptr));

    W_TEST_BOOL(!WTokenParseUtils::Accept(tokens, uiCurToken, WTokenType::Newline, nullptr));
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, WTokenType::EndOfFile, nullptr));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Accept2")
  {
    WUInt32 uiCurToken = 0;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, "\n"_wsv, "// Some comment"_wsv, nullptr));
    W_TEST_INT(uiCurToken, 2);
    WUInt32 uiTouple1Token = 0;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, "\n"_wsv, "/* A block comment\nSome block\n*/"_wsv, &uiTouple1Token));
    W_TEST_INT(uiTouple1Token, 2);
    W_TEST_INT(uiCurToken, 4);
    W_TEST_BOOL(!WTokenParseUtils::AcceptUnless(tokens, uiCurToken, "\n"_wsv, "Identifier"_wsv, nullptr));
    uiCurToken++;
    WUInt32 uiIdentifierToken = 0;
    W_TEST_BOOL(WTokenParseUtils::AcceptUnless(tokens, uiCurToken, "Identifier"_wsv, "ScaryString"_wsv, &uiIdentifierToken));
    W_TEST_INT(uiIdentifierToken, 5);
    W_TEST_INT(uiCurToken, 6);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Accept3")
  {
    WUInt32 uiCurToken = 0;
    WTokenParseUtils::TokenMatch templatePattern[] = {WTokenType::Newline, WTokenType::Newline, "Identifier"_wsv};
    WTempHybridArray<WUInt32, 8> acceptedTokens;
    W_TEST_BOOL(!WTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));
    uiCurToken++;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens, uiCurToken, templatePattern, &acceptedTokens));

    W_TEST_INT(acceptedTokens.GetCount(), W_ARRAY_SIZE(templatePattern));
    W_TEST_INT(acceptedTokens[0], 2);
    W_TEST_INT(acceptedTokens[1], 4);
    W_TEST_INT(acceptedTokens[2], 5);
    W_TEST_INT(uiCurToken, 6);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Accept4")
  {
    const char* vectorString = "Vec2(2.2, 1.1)";

    WTokenizer tokenizer2(WFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(WMakeArrayPtr(reinterpret_cast<const WUInt8*>(vectorString), WStringUtils::GetStringElementCount(vectorString)), WLog::GetThreadLocalLogSystem(), false);

    WTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    WUInt32 uiCurToken = 0;
    WTokenParseUtils::TokenMatch templatePattern[] = {"Vec2"_wsv, "("_wsv, WTokenType::Float, ","_wsv, WTokenType::Float, ")"_wsv};
    WTempHybridArray<WUInt32, 6> acceptedTokens;
    W_TEST_BOOL(WTokenParseUtils::Accept(tokens2, uiCurToken, templatePattern, &acceptedTokens));
    W_TEST_INT(uiCurToken, 7);
    W_TEST_INT(acceptedTokens.GetCount(), W_ARRAY_SIZE(templatePattern));
    W_TEST_INT(acceptedTokens[2], 2);
    W_TEST_INT(acceptedTokens[4], 5);
    W_TEST_STRING(tokens2[acceptedTokens[2]]->m_DataView, "2.2");
    W_TEST_STRING(tokens2[acceptedTokens[4]]->m_DataView, "1.1");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CombineTokensToString")
  {
    WUInt32 uiCurToken = 0;
    WStringBuilder sResult;
    WTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult);
    W_TEST_STRING(sResult, stringLiteral);

    WTokenParseUtils::CombineTokensToString(tokens, uiCurToken, sResult, false, true);
    W_TEST_STRING(sResult, "\n\n\nIdentifier\n");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CombineRelevantTokensToString")
  {
    WUInt32 uiCurToken = 0;
    WStringBuilder sResult;
    WTokenParseUtils::CombineRelevantTokensToString(tokens, uiCurToken, sResult);
    W_TEST_STRING(sResult, "Identifier");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CreateCleanTokenStream")
  {
    const char* stringLiteralWithRedundantStuff = "\n\nID1 \nID2";

    WTokenizer tokenizer2(WFoundation::GetDefaultAllocator());
    tokenizer2.Tokenize(WMakeArrayPtr(reinterpret_cast<const WUInt8*>(stringLiteralWithRedundantStuff), WStringUtils::GetStringElementCount(stringLiteralWithRedundantStuff)), WLog::GetThreadLocalLogSystem(), false);

    WTokenParseUtils::TokenStream tokens2;
    tokenizer2.GetAllTokens(tokens2);

    WUInt32 uiCurToken = 0;
    WTokenParseUtils::TokenStream result;
    WTokenParseUtils::CreateCleanTokenStream(tokens2, uiCurToken, result);

    W_TEST_INT(result.GetCount(), 5);

    WTokenParseUtils::TokenMatch templatePattern[] = {WTokenType::Newline, "ID1"_wsv, WTokenType::Newline, "ID2"_wsv, WTokenType::EndOfFile};
    WTempHybridArray<WUInt32, 8> acceptedTokens;
    W_TEST_BOOL(WTokenParseUtils::Accept(result, uiCurToken, templatePattern, nullptr));
    W_TEST_INT(uiCurToken, 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RenderTemplate")
  {
    // Renders every placeholder as '<NAME>' or '<NAME:INDEX>' so that both the resolved name and index are visible in the result. Optional placeholders use '(..)' instead of '<..>'.
    auto Resolver = [](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    {
      ref_sOutput.Append(bOptional ? "(" : "<", sPlaceholder);
      if (index.IsValid())
      {
        const WString sIndex = index.ConvertTo<WString>();
        ref_sOutput.Append(":", sIndex.GetView());
      }
      ref_sOutput.Append(bOptional ? ")" : ">");
    };

    WStringBuilder sOutput;

    struct TemplateTest
    {
      WStringView m_sTemplate;
      WStringView m_sExpected;
    };

    TemplateTest tests[] = {
      // trivial cases
      {""_wsv, ""_wsv},
      {"Perlin Noise"_wsv, "Perlin Noise"_wsv},
      {"{Name}"_wsv, "<Name>"_wsv},

      // examples taken from existing WTitleAttribute usages
      {"Random: {Seed}"_wsv, "Random: <Seed>"_wsv},
      {"Subgraph: {Name}"_wsv, "Subgraph: <Name>"_wsv},
      {"{Active} Placement Output: {Name}"_wsv, "<Active> Placement Output: <Name>"_wsv},
      {"{Operator}({A}, {B})"_wsv, "<Operator>(<A>, <B>)"_wsv},
      {"Remap: [{InputMin}, {InputMax}] -> [{OutputMin}, {OutputMax}]"_wsv, "Remap: [<InputMin>, <InputMax>] -> [<OutputMin>, <OutputMax>]"_wsv},
      {"Height: [{MinHeight}, {MaxHeight}]"_wsv, "Height: [<MinHeight>, <MaxHeight>]"_wsv},
      {"Coroutine::MoveTo {TargetPos}"_wsv, "Coroutine::MoveTo <TargetPos>"_wsv},
      {"= {Expression}"_wsv, "= <Expression>"_wsv},
      {"Compare: Number {Comparison} {ReferenceValue}"_wsv, "Compare: Number <Comparison> <ReferenceValue>"_wsv},
      {"{Type}::Set {Property} = {Value}"_wsv, "<Type>::Set <Property> = <Value>"_wsv},
      {"ForLoop [{FirstIndex}..{LastIndex}]"_wsv, "ForLoop [<FirstIndex>..<LastIndex>]"_wsv},
      {"Array::GetElement[{Index}]"_wsv, "Array::GetElement[<Index>]"_wsv},
      {"Clamp({X}, {Min}, {Max})"_wsv, "Clamp(<X>, <Min>, <Max>)"_wsv},
      {"{Condition} ? {A} : {B}"_wsv, "<Condition> ? <A> : <B>"_wsv},
      {"Expression::{Expression}"_wsv, "Expression::<Expression>"_wsv},
      {"Variant::ConvertTo {Type}"_wsv, "Variant::ConvertTo <Type>"_wsv},

      // indexed placeholders
      {"BlendSpace 1D: {Clips[0]} {Clips[1]} {Clips[2]}"_wsv, "BlendSpace 1D: <Clips:0> <Clips:1> <Clips:2>"_wsv},
      {"Bone Weights {RootBones[10]}"_wsv, "Bone Weights <RootBones:10>"_wsv},

      // whitespace between the placeholder tokens is tolerated and stripped
      {"{ Name }"_wsv, "<Name>"_wsv},
      {"{ Clips [ 2 ] }"_wsv, "<Clips:2>"_wsv},

      // the optional '$' prefix is stripped, as used by the visual shader titles
      {"{$Name}"_wsv, "<Name>"_wsv},
      {"{$Clips[0]}"_wsv, "<Clips:0>"_wsv},
      {"{$in0} + {$in1}"_wsv, "<in0> + <in1>"_wsv},
      {"Color: {$prop0}"_wsv, "Color: <prop0>"_wsv},
      {"Lerp: {$in0} -> {$in1} ({$in2})"_wsv, "Lerp: <in0> -> <in1> (<in2>)"_wsv},
      {"{ $ Name }"_wsv, "<Name>"_wsv},

      // the optional '?' prefix marks a placeholder as optional and can be combined with '$'
      {"{?Name}"_wsv, "(Name)"_wsv},
      {"{?Clips[2]}"_wsv, "(Clips:2)"_wsv},
      {"{?$in0}"_wsv, "(in0)"_wsv},
      {"{?$Clips[1]}"_wsv, "(Clips:1)"_wsv},
      {"{ ? Name }"_wsv, "(Name)"_wsv},
      {"Set Bool: '{BlackboardEntry}' to {?Bool}"_wsv, "Set Bool: '<BlackboardEntry>' to (Bool)"_wsv},

      // placeholders don't need to be separated by anything
      {"{A}{B}"_wsv, "<A><B>"_wsv},

      // the index goes through the integer conversion, which strips leading zeros
      {"{Clips[007]}"_wsv, "<Clips:7>"_wsv},

      // a template may span several lines
      {"{A}\n{B}"_wsv, "<A>\n<B>"_wsv},

      // non-ASCII text is passed through unchanged
      {"Gr\u00f6\u00dfe: {Size}"_wsv, "Gr\u00f6\u00dfe: <Size>"_wsv},

      // comments count as whitespace inside a placeholder and are swallowed with it
      {"{ /*c*/ Name }"_wsv, "<Name>"_wsv},

      // a comment is a single token, so a placeholder inside one is not resolved
      {"/* {Name} */"_wsv, "/* {Name} */"_wsv},

      // nothing that doesn't form a complete placeholder is touched
      {"{Name"_wsv, "{Name"_wsv},
      {"Name}"_wsv, "Name}"_wsv},
      {"{}"_wsv, "{}"_wsv},
      {"{123}"_wsv, "{123}"_wsv},
      {"{Clips[]}"_wsv, "{Clips[]}"_wsv},
      {"{Clips[a]}"_wsv, "{Clips[a]}"_wsv},
      {"{Clips[-1]}"_wsv, "{Clips[-1]}"_wsv},
      {"{Clips[99999999999999]}"_wsv, "{Clips[99999999999999]}"_wsv},
      {"{$}"_wsv, "{$}"_wsv},
      {"{$$Name}"_wsv, "{$$Name}"_wsv},
      {"{$0}"_wsv, "{$0}"_wsv},
      {"{?}"_wsv, "{?}"_wsv},
      {"{??Name}"_wsv, "{??Name}"_wsv},
      {"{$?Name}"_wsv, "{$?Name}"_wsv},
      {"{{Name}}"_wsv, "{<Name>}"_wsv},
      {"100% {Name} & <stuff>"_wsv, "100% <Name> & <stuff>"_wsv},

      // placeholders inside quoted sections are resolved as well, the quotes are preserved
      {"Sample Clip: '{Clip}'"_wsv, "Sample Clip: '<Clip>'"_wsv},
      {"Log: \"{Text}\""_wsv, "Log: \"<Text>\""_wsv},
      {"Set Number: '{BlackboardEntry}' to {Number}"_wsv, "Set Number: '<BlackboardEntry>' to <Number>"_wsv},
      {"BlendSpace 1D: '{Clips[0]}' '{Clips[1]}'"_wsv, "BlendSpace 1D: '<Clips:0>' '<Clips:1>'"_wsv},
      {"'\"{A}\"'"_wsv, "'\"<A>\"'"_wsv},
      {"'{?A}'"_wsv, "'(A)'"_wsv},
      {"Log: 'nothing here'"_wsv, "Log: 'nothing here'"_wsv},
      {"''"_wsv, "''"_wsv},
      {"\"\""_wsv, "\"\""_wsv},
    };

    for (const auto& test : tests)
    {
      WTokenParseUtils::RenderTemplate(test.m_sTemplate, Resolver, sOutput);
      W_TEST_STRING(sOutput, test.m_sExpected);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RenderTemplate: optional flag")
  {
    // Mimics a node title that omits optional placeholders.
    auto Resolver = [](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    {
      W_IGNORE_UNUSED(index);
      if (!bOptional)
      {
        ref_sOutput.Append(sPlaceholder);
      }
    };

    WStringBuilder sOutput;

    WTokenParseUtils::RenderTemplate("{?A}"_wsv, Resolver, sOutput);
    W_TEST_STRING(sOutput, "");

    WTokenParseUtils::RenderTemplate("{?A}{?B}{C}"_wsv, Resolver, sOutput);
    W_TEST_STRING(sOutput, "C");

    WTokenParseUtils::RenderTemplate("Set {Name} to {?Value}"_wsv, Resolver, sOutput);
    W_TEST_STRING(sOutput, "Set Name to ");

    // the quotes around a dropped placeholder remain, which is what callers have to clean up afterwards
    WTokenParseUtils::RenderTemplate("['{?A}' '{?B}']"_wsv, Resolver, sOutput);
    W_TEST_STRING(sOutput, "['' '']");
  }
}
