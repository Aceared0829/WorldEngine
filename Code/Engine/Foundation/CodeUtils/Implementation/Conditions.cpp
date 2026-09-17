#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace WTokenParseUtils;

WResult WPreprocessor::CopyTokensAndEvaluateDefined(const TokenStream& Source, WUInt32 uiFirstSourceToken, TokenStream& Destination)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    WUInt32 uiCurToken = uiFirstSourceToken;
    SkipWhitespace(Source, uiCurToken);

    // add all the relevant tokens to the definition
    while (uiCurToken < Source.GetCount())
    {
      if (Source[uiCurToken]->m_iType == WTokenType::BlockComment || Source[uiCurToken]->m_iType == WTokenType::LineComment || Source[uiCurToken]->m_iType == WTokenType::EndOfFile || Source[uiCurToken]->m_iType == WTokenType::Newline)
      {
        ++uiCurToken;
        continue;
      }

      if (Source[uiCurToken]->m_DataView.IsEqual("defined"))
      {
        ++uiCurToken;

        const bool bParenthesis = Accept(Source, uiCurToken, "(");

        WUInt32 uiIdentifier = uiCurToken;
        if (Expect(Source, uiCurToken, WTokenType::Identifier, &uiIdentifier).Failed())
          return W_FAILURE;

        WToken* pReplacement = nullptr;

        const bool bDefined = m_Macros.Find(Source[uiIdentifier]->m_DataView).IsValid();

        // broadcast that 'defined' is being evaluated
        {
          ProcessingEvent pe;
          pe.m_pToken = Source[uiIdentifier];
          pe.m_Type = ProcessingEvent::CheckDefined;
          pe.m_sInfo = bDefined ? "defined" : "undefined";
          m_ProcessingEvents.Broadcast(pe);
        }

        pReplacement = AddCustomToken(Source[uiIdentifier], bDefined ? "1" : "0");

        Destination.PushBack(pReplacement);

        if (bParenthesis)
        {
          if (Expect(Source, uiCurToken, ")").Failed())
            return W_FAILURE;
        }
      }
      else
      {
        Destination.PushBack(Source[uiCurToken]);
        ++uiCurToken;
      }
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == WTokenType::Whitespace)
    Destination.PopBack();

  return W_SUCCESS;
}

WResult WPreprocessor::EvaluateCondition(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  iResult = 0;

  TokenStream Copied(&m_ClassAllocator);
  if (CopyTokensAndEvaluateDefined(Tokens, uiCurToken, Copied).Failed())
    return W_FAILURE;

  TokenStream Expanded(&m_ClassAllocator);

  if (Expand(Copied, Expanded).Failed())
    return W_FAILURE;

  if (Expanded.IsEmpty())
  {
    PP_LOG0(Error, "After expansion the condition is empty", Tokens[uiCurToken]);
    return W_FAILURE;
  }

  WUInt32 uiCurToken2 = 0;
  if (ParseExpressionOr(Expanded, uiCurToken2, iResult).Failed())
    return W_FAILURE;

  return ExpectEndOfLine(Expanded, uiCurToken2);
}

WResult WPreprocessor::ParseFactor(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  while (Accept(Tokens, uiCurToken, "+"))
  {
  }

  if (Accept(Tokens, uiCurToken, "-"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return W_FAILURE;

    iResult = -iResult;
    return W_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "~"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return W_FAILURE;

    iResult = ~iResult;
    return W_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "!"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return W_FAILURE;

    iResult = (iResult != 0) ? 0 : 1;
    return W_SUCCESS;
  }

  WUInt32 uiValueToken = uiCurToken;
  if (Accept(Tokens, uiCurToken, WTokenType::Identifier, &uiValueToken) || Accept(Tokens, uiCurToken, WTokenType::Integer, &uiValueToken))
  {
    const WString sVal = Tokens[uiValueToken]->m_DataView;

    WInt32 iResult32 = 0;

    if (sVal == "true")
    {
      iResult32 = 1;
    }
    else if (sVal == "false")
    {
      iResult32 = 0;
    }
    else if (WConversionUtils::StringToInt(sVal, iResult32).Failed())
    {
      // this is not an error, all unknown identifiers are assumed to be zero

      // broadcast that we encountered this unknown identifier
      ProcessingEvent pe;
      pe.m_pToken = Tokens[uiValueToken];
      pe.m_Type = ProcessingEvent::EvaluateUnknown;
      m_ProcessingEvents.Broadcast(pe);
    }

    iResult = (WInt64)iResult32;

    return W_SUCCESS;
  }
  else if (Accept(Tokens, uiCurToken, "("))
  {
    if (ParseExpressionOr(Tokens, uiCurToken, iResult).Failed())
      return W_FAILURE;

    return Expect(Tokens, uiCurToken, ")");
  }

  uiCurToken = WMath::Min(uiCurToken, Tokens.GetCount() - 1);
  PP_LOG0(Error, "Syntax error, expected identifier, number or '('", Tokens[uiCurToken]);

  return W_FAILURE;
}

WResult WPreprocessor::ParseExpressionPlus(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionMul(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "+"))
    {
      WInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult += iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "-"))
    {
      WInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult -= iNextValue;
    }
    else
      break;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionShift(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionPlus(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, ">", ">"))
    {
      WInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult >>= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "<", "<"))
    {
      WInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult <<= iNextValue;
    }
    else
      break;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionOr(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionAnd(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (Accept(Tokens, uiCurToken, "|", "|"))
  {
    WInt64 iNextValue = 0;
    if (ParseExpressionAnd(Tokens, uiCurToken, iNextValue).Failed())
      return W_FAILURE;

    iResult = (iResult != 0 || iNextValue != 0) ? 1 : 0;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionAnd(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionBitOr(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (Accept(Tokens, uiCurToken, "&", "&"))
  {
    WInt64 iNextValue = 0;
    if (ParseExpressionBitOr(Tokens, uiCurToken, iNextValue).Failed())
      return W_FAILURE;

    iResult = (iResult != 0 && iNextValue != 0) ? 1 : 0;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionBitOr(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionBitXor(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "|", "|"))
  {
    WInt64 iNextValue = 0;
    if (ParseExpressionBitXor(Tokens, uiCurToken, iNextValue).Failed())
      return W_FAILURE;

    iResult |= iNextValue;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionBitAnd(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseCondition(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "&", "&"))
  {
    WInt64 iNextValue = 0;
    if (ParseCondition(Tokens, uiCurToken, iNextValue).Failed())
      return W_FAILURE;

    iResult &= iNextValue;
  }

  return W_SUCCESS;
}

WResult WPreprocessor::ParseExpressionBitXor(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseExpressionBitAnd(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (Accept(Tokens, uiCurToken, "^"))
  {
    WInt64 iNextValue = 0;
    if (ParseExpressionBitAnd(Tokens, uiCurToken, iNextValue).Failed())
      return W_FAILURE;

    iResult ^= iNextValue;
  }

  return W_SUCCESS;
}
WResult WPreprocessor::ParseExpressionMul(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
    return W_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "*"))
    {
      WInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult *= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "/"))
    {
      WInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult /= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "%"))
    {
      WInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return W_FAILURE;

      iResult %= iNextValue;
    }
    else
      break;
  }

  return W_SUCCESS;
}

enum class Comparison
{
  None,
  Equal,
  Unequal,
  LessThan,
  GreaterThan,
  LessThanEqual,
  GreaterThanEqual
};

WResult WPreprocessor::ParseCondition(const TokenStream& Tokens, WUInt32& uiCurToken, WInt64& iResult)
{
  WInt64 iResult1 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult1).Failed())
    return W_FAILURE;

  Comparison Operator = Comparison::None;

  if (Accept(Tokens, uiCurToken, "=", "="))
    Operator = Comparison::Equal;
  else if (Accept(Tokens, uiCurToken, "!", "="))
    Operator = Comparison::Unequal;
  else if (Accept(Tokens, uiCurToken, ">", "="))
    Operator = Comparison::GreaterThanEqual;
  else if (Accept(Tokens, uiCurToken, "<", "="))
    Operator = Comparison::LessThanEqual;
  else if (AcceptUnless(Tokens, uiCurToken, ">", ">"))
    Operator = Comparison::GreaterThan;
  else if (AcceptUnless(Tokens, uiCurToken, "<", "<"))
    Operator = Comparison::LessThan;
  else
  {
    iResult = iResult1;
    return W_SUCCESS;
  }

  WInt64 iResult2 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult2).Failed())
    return W_FAILURE;

  switch (Operator)
  {
    case Comparison::Equal:
      iResult = (iResult1 == iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::GreaterThan:
      iResult = (iResult1 > iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::GreaterThanEqual:
      iResult = (iResult1 >= iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::LessThan:
      iResult = (iResult1 < iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::LessThanEqual:
      iResult = (iResult1 <= iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::Unequal:
      iResult = (iResult1 != iResult2) ? 1 : 0;
      return W_SUCCESS;
    case Comparison::None:
      WLog::Error(m_pLog, "Unknown operator");
      return W_FAILURE;
  }

  return W_FAILURE;
}
