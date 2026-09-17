
/// Converts a windows HRESULT into an error code and a human-readable error message.
/// Pass in `GetLastError()` function or an HRESULT from another error source. Be careful when printing multiple values, a function could clear `GetLastError` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// \sa https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
struct WArgErrorCode
{
  inline explicit WArgErrorCode(WUInt32 uiErrorCode)
    : m_ErrorCode(uiErrorCode)
  {
  }

  WUInt32 m_ErrorCode;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgErrorCode& arg);
