/// Many Linux APIs will fill out error on failure. This converts the error into an error code and a human-readable error message.
/// Pass in the linux `errno` symbol. Be careful when printing multiple values, a function could clear `errno` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// You may have to include #include <errno.h> use this.
/// \sa https://man7.org/linux/man-pages/man3/errno.3.html
struct WArgErrno
{
  inline explicit WArgErrno(WInt32 iErrno)
    : m_iErrno(iErrno)
  {
  }

  WInt32 m_iErrno;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgErrno& arg);

struct WArgErrorCode
{
  inline explicit WArgErrorCode(WUInt32 uiErrorCode)
    : m_ErrorCode(uiErrorCode)
  {
  }

  WUInt32 m_ErrorCode;
};

W_FOUNDATION_DLL WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgErrorCode& arg);
