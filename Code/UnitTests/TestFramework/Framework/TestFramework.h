#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/TestResults.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <TestFrameworkEntryPoint_Platform.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

class WCommandLineUtils;

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class W_TEST_DLL WTestFramework
{
public:
  WTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~WTestFramework();

  using OutputHandler = void (*)(WTestOutput::Enum, const char*);

  // Test management
  void CreateOutputFolder();
  void UpdateReferenceImages();
  const char* GetTestName() const;
  const char* GetAbsOutputPath() const;
  const char* GetRelTestDataPath() const;
  const char* GetAbsTestOrderFilePath() const;
  const char* GetAbsTestSettingsFilePath() const;
  void RegisterOutputHandler(OutputHandler handler);
  void GatherAllTests();
  void LoadTestOrder();
  void ApplyTestOrderFromCommandLine(const WCommandLineUtils& cmd);
  void LoadTestSettings();
  void AutoSaveTestOrder();
  void SaveTestOrder(const char* const szFilePath);
  void SaveTestSettings(const char* const szFilePath);
  void SetAllTestsEnabledStatus(bool bEnable);
  void SetAllFailedTestsEnabledStatus();
  // Each function on a test must not take longer than the given time or the test process will be terminated.
  void SetTestTimeout(WUInt32 uiTestTimeoutMS);
  WUInt32 GetTestTimeout() const;
  void GetTestSettingsFromCommandLine(const WCommandLineUtils& cmd);

  // Test execution
  void ResetTests();
  WTestAppRun RunTestExecutionLoop();

  /// Top-level function to run tests, can be overridden by platform specific implementations
  virtual WTestAppRun RunTests() { return RunTestExecutionLoop(); }

  void StartTests();
  void ExecuteNextTest();
  void EndTests();
  void AbortTests();

  // Test queries
  WUInt32 GetTestCount() const;
  WUInt32 GetTestEnabledCount() const;
  WUInt32 GetSubTestEnabledCount(WUInt32 uiTestIndex) const;
  const std::string& IsTestAvailable(WUInt32 uiTestIndex) const;
  bool IsTestEnabled(WUInt32 uiTestIndex) const;
  bool IsSubTestEnabled(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex) const;
  void SetTestEnabled(WUInt32 uiTestIndex, bool bEnabled);
  void SetSubTestEnabled(WUInt32 uiTestIndex, WUInt32 uiSubTestIndex, bool bEnabled);

  WUInt32 GetCurrentTestIndex() const { return m_uiCurrentTestIndex; }
  WUInt32 GetCurrentSubTestIndex() const { return m_uiCurrentSubTestIndex; }
  WInt32 GetCurrentSubTestIdentifier() const;

  /// Returns the index of the sub-test with the given identifier.
  ///
  /// Only looks at the currently running test, assuming that the identifier is unique among its sub-tests.
  WUInt32 FindSubTestIndexForSubTestIdentifier(WInt32 iSubTestIdentifier) const;

  WTestEntry* GetTest(WUInt32 uiTestIndex);
  const WTestEntry* GetTest(WUInt32 uiTestIndex) const;
  bool GetTestsRunning() const { return m_bTestsRunning; }

  const WTestEntry* GetCurrentTest() const;
  const WSubTestEntry* GetCurrentSubTest() const;

  // Global settings
  TestSettings GetSettings() const;
  void SetSettings(const TestSettings& settings);

  // Test results
  WTestFrameworkResult& GetTestResult();
  WInt32 GetTotalErrorCount() const;
  WInt32 GetTestsPassedCount() const;
  WInt32 GetTestsFailedCount() const;
  double GetTotalTestDuration() const;

  // Image comparison
  void ScheduleImageComparison(WUInt32 uiImageNumber, WUInt32 uiMaxError);
  void ScheduleDepthImageComparison(WUInt32 uiImageNumber, WUInt32 uiMaxError);
  bool IsImageComparisonScheduled() const { return m_bImageComparisonScheduled; }
  bool IsDepthImageComparisonScheduled() const { return m_bDepthImageComparisonScheduled; }
  void GenerateComparisonImageName(WUInt32 uiImageNumber, WStringBuilder& ref_sImgName);
  void GetCurrentComparisonImageName(WStringBuilder& ref_sImgName);
  void SetImageReferenceFolderName(const char* szFolderName);

  /// Registers a tag that will be used as a filename suffix when looking for reference images.
  ///
  /// During image comparison, after trying the base image name, all registered tags and their combinations
  /// are tried as suffixes (e.g. "image-amd.png", "image-vulkan.png", "image-amd-vulkan.png").
  /// Call ClearImageReferenceTags() first when reinitializing (e.g. after device setup).
  void AddImageReferenceTag(const char* szTag);

  /// Removes all previously registered image reference tags.
  void ClearImageReferenceTags();

  /// Derives and sets the appropriate image reference tags from platform, renderer, and GPU adapter name.
  ///
  /// Clears any previously set tags and adds tags for the current environment, such as the platform name, if it differs from
  /// windows ("linux", "osx", "android"), the renderer ("vulkan"), and GPU vendor ("amd", "nvidia", "intel"),
  /// or special renderer variants ("d3dref", "llvmpipe", "swiftshader").
  /// Pass empty strings for \a sRenderer and \a sAdapterName when not using a renderer.
  void SetImageReferenceTagsFromEnvironment(WStringView sPlatform, WStringView sRenderer, WStringView sAdapterName);

  /// Writes an Html file that contains test information and an image diff view for failed image comparisons.
  void WriteImageDiffHtml(const char* szFileName, const WImage& referenceImgRgb, const WImage& referenceImgAlpha, const WImage& capturedImgRgb, const WImage& capturedImgAlpha, const WImage& diffImgRgb, const WImage& diffImgAlpha, WUInt32 uiError, WUInt32 uiThreshold, WUInt8 uiMinDiffRgb,
    WUInt8 uiMaxDiffRgb, WUInt8 uiMinDiffAlpha, WUInt8 uiMaxDiffAlpha);

  bool PerformImageComparison(WStringBuilder sImgName, const WImage& img, WUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg);
  bool CompareImages(WUInt32 uiImageNumber, WUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage = false, bool bIsLineImage = false);

  /// A function to be called to add extra info to image diff output, that is not available from here.
  /// E.g. device specific info like driver version.
  using ImageDiffExtraInfoCallback = std::function<WDynamicArray<std::pair<WString, WString>>()>;
  void SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider);

  using ImageComparisonCallback = std::function<void(bool)>; /// A function to be called after every image comparison with a bool
                                                             /// indicating if the images matched or not.
  void SetImageComparisonCallback(const ImageComparisonCallback& callback);

  static WResult CaptureRegressionStat(WStringView sTestName, WStringView sName, WStringView sUnit, float value, WInt32 iTestId = -1);

protected:
  void Initialize();
  void DeInitialize();

  /// Will be called for test failures to record the location of the failure and forward the error to OutputImpl.
  virtual void ErrorImpl(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg);
  /// Receives WLog messages (via LogWriter) as well as test-framework internal logging. Any WTestOutput::Error will
  /// cause the test to fail.
  virtual void OutputImpl(WTestOutput::Enum Type, const char* szMsg);
  virtual void TestResultImpl(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  virtual void SetSubTestStatusImpl(WUInt32 uiSubTestIndex, const char* szStatus);
  void FlushAsserts();
  void TimeoutThread();
  void UpdateTestTimeout();

  // ignore this for now
public:
  static const char* s_szTestBlockName;
  static int s_iAssertCounter;
  static bool s_bCallstackOnAssert;
  static WLog::TimestampMode s_LogTimestampMode;

  // static functions
public:
  static W_ALWAYS_INLINE WTestFramework* GetInstance() { return s_pInstance; }

  /// Returns whether to assert on test failure.
  static bool GetAssertOnTestFail();

  static void Output(WTestOutput::Enum type, const char* szMsg, ...);
  static void OutputArgs(WTestOutput::Enum type, const char* szMsg, va_list szArgs);
  static void Error(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, WStringView sMsg, ...);
  static void Error(const char* szError, const char* szFile, WInt32 iLine, const char* szFunction, WStringView sMsg, va_list szArgs);
  static void TestResult(WUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  static void SetSubTestStatus(WUInt32 uiSubTestIndex, const char* szStatus);

  // static members
private:
  static WTestFramework* s_pInstance;

private:
  std::string m_sTestName;                ///< The name of the tests being done
  std::string m_sAbsTestOutputDir;        ///< Absolute path to the output folder where results and temp data is stored
  std::string m_sRelTestDataDir;          ///< Relative path from the SDK to where the unit test data is located
  std::string m_sAbsTestOrderFilePath;    ///< Absolute path to the test order file
  std::string m_sAbsTestSettingsFilePath; ///< Absolute path to the test settings file
  WInt32 m_iErrorCount = 0;
  WInt32 m_iTestsFailed = 0;
  WInt32 m_iTestsPassed = 0;
  TestSettings m_Settings;
  std::recursive_mutex m_OutputMutex;
  std::deque<OutputHandler> m_OutputHandlers;
  std::deque<WTestEntry> m_TestEntries;
  WTestFrameworkResult m_Result;
  WAssertHandler m_PreviousAssertHandler = nullptr;
  ImageDiffExtraInfoCallback m_ImageDiffExtraInfoCallback;
  ImageComparisonCallback m_ImageComparisonCallback;

  std::mutex m_TimeoutLock;
  WUInt32 m_uiTimeoutMS = 5 * 60 * 1000; // 5 min default timeout
  bool m_bUseTimeout = false;
  bool m_bArm = false;
  std::condition_variable m_TimeoutCV;
  std::thread m_TimeoutThread;

  WUInt32 m_uiExecutingTest = 0;
  WUInt32 m_uiExecutingSubTest = 0;
  bool m_bSubTestInitialized = false;
  bool m_bAbortTests = false;
  double m_fTotalTestDuration = 0.0;
  double m_fTotalSubTestDuration = 0.0;
  WInt32 m_iErrorCountBeforeTest = 0;
  WUInt32 m_uiSubTestInvocationCount = 0;

  bool m_bIsInitialized = false;

  // image comparisons
  bool m_bImageComparisonScheduled = false;
  WUInt32 m_uiMaxImageComparisonError = 0;
  WUInt32 m_uiComparisonImageNumber = 0;

  bool m_bDepthImageComparisonScheduled = false;
  WUInt32 m_uiMaxDepthImageComparisonError = 0;
  WUInt32 m_uiComparisonDepthImageNumber = 0;

  std::string m_sImageReferenceFolderName = "Images_Reference";
  WDynamicArray<WString> m_ImageReferenceTags;

protected:
  WUInt32 m_uiCurrentTestIndex = WInvalidIndex;
  WUInt32 m_uiCurrentSubTestIndex = WInvalidIndex;
  bool m_bTestsRunning = false;
};

/// Enum for usage in W_TEST_BLOCK to enable or disable the block.
struct WTestBlock
{
  /// Enum for usage in W_TEST_BLOCK to enable or disable the block.
  enum Enum
  {
    Enabled,           ///< The test block is enabled.
    Disabled,          ///< The test block will be skipped. The test framework will print a warning message, that some block is deactivated.
    DisabledNoWarning, ///< The test block will be skipped, but no warning printed. Used to deactivate 'on demand/optional' tests.
  };
};

#define safeprintf WStringUtils::snprintf

/// Starts a small test block inside a larger test.
///
/// First parameter allows to quickly disable a block depending on a condition (e.g. platform).
/// Second parameter just gives it a name for better error reporting.
/// Also skipped tests are highlighted in the output, such that people can quickly see when a test is currently deactivated.
#define W_TEST_BLOCK(enable, name)                                                  \
  WTestFramework::s_szTestBlockName = name;                                         \
  if (enable == WTestBlock::Disabled)                                               \
  {                                                                                  \
    WTestFramework::s_szTestBlockName = "";                                         \
    WTestFramework::Output(WTestOutput::Warning, "Skipped Test Block '%s'", name); \
  }                                                                                  \
  else if (enable == WTestBlock::DisabledNoWarning)                                 \
  {                                                                                  \
    WTestFramework::s_szTestBlockName = "";                                         \
  }                                                                                  \
  else


/// Will trigger a debug break, if the test framework is configured to do so on test failure
#define W_TEST_DEBUG_BREAK                   \
  if (WTestFramework::GetAssertOnTestFail()) \
  W_DEBUG_BREAK

#define W_TEST_FAILURE(erroroutput, msg, ...)                                                                   \
  {                                                                                                              \
    WTestFramework::Error(erroroutput, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__); \
    W_TEST_DEBUG_BREAK                                                                                          \
  }

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestBool(
  bool bCondition, const char* szErrorText, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define W_TEST_BOOL(condition) W_TEST_BOOL_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define W_TEST_BOOL_MSG(condition, msg, ...) \
  WTestBool(condition, "Test failed: " W_PP_STRINGIFY(condition), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestResult(
  WResult condition, const char* szErrorText, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define W_TEST_RESULT(condition) W_TEST_RESULT_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define W_TEST_RESULT_MSG(condition, msg, ...) \
  WTestResult(condition, "Test failed: " W_PP_STRINGIFY(condition), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestResult(
  WResult condition, const char* szErrorText, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests for a boolean condition, does not output an extra message.
#define W_TEST_RESULT(condition) W_TEST_RESULT_MSG(condition, "")

/// Tests for a boolean condition, outputs a custom message on failure.
#define W_TEST_RESULT_MSG(condition, msg, ...) \
  WTestResult(condition, "Test failed: " W_PP_STRINGIFY(condition), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests for a WStatus condition, outputs WStatus message on failure
#define W_TEST_STATUS(condition)                    \
  auto W_PP_CONCAT(l_, W_SOURCE_LINE) = condition; \
  WTestResult(W_PP_CONCAT(l_, W_SOURCE_LINE).GetResult(), "Test failed: " W_PP_STRINGIFY(condition), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, W_PP_CONCAT(l_, W_SOURCE_LINE).GetMessageString())

inline double ToFloat(int f)
{
  return static_cast<double>(f);
}

inline double ToFloat(float f)
{
  return static_cast<double>(f);
}

inline double ToFloat(double f)
{
  return static_cast<double>(f);
}

W_TEST_DLL bool WTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, WInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output.
#define W_TEST_FLOAT(f1, f2, epsilon) W_TEST_FLOAT_MSG(f1, f2, epsilon, "")

/// Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define W_TEST_FLOAT_MSG(f1, f2, epsilon, msg, ...)                                                                                                     \
  WTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), W_PP_STRINGIFY(f1), W_PP_STRINGIFY(f2), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////

/// Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output.
#define W_TEST_DOUBLE(f1, f2, epsilon) W_TEST_DOUBLE_MSG(f1, f2, epsilon, "")

/// Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define W_TEST_DOUBLE_MSG(f1, f2, epsilon, msg, ...)                                                                                                    \
  WTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), W_PP_STRINGIFY(f1), W_PP_STRINGIFY(f2), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestInt(
  WInt64 i1, WInt64 i2, const char* szI1, const char* szI2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two ints for equality. On failure both actual and expected values are output.
#define W_TEST_INT(i1, i2) W_TEST_INT_MSG(i1, i2, "")

/// Tests two ints for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_INT_MSG(i1, i2, msg, ...) \
  WTestInt(i1, i2, W_PP_STRINGIFY(i1), W_PP_STRINGIFY(i2), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestString(WStringView s1, WStringView s2, const char* szString1, const char* szString2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two strings for equality. On failure both actual and expected values are output.
#define W_TEST_STRING(i1, i2) W_TEST_STRING_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_STRING_MSG(s1, s2, msg, ...)                                                                                                           \
  WTestString(static_cast<WStringView>(s1), static_cast<WStringView>(s2), W_PP_STRINGIFY(s1), W_PP_STRINGIFY(s2), W_SOURCE_FILE, W_SOURCE_LINE, \
    W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestWString(std::wstring s1, std::wstring s2, const char* szString1, const char* szString2, const char* szFile, WInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// Tests two strings for equality. On failure both actual and expected values are output.
#define W_TEST_WSTRING(i1, i2) W_TEST_WSTRING_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_WSTRING_MSG(s1, s2, msg, ...)                                                                                               \
  WTestWString(static_cast<const wchar_t*>(s1), static_cast<const wchar_t*>(s2), W_PP_STRINGIFY(s1), W_PP_STRINGIFY(s2), W_SOURCE_FILE, \
    W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two strings for equality. On failure both actual and expected values are output. Does not embed the original expression to
/// work around issues with the current code page and unicode literals.
#define W_TEST_STRING_UNICODE(i1, i2) W_TEST_STRING_UNICODE_MSG(i1, i2, "")

/// Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed. Does not
/// embed the original expression to work around issues with the current code page and unicode literals.
#define W_TEST_STRING_UNICODE_MSG(s1, s2, msg, ...) \
  WTestString(                                      \
    static_cast<const char*>(s1), static_cast<const char*>(s2), "", "", W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestVector(
  WVec4d v1, WVec4d v2, double fEps, const char* szCondition, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Tests two WVec2's for equality, using some epsilon. On failure both actual and expected values are output.
#define W_TEST_VEC2(i1, i2, epsilon) W_TEST_VEC2_MSG(i1, i2, epsilon, "")

/// Tests two WVec2's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_VEC2_MSG(r1, r2, epsilon, msg, ...)                                                                                \
  WTestVector(WVec4d(ToFloat((r1).x), ToFloat((r1).y), 0, 0), WVec4d(ToFloat((r2).x), ToFloat((r2).y), 0, 0), ToFloat(epsilon), \
    W_PP_STRINGIFY(r1) " == " W_PP_STRINGIFY(r2), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two WVec3's for equality, using some epsilon. On failure both actual and expected values are output.
#define W_TEST_VEC3(i1, i2, epsilon) W_TEST_VEC3_MSG(i1, i2, epsilon, "")

/// Tests two WVec3's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_VEC3_MSG(r1, r2, epsilon, msg, ...)                                                                                          \
  WTestVector(WVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), 0), WVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), 0), \
    ToFloat(epsilon), W_PP_STRINGIFY(r1) " == " W_PP_STRINGIFY(r2), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// Tests two WVec4's for equality, using some epsilon. On failure both actual and expected values are output.
#define W_TEST_VEC4(i1, i2, epsilon) W_TEST_VEC4_MSG(i1, i2, epsilon, "")

/// Tests two WVec4's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define W_TEST_VEC4_MSG(r1, r2, epsilon, msg, ...)                                                                                                \
  WTestVector(WVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), ToFloat((r1).w)),                                                        \
    WVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), ToFloat((r2).w)), ToFloat(epsilon), W_PP_STRINGIFY(r1) " == " W_PP_STRINGIFY(r2), \
    W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestFiles(
  const char* szFile1, const char* szFile2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define W_TEST_FILES(szFile1, szFile2, msg, ...) \
  WTestFiles(szFile1, szFile2, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

W_TEST_DLL bool WTestTextFiles(
  const char* szFile1, const char* szFile2, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define W_TEST_TEXT_FILES(szFile1, szFile2, msg, ...) \
  WTestTextFiles(szFile1, szFile2, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

W_TEST_DLL bool WTestImage(
  WUInt32 uiImageNumber, WUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, WInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// Same as W_TEST_IMAGE_MSG but uses an empty error message.
#define W_TEST_IMAGE(ImageNumber, MaxError) W_TEST_IMAGE_MSG(ImageNumber, MaxError, "")

/// Same as W_TEST_DEPTH_IMAGE_MSG but uses an empty error message.
#define W_TEST_DEPTH_IMAGE(ImageNumber, MaxError) W_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, "")

/// Same as W_TEST_LINE_IMAGE_MSG but uses an empty error message.
#define W_TEST_LINE_IMAGE(ImageNumber, MaxError) W_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, "")

/// Executes an image comparison right now.
///
/// The reference image is read from disk.
/// The path to the reference image is constructed from the test and sub-test name and the 'ImageNumber'.
/// One can, for instance, use the 'invocation count' that is passed to WTestBaseClass::RunSubTest() as the ImageNumber,
/// but any other integer is fine as well.
///
/// The current image to compare is taken from WTestBaseClass::GetImage().
/// Rendering tests typically override this function to return the result of the currently rendered frame.
///
/// 'MaxError' specifies the maximum mean-square error that is still considered acceptable
/// between the reference image and the current image.
///
/// Use the * DEPTH * variant if a depth buffer comparison should be requested.
///
/// \note Some tests need to know at the start, whether an image comparison will be done at the end, so they
/// can capture the image first. For such use cases, use W_SCHEDULE_IMAGE_TEST at the start of a sub-test instead.
#define W_TEST_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  WTestImage(ImageNumber, MaxError, false, false, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

#define W_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  WTestImage(ImageNumber, MaxError, true, false, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// Same as W_TEST_IMAGE_MSG, but allows for pixels to shift in a 1-pixel radius to account for different line rasterization of GPU vendors.
#define W_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  WTestImage(ImageNumber, MaxError, false, true, W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// Schedules an W_TEST_IMAGE to be executed after the current sub-test execution finishes.
///
/// Call this at the beginning of a sub-test, to automatically execute an image comparison when it is finished.
/// Calling WTestFramework::IsImageComparisonScheduled() will now return true.
///
/// To support image comparisons, tests derived from WTestBaseClass need to provide the current image through WTestBaseClass::GetImage().
/// To support 'scheduled' image comparisons, the class should poll WTestFramework::IsImageComparisonScheduled() every step and capture the
/// image when needed.
///
/// Use the * DEPTH * variant if a depth buffer comparison is intended.
///
/// \note Scheduling image comparisons is an optimization to only capture data when necessary, instead of capturing it every single frame.
#define W_SCHEDULE_IMAGE_TEST(ImageNumber, MaxError) WTestFramework::GetInstance()->ScheduleImageComparison(ImageNumber, MaxError);

#define W_SCHEDULE_DEPTH_IMAGE_TEST(ImageNumber, MaxError) WTestFramework::GetInstance()->ScheduleDepthImageComparison(ImageNumber, MaxError);
