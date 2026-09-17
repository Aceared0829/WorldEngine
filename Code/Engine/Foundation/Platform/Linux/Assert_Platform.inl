
bool WDefaultAssertHandler_Platform(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp)
{
  W_IGNORE_UNUSED(szSourceFile);
  W_IGNORE_UNUSED(uiLine);
  W_IGNORE_UNUSED(szFunction);
  W_IGNORE_UNUSED(szExpression);
  W_IGNORE_UNUSED(szAssertMsg);
  W_IGNORE_UNUSED(szTemp);

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}
