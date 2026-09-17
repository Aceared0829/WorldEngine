#pragma once

#ifdef W_USE_QT

#  include <QObject>
#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

/// Derived WTestFramework which signals the GUI to update whenever a new tests result comes in.
class W_TEST_DLL WQtTestFramework : public QObject, public WTestFramework
{
  Q_OBJECT
public:
  WQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~WQtTestFramework();

private:
  WQtTestFramework(WQtTestFramework&);
  void operator=(WQtTestFramework&);

Q_SIGNALS:
  void TestResultReceived(qint32 testIndex, qint32 subTestIndex);

protected:
  virtual void OutputImpl(WTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration) override;
  virtual void SetSubTestStatusImpl(WUInt32 uiSubTestIndex, const char* szStatus) override;
};

#endif
