#pragma once

#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <TestFramework/Utilities/TestLogInterface.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestProcGen : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    VertexColors,
    CurveNode,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  virtual WResult DeInitializeTest() override;

  using InputVertex = WProcGenInternal::VertexColorTask::InputVertex;
  WResult TestOutput(const WHashedString& sOutputName, WArrayPtr<InputVertex> inputVertices, WArrayPtr<const WVec4> expectedOutputs);

  WInt32 m_iFrame = 0;
  WGameEngineTestApplication* m_pOwnApplication = nullptr;

  WUInt32 m_uiImgCompIdx = 0;
  WHybridArray<WUInt32, 8> m_ImgCompFrames;

  WExpression::GlobalData m_GlobalData;
  WUniquePtr<WExpressionVM> m_pVM;
};
