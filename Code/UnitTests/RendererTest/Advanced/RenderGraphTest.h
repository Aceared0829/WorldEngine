#pragma once

#include "../TestClass/TestClass.h"
#include <Foundation/Types/SharedPtr.h>

class WRenderGraphTest : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "RenderGraph"; }

private:
  enum SubTests
  {
    ST_DeadPassCulling,
    ST_ResourceAliasing,
    ST_ImportReplace,
    ST_ExecuteCallbacks,
    ST_EmptyGraph,
    ST_StressTest,
    ST_MsaaResolve,
  };

  virtual void SetupSubTests() override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void DeadPassCulling();
  void ResourceAliasing();
  void ImportReplace();
  void ExecuteCallbacks();
  void EmptyGraph();
  void MsaaResolve();
  WTestAppRun StressTestRenderGraph(WUInt32 uiNumPasses);

private:
  WSharedPtr<WRenderGraph> m_pRenderGraph;
};
