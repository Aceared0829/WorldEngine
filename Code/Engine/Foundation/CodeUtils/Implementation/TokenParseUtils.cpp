#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace WTokenParseUtils
{
  void SkipWhitespace(const TokenStream& tokens, WUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == WTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == WTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == WTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  void SkipWhitespaceAndNewline(const TokenStream& tokens, WUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == WTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == WTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == WTokenType::Newline) || (tokens[ref_uiCurToken]->m_iType == WTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  bool IsEndOfLine(const TokenStream& tokens, WUInt32 uiCurToken, bool bIgnoreWhitespace)
  {
    if (bIgnoreWhitespace)
      SkipWhitespace(tokens, uiCurToken);

    if (uiCurToken >= tokens.GetCount())
      return true;

    return tokens[uiCurToken]->m_iType == WTokenType::Newline || tokens[uiCurToken]->m_iType == WTokenType::EndOfFile;
  }

  void CopyRelevantTokens(const TokenStream& source, WUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines)
  {
    ref_destination.Reserve(ref_destination.GetCount() + source.GetCount() - uiFirstSourceToken);

    {
      // skip all whitespace at the start of the replacement string
      WUInt32 i = uiFirstSourceToken;
      SkipWhitespace(source, i);

      // add all the relevant tokens to the definition
      for (; i < source.GetCount(); ++i)
      {
        if (source[i]->m_iType == WTokenType::BlockComment || source[i]->m_iType == WTokenType::LineComment || source[i]->m_iType == WTokenType::EndOfFile || (!bPreserveNewLines && source[i]->m_iType == WTokenType::Newline))
          continue;

        ref_destination.PushBack(source[i]);
      }
    }

    // remove whitespace at end of macro
    while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == WTokenType::Whitespace)
      ref_destination.PopBack();
  }

  bool Accept(const TokenStream& tokens, WUInt32& ref_uiCurToken, WStringView sToken, WUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, WUInt32& ref_uiCurToken, WTokenType::Enum type, WUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_iType == type)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, WUInt32& ref_uiCurToken, WStringView sToken1, WStringView sToken2, WUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView == sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 2;
      return true;
    }

    return false;
  }

  bool AcceptUnless(const TokenStream& tokens, WUInt32& ref_uiCurToken, WStringView sToken1, WStringView sToken2, WUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView != sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 1;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, WUInt32& ref_uiCurToken, WArrayPtr<const TokenMatch> matches, WDynamicArray<WUInt32>* pAccepted)
  {
    if (pAccepted)
      pAccepted->Clear();

    WUInt32 uiCurToken = ref_uiCurToken;
    bool bAccepted = true;
    for (WUInt32 i = 0; i < matches.GetCount() && bAccepted; ++i)
    {
      WUInt32 uiAcceptedToken = uiCurToken;
      const TokenMatch& match = matches[i];
      if (match.m_Type == WTokenType::Unknown)
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_sToken, &uiAcceptedToken);
      }
      else
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_Type, &uiAcceptedToken);
      }

      if (pAccepted && bAccepted)
        pAccepted->PushBack(uiAcceptedToken);
    }

    if (bAccepted)
    {
      ref_uiCurToken = uiCurToken;
    }
    else
    {
      if (pAccepted)
        pAccepted->Clear();
    }
    return bAccepted;
  }

  void CombineRelevantTokensToString(const TokenStream& tokens, WUInt32 uiCurToken, WStringBuilder& ref_sResult)
  {
    ref_sResult.Clear();
    WStringBuilder sTemp;

    for (WUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if ((tokens[t]->m_iType == WTokenType::LineComment) || (tokens[t]->m_iType == WTokenType::BlockComment) || (tokens[t]->m_iType == WTokenType::Newline) || (tokens[t]->m_iType == WTokenType::EndOfFile))
        continue;

      sTemp = tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }

  void CreateCleanTokenStream(const TokenStream& tokens, WUInt32 uiCurToken, TokenStream& ref_destination)
  {
    SkipWhitespace(tokens, uiCurToken);

    for (WUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if (tokens[t]->m_iType == WTokenType::Newline)
      {
        // remove all whitespace before a newline
        while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == WTokenType::Whitespace)
          ref_destination.PopBack();

        // if there is already a newline stored, discard the new one
        if (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == WTokenType::Newline)
          continue;
      }

      ref_destination.PushBack(tokens[t]);
    }
  }

  void CombineTokensToString(const TokenStream& tokens0, WUInt32 uiCurToken, WStringBuilder& ref_sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
  {
    TokenStream Tokens;

    if (bRemoveRedundantWhitespace)
    {
      CreateCleanTokenStream(tokens0, uiCurToken, Tokens);
      uiCurToken = 0;
    }
    else
      Tokens = tokens0;

    ref_sResult.Clear();
    WStringBuilder sTemp;

    WUInt32 uiCurLine = 0xFFFFFFFF;
    WHashedString sCurFile;

    for (WUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      // skip all comments, if not desired
      if ((Tokens[t]->m_iType == WTokenType::BlockComment || Tokens[t]->m_iType == WTokenType::LineComment) && !bKeepComments)
        continue;

      if (Tokens[t]->m_iType == WTokenType::EndOfFile)
        return;

      if (bInsertLine)
      {
        if (ref_sResult.IsEmpty())
        {
          ref_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile = Tokens[t]->m_File;
        }

        if (t > 0 && Tokens[t - 1]->m_iType == WTokenType::Newline)
        {
          if (Tokens[t]->m_uiLine != uiCurLine || Tokens[t]->m_File != sCurFile)
          {
            if (!ref_sResult.EndsWith("\n"))
              ref_sResult.Append("\n");

            ref_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
            uiCurLine = Tokens[t]->m_uiLine;
            sCurFile = Tokens[t]->m_File;
          }
        }

        if (Tokens[t]->m_iType == WTokenType::Newline)
        {
          ++uiCurLine;
        }
      }

      sTemp = Tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }

  void RenderTemplate(WStringView sTemplate, const WDelegate<void(WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)>& resolveAndAppendPlaceholder, WStringBuilder& out_sOutput)
  {
    WTokenizer tokenizer(WTempAllocator::Get());
    tokenizer.Tokenize(WMakeByteArrayPtr(sTemplate.GetStartPointer(), sTemplate.GetElementCount()), WLog::GetThreadLocalLogSystem(), false);

    WUInt32 uiCurToken = 0;
    WTempHybridArray<const WToken*, 32> tokens;
    tokens.Reserve(tokenizer.GetTokens().GetCount());
    for (const WToken& token : tokenizer.GetTokens())
    {
      tokens.PushBack(&token);
    }

    out_sOutput.Clear();
    out_sOutput.Reserve(sTemplate.GetElementCount());

    const char* szStart = sTemplate.GetStartPointer();
    while (!Accept(tokens, uiCurToken, WTokenType::EndOfFile))
    {
      const WToken* pToken = tokens[uiCurToken];

      // Find '{', an optional '?' and '$', a NAME, an optional '[INDEX]' and finally '}'.
      WUInt32 uiNextToken = uiCurToken;
      WUInt32 uiOpenToken = 0;
      WUInt32 uiNameToken = 0;
      WUInt32 uiCloseToken = 0;
      bool bOptional = false;
      bool bMatched = false;
      WVariant index;

      if (Accept(tokens, uiNextToken, "{"_wsv, &uiOpenToken))
      {
        bOptional = Accept(tokens, uiNextToken, "?"_wsv);

        // the '$' carries no meaning, {$NAME} and {NAME} are equivalent
        Accept(tokens, uiNextToken, "$"_wsv);

        if (Accept(tokens, uiNextToken, WTokenType::Identifier, &uiNameToken))
        {
          bool bIndexValid = true;

          WUInt32 uiIndexToken = 0;
          if (Accept(tokens, uiNextToken, "["_wsv))
          {
            bIndexValid = Accept(tokens, uiNextToken, WTokenType::Integer, &uiIndexToken) && Accept(tokens, uiNextToken, "]"_wsv);

            if (bIndexValid)
            {
              // an integer token that doesn't fit into WInt32 is not treated as a placeholder
              WInt32 iIndex = 0;
              bIndexValid = WConversionUtils::StringToInt(tokens[uiIndexToken]->m_DataView, iIndex).Succeeded();

              if (bIndexValid)
              {
                index = iIndex;
              }
            }
          }

          bMatched = bIndexValid && Accept(tokens, uiNextToken, "}"_wsv, &uiCloseToken);
        }
      }

      if (bMatched)
      {
        out_sOutput.Append(WStringView(szStart, tokens[uiOpenToken]->m_DataView.GetStartPointer()));
        resolveAndAppendPlaceholder(tokens[uiNameToken]->m_DataView, index, bOptional, out_sOutput);
        szStart = tokens[uiCloseToken]->m_DataView.GetEndPointer();
        uiCurToken = uiNextToken;
      }
      else if (pToken->m_iType == WTokenType::String1 || pToken->m_iType == WTokenType::String2)
      {
        // The tokenizer turns a quoted section into a single string token, so placeholders inside it have to be resolved by rendering the string content (without the enclosing quotes) separately.
        const char* szContentStart = pToken->m_DataView.GetStartPointer() + 1;
        const char* szContentEnd = pToken->m_DataView.GetEndPointer();

        // an unterminated string literal has no closing quote to exclude
        if (szContentEnd > szContentStart && *(szContentEnd - 1) == *pToken->m_DataView.GetStartPointer())
        {
          --szContentEnd;
        }

        WStringBuilder sContent(WTempAllocator::Get());
        RenderTemplate(WStringView(szContentStart, szContentEnd), resolveAndAppendPlaceholder, sContent);

        out_sOutput.Append(WStringView(szStart, szContentStart));
        out_sOutput.Append(sContent.GetView());
        szStart = szContentEnd;
        ++uiCurToken;
      }
      else
      {
        ++uiCurToken;
      }
    }
    out_sOutput.Append(WStringView(szStart, sTemplate.GetEndPointer()));
  }
} // namespace WTokenParseUtils
