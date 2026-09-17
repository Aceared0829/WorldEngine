#pragma once

#include "../TestClass/TestClass.h"

class WRendererTestDynamicBuffer : public WGraphicsTest
{
  using SUPER = WGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Dynamic Buffer"; }

private:
  enum SubTests
  {
    ST_Allocations,
    ST_Deallocations,
    ST_Compaction,
    ST_ResizeWhileMapped,
  };

  virtual void SetupSubTests() override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

private:
  WGALDynamicBufferHandle m_hDynamicBuffer;
};
