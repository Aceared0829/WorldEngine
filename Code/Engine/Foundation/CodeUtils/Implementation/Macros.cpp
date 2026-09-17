#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace WTokenParseUtils;

void WPreprocessor::CopyTokensReplaceParams(const TokenStream& Source, WUInt32 uiFirstSourceToken, TokenStream& Destination, const WArrayPtr<WString>& parameters)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    WUInt32 i = uiFirstSourceToken;
    SkipWhitespace(Source, i);

    // add all the relevant tokens to the definition
    for (; i < Source.GetCount(); ++i)
    {
      if (Source[i]->m_iType == WTokenType::BlockComment || Source[i]->m_iType == WTokenType::LineComment || Source[i]->m_iType == WTokenType::EndOfFile || Source[i]->m_iType == WTokenType::Newline)
        continue;

      if (Source[i]->m_iType == WTokenType::Identifier)
      {
        for (WUInt32 p = 0; p < parameters.GetCount(); ++p)
        {
          if (Source[i]->m_DataView == parameters[p])
          {
            // create a custom token for the parameter, for better error messages
            WToken* pParamToken = AddCustomToken(Source[i], parameters[p]);
            pParamToken->m_iType = s_iMacroParameter0 + p;

            Destination.PushBack(pParamToken);
            goto tokenfound;
          }
        }
      }

      Destination.PushBack(Source[i]);

    tokenfound:
      continue;
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == WTokenType::Whitespace)
    Destination.PopBack();
}

WResult WPreprocessor::ExtractParameterName(const TokenStream& Tokens, WUInt32& uiCurToken, WString& sIdentifierName)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 2 < Tokens.GetCount() && Tokens[uiCurToken + 0]->m_DataView == "." && Tokens[uiCurToken + 1]->m_DataView == "." && Tokens[uiCurToken + 2]->m_DataView == ".")
  {
    sIdentifierName = "...";
    uiCurToken += 3;
  }
  else
  {
    WUInt32 uiParamToken = uiCurToken;

    if (Expect(Tokens, uiCurToken, WTokenType::Identifier, &uiParamToken).Failed())
      return W_FAILURE;

    sIdentifierName = Tokens[uiParamToken]->m_DataView;
  }

  // skip a trailing comma
  if (Accept(Tokens, uiCurToken, ","))
    SkipWhitespace(Tokens, uiCurToken);

  return W_SUCCESS;
}

WResult WPreprocessor::ExtractAllMacroParameters(const TokenStream& Tokens, WUInt32& uiCurToken, WDeque<TokenStream>& AllParameters)
{
  if (Expect(Tokens, uiCurToken, "(").Failed())
    return W_FAILURE;

  do
  {
    // add one parameter
    // note: we always add one extra parameter value, because MACRO() is actually a macro call with one empty parameter
    // the same for MACRO(a,) is a macro with two parameters, the second being empty
    AllParameters.SetCount(AllParameters.GetCount() + 1);

    if (ExtractParameterValue(Tokens, uiCurToken, AllParameters.PeekBack()).Failed())
      return W_FAILURE;

    // reached the end of the parameter list
    if (Accept(Tokens, uiCurToken, ")"))
      return W_SUCCESS;
  } while (Accept(Tokens, uiCurToken, ",")); // continue with the next parameter

  WString s = Tokens[uiCurToken]->m_DataView;
  PP_LOG(Error, "',' or ')' expected, got '{0}' instead", Tokens[uiCurToken], s);

  return W_FAILURE;
}

WResult WPreprocessor::ExtractParameterValue(const TokenStream& Tokens, WUInt32& uiCurToken, TokenStream& ParamTokens)
{
  SkipWhitespaceAndNewline(Tokens, uiCurToken);
  const WUInt32 uiFirstToken = WMath::Min(uiCurToken, Tokens.GetCount() - 1);

  WInt32 iParenthesis = 0;

  // get all tokens up until a comma or the last closing parenthesis
  // ignore commas etc. as long as they are surrounded with parenthesis
  for (; uiCurToken < Tokens.GetCount(); ++uiCurToken)
  {
    if (Tokens[uiCurToken]->m_iType == WTokenType::BlockComment || Tokens[uiCurToken]->m_iType == WTokenType::LineComment || Tokens[uiCurToken]->m_iType == WTokenType::Newline)
      continue;

    if (Tokens[uiCurToken]->m_iType == WTokenType::EndOfFile)
      break; // outputs an error

    if (iParenthesis == 0)
    {
      if (Tokens[uiCurToken]->m_DataView == "," || Tokens[uiCurToken]->m_DataView == ")")
      {
        if (!ParamTokens.IsEmpty() && ParamTokens.PeekBack()->m_iType == WTokenType::Whitespace)
        {
          ParamTokens.PopBack();
        }
        return W_SUCCESS;
      }
    }

    if (Tokens[uiCurToken]->m_DataView == "(")
      ++iParenthesis;
    else if (Tokens[uiCurToken]->m_DataView == ")")
      --iParenthesis;

    ParamTokens.PushBack(Tokens[uiCurToken]);
  }

  // reached the end of the stream without encountering the closing parenthesis first
  PP_LOG0(Error, "Unexpected end of file during macro parameter extraction", Tokens[uiFirstToken]);
  return W_FAILURE;
}

void WPreprocessor::StringifyTokens(const TokenStream& Tokens, WStringBuilder& sResult, bool bSurroundWithQuotes)
{
  WUInt32 uiCurToken = 0;

  sResult.Clear();

  if (bSurroundWithQuotes)
    sResult = "\"";

  WStringBuilder sTemp;

  SkipWhitespace(Tokens, uiCurToken);

  WUInt32 uiLastNonWhitespace = Tokens.GetCount();

  while (uiLastNonWhitespace > 0)
  {
    if (Tokens[uiLastNonWhitespace - 1]->m_iType != WTokenType::Whitespace && Tokens[uiLastNonWhitespace - 1]->m_iType != WTokenType::Newline && Tokens[uiLastNonWhitespace - 1]->m_iType != WTokenType::BlockComment && Tokens[uiLastNonWhitespace - 1]->m_iType != WTokenType::LineComment)
      break;

    --uiLastNonWhitespace;
  }

  for (WUInt32 t = uiCurToken; t < uiLastNonWhitespace; ++t)
  {
    // comments, newlines etc. are stripped out
    if ((Tokens[t]->m_iType == WTokenType::LineComment) || (Tokens[t]->m_iType == WTokenType::BlockComment) || (Tokens[t]->m_iType == WTokenType::Newline) || (Tokens[t]->m_iType == WTokenType::EndOfFile))
      continue;

    sTemp = Tokens[t]->m_DataView;

    // all whitespace becomes a single white space
    if (Tokens[t]->m_iType == WTokenType::Whitespace)
      sTemp = " ";

    // inside strings, all backslashes and double quotes are escaped
    if ((Tokens[t]->m_iType == WTokenType::String1) || (Tokens[t]->m_iType == WTokenType::String2))
    {
      sTemp.ReplaceAll("\\", "\\\\");
      sTemp.ReplaceAll("\"", "\\\"");
    }

    sResult.Append(sTemp.GetView());
  }

  if (bSurroundWithQuotes)
    sResult.Append("\"");
}
