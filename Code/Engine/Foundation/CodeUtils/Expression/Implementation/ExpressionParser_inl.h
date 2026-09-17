
inline bool WExpressionParser::AcceptStatementTerminator()
{
  return WTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, WTokenType::Newline) ||
         WTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, ";");
}

inline WResult WExpressionParser::Expect(WStringView sToken, const WToken** pExpectedToken)
{
  WUInt32 uiAcceptedToken = 0;
  if (WTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, sToken, &uiAcceptedToken) == false)
  {
    const WUInt32 uiErrorToken = WMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, WFmt("Syntax error, expected {} but got {}", sToken, pToken->m_DataView));
    return W_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return W_SUCCESS;
}

inline WResult WExpressionParser::Expect(WTokenType::Enum Type, const WToken** pExpectedToken /*= nullptr*/)
{
  WUInt32 uiAcceptedToken = 0;
  if (WTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, Type, &uiAcceptedToken) == false)
  {
    const WUInt32 uiErrorToken = WMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, WFmt("Syntax error, expected token type {} but got {}", WTokenType::EnumNames[Type], WTokenType::EnumNames[pToken->m_iType]));
    return W_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return W_SUCCESS;
}

inline void WExpressionParser::ReportError(const WToken* pToken, const WFormatString& message0)
{
  WStringBuilder tmp;
  WStringView message = message0.GetText(tmp);
  WLog::Error("{}({},{}): {}", pToken->m_File, pToken->m_uiLine, pToken->m_uiColumn, message);
}
