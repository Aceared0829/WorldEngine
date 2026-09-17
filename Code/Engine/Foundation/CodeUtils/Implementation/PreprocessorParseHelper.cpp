#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace WTokenParseUtils;

WResult WPreprocessor::Expect(const TokenStream& Tokens, WUInt32& uiCurToken, WStringView sToken, WUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    WLog::Error(m_pLog, "Expected token '{0}', got empty token stream", sToken);
    return W_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken, pAccepted))
    return W_SUCCESS;

  const WUInt32 uiErrorToken = WMath::Min(Tokens.GetCount() - 1, uiCurToken);
  WString sErrorToken = Tokens[uiErrorToken]->m_DataView;
  PP_LOG(Error, "Expected token '{0}' got '{1}'", Tokens[uiErrorToken], sToken, sErrorToken);

  return W_FAILURE;
}

WResult WPreprocessor::Expect(const TokenStream& Tokens, WUInt32& uiCurToken, WTokenType::Enum Type, WUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    WLog::Error(m_pLog, "Expected token of type '{0}', got empty token stream", WTokenType::EnumNames[Type]);
    return W_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, Type, pAccepted))
    return W_SUCCESS;

  const WUInt32 uiErrorToken = WMath::Min(Tokens.GetCount() - 1, uiCurToken);
  PP_LOG(Error, "Expected token of type '{0}' got type '{1}' instead", Tokens[uiErrorToken], WTokenType::EnumNames[Type], WTokenType::EnumNames[Tokens[uiErrorToken]->m_iType]);

  return W_FAILURE;
}

WResult WPreprocessor::Expect(const TokenStream& Tokens, WUInt32& uiCurToken, WStringView sToken1, WStringView sToken2, WUInt32* pAccepted)
{
  if (Tokens.GetCount() < 2)
  {
    WLog::Error(m_pLog, "Expected tokens '{0}{1}', got empty token stream", sToken1, sToken2);
    return W_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken1, sToken2, pAccepted))
    return W_SUCCESS;

  const WUInt32 uiErrorToken = WMath::Min(Tokens.GetCount() - 2, uiCurToken);
  WString sErrorToken1 = Tokens[uiErrorToken]->m_DataView;
  WString sErrorToken2 = Tokens[uiErrorToken + 1]->m_DataView;
  PP_LOG(Error, "Expected tokens '{0}{1}', got '{2}{3}'", Tokens[uiErrorToken], sToken1, sToken2, sErrorToken1, sErrorToken2);

  return W_FAILURE;
}

WResult WPreprocessor::ExpectEndOfLine(const TokenStream& Tokens, WUInt32 uiCurToken)
{
  if (!IsEndOfLine(Tokens, uiCurToken, true))
  {
    WString sToken = Tokens[uiCurToken]->m_DataView;
    PP_LOG(Warning, "Expected end-of-line, found token '{0}'", Tokens[uiCurToken], sToken);
    return W_FAILURE;
  }

  return W_SUCCESS;
}
