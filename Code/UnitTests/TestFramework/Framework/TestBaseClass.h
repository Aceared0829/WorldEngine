#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>

struct WTestConfiguration;
class WImage;

class W_TEST_DLL WTestBaseClass : public WEnumerable<WTestBaseClass>
{
  friend class WTestFramework;

  W_DECLARE_ENUMERABLE_CLASS(WTestBaseClass);

public:
  // *** Override these functions to implement the required test functionality ***

  /// Override this function to give the test a proper name.
  virtual const char* GetTestName() const /*override*/ = 0;

  const char* GetSubTestName(WInt32 iIdentifier) const;

  /// Override this function to add additional information to the test configuration
  virtual void UpdateConfiguration(WTestConfiguration& ref_config) const /*override*/;

  /// Implement this to add support for image comparisons. See W_TEST_IMAGE_MSG.
  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) { return W_FAILURE; }

  /// Implement this to add support for depth buffer image comparisons. See W_TEST_DEPTH_IMAGE_MSG.
  virtual WResult GetDepthImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) { return W_FAILURE; }

  /// Used to map the 'number' for an image comparison, to a string used for finding the comparison image.
  ///
  /// By default image comparison screenshots are called 'TestName_SubTestName_XYZ'
  /// This can be fully overridden to use any other file name.
  /// The location of the comparison images (ie the folder) cannot be specified at the moment.
  virtual void MapImageNumberToString(const char* szTestName, const WSubTestEntry& subTest, WUInt32 uiImageNumber, WStringBuilder& out_sString) const;

protected:
  /// Called at startup to determine if the test can be run. Should return a detailed error message on failure.
  virtual std::string IsTestAvailable() const { return {}; };
  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual WResult InitializeTest() { return W_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual WResult DeInitializeTest() { return W_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual WResult InitializeSubTest(WInt32 iIdentifier) { return W_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) { return W_SUCCESS; }


  /// Adds a sub-test to the test suite. The index is used to identify it when running the sub-tests.
  void AddSubTest(const char* szName, WInt32 iIdentifier);

private:
  struct TestEntry
  {
    const char* m_szName = "";
    WInt32 m_iIdentifier = -1;
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  // Called by WTestFramework.
  WResult DoTestInitialization();
  void DoTestDeInitialization();
  WResult DoSubTestInitialization(WInt32 iIdentifier);
  void DoSubTestDeInitialization(WInt32 iIdentifier);
  WTestAppRun DoSubTestRun(WInt32 iIdentifier, double& fDuration, WUInt32 uiInvocationCount);

  // Finds internal entry index for identifier
  WInt32 FindEntryForIdentifier(WInt32 iIdentifier) const;

  std::deque<TestEntry> m_Entries;
};

#define W_CREATE_TEST(TestClass)
