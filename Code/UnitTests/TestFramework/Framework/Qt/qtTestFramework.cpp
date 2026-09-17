#include <TestFramework/TestFrameworkPCH.h>

#ifdef W_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#    include <combaseapi.h>
#  endif

////////////////////////////////////////////////////////////////////////
// WQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

WQtTestFramework::WQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : WTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  // Why this is needed: We use the DirectXTex library which calls GetWICFactory to create its factory singleton. If CoUninitialize is called, this pointer is deleted and another access would crash the application. To prevent this, we take the first reference to CoInitializeEx to ensure that nobody else can init+deinit COM and subsequently corrupt the DirectXTex library. As we init+deinit qt for each test, the first test that does an image comparison will init GetWICFactory and the qt deinit would destroy the WICFactory pointer. The next test that uses image comparison would then trigger the crash.
  HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  W_ASSERT_DEV(SUCCEEDED(res), "CoInitializeEx failed with: {}", WArgErrorCode(res));
#  endif

  Q_INIT_RESOURCE(resources);
  Initialize();
}

WQtTestFramework::~WQtTestFramework()
{
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  CoUninitialize();
#  endif
}


////////////////////////////////////////////////////////////////////////
// WQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void WQtTestFramework::OutputImpl(WTestOutput::Enum Type, const char* szMsg)
{
  WTestFramework::OutputImpl(Type, szMsg);
}

void WQtTestFramework::TestResultImpl(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  WTestFramework::TestResultImpl(uiSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}


void WQtTestFramework::SetSubTestStatusImpl(WUInt32 uiSubTestIndex, const char* szStatus)
{
  WTestFramework::SetSubTestStatusImpl(uiSubTestIndex, szStatus);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}

#endif
