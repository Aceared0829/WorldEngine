#include <GameEngineTest/GameEngineTestPCH.h>

#include "ProcGenTest.h"

#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Tasks/Utils.h>

static WGameEngineTestProcGen s_GameEngineTestProcGen;

const char* WGameEngineTestProcGen::GetTestName() const
{
  return "ProcGen Tests";
}

WGameEngineTestApplication* WGameEngineTestProcGen::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "ProcGen");
  return m_pOwnApplication;
}

void WGameEngineTestProcGen::SetupSubTests()
{
  AddSubTest("VertexColors", SubTests::VertexColors);
  AddSubTest("CurveNode", SubTests::CurveNode);
}

WResult WGameEngineTestProcGen::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::VertexColors)
  {
    m_ImgCompFrames.PushBack(1);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("ProcGen/AssetCache/Common/Scenes/VertexColors.WBinScene"));
    return W_SUCCESS;
  }
  else if (iIdentifier == SubTests::CurveNode)
  {
    InputVertex inputVertices[] = {
      {WVec3(0.0f), WVec3(0, 0, 1), WColor::White, 0},
      {WVec3(0.25f), WVec3(0, 0, 1), WColor::White, 1},
      {WVec3(0.5f), WVec3(0, 0, 1), WColor::White, 2},
      {WVec3(1.0f), WVec3(0, 0, 1), WColor::White, 3},
      {WVec3(2.0f), WVec3(0, 0, 1), WColor::White, 4},
    };

    WVec4 expectedOutputs[] = {
      WVec4(1.0f, 0, 0, 1),
      WVec4(0.375f, 1, 0.02f, 1),
      WVec4(0.0f, 0, 0.274f, 1),
      WVec4(1.0f, 0, 2, 1),
      WVec4(1.0f, 0, 2, 0),
    };

    W_SUCCEED_OR_RETURN(TestOutput(WMakeHashedString("CurveNodeTest"), WMakeArrayPtr(inputVertices), WMakeArrayPtr(expectedOutputs)));
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestProcGen::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  const bool bVulkan = WGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return WTestAppRun::Quit;
  }

  if (m_ImgCompFrames.IsEmpty())
  {
    return WTestAppRun::Quit;
  }

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    W_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}

WResult WGameEngineTestProcGen::DeInitializeTest()
{
  m_pVM.Clear();

  return SUPER::DeInitializeTest();
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
W_ALWAYS_INLINE WProcessingStream MakeStream(WArrayPtr<T> data, WUInt32 uiOffset, const WHashedString& sName, WProcessingStream::DataType dataType = WProcessingStream::DataType::Float)
{
  return WProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
}

WResult WGameEngineTestProcGen::TestOutput(const WHashedString& sOutputName, WArrayPtr<InputVertex> inputVertices, WArrayPtr<const WVec4> expectedOutputs)
{
  W_ASSERT_DEV(inputVertices.GetCount() == expectedOutputs.GetCount(), "Input and expected output count must match");

  // Data/ProcGenGraph.WProcGenGraphAsset
  WProcGenGraphResourceHandle hResource = WResourceManager::LoadResource<WProcGenGraphResource>("{ 11fe0278-f21e-4e05-9262-c836adeeef10 }");

  WResourceLock<WProcGenGraphResource> pResource(hResource, WResourceAcquireMode::BlockTillLoaded);
  if (pResource.GetAcquireResult() != WResourceAcquireResult::Final)
  {
    WLog::Error("Failed to load ProcGenGraphResource for testing");
    return W_FAILURE;
  }

  WSharedPtr<const WProcGenInternal::VertexColorOutput> pOutput;
  {
    auto& vcOutputs = pResource->GetVertexColorOutputs();
    for (auto& pVcOutput : vcOutputs)
    {
      if (pVcOutput->m_sName == sOutputName)
      {
        pOutput = pVcOutput;
        break;
      }
    }
    if (!pOutput)
    {
      WLog::Error("Failed to find VertexColorOutput '{0}' in ProcGenGraphResource", sOutputName);
      return W_FAILURE;
    }
  }

  if (m_pVM == nullptr)
  {
    m_pVM = W_DEFAULT_NEW(WExpressionVM);
    m_pVM->RegisterFunction(WExtendedExpressionFunctions::s_SampleCurveFunc);
  }

  WTempHybridArray<WProcessingStream, 8> inputs;
  {
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.x), WProcGenInternal::ExpressionInputs::s_sPositionX));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.y), WProcGenInternal::ExpressionInputs::s_sPositionY));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.z), WProcGenInternal::ExpressionInputs::s_sPositionZ));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.x), WProcGenInternal::ExpressionInputs::s_sNormalX));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.y), WProcGenInternal::ExpressionInputs::s_sNormalY));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.z), WProcGenInternal::ExpressionInputs::s_sNormalZ));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.r), WProcGenInternal::ExpressionInputs::s_sColorR));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.g), WProcGenInternal::ExpressionInputs::s_sColorG));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.b), WProcGenInternal::ExpressionInputs::s_sColorB));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.a), WProcGenInternal::ExpressionInputs::s_sColorA));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_uiIndex), WProcGenInternal::ExpressionInputs::s_sPointIndex, WProcessingStream::DataType::Int));
  }

  WTempHybridArray<WVec4, 16> m_TempData;
  m_TempData.SetCountUninitialized(inputVertices.GetCount());

  WTempHybridArray<WProcessingStream, 8> outputs;
  {
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WVec4, x), WProcGenInternal::ExpressionOutputs::s_sOutColorR));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WVec4, y), WProcGenInternal::ExpressionOutputs::s_sOutColorG));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WVec4, z), WProcGenInternal::ExpressionOutputs::s_sOutColorB));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(WVec4, w), WProcGenInternal::ExpressionOutputs::s_sOutColorA));
  }

  m_GlobalData.Clear();
  WProcGenGlobalData::SetCurves(*pOutput, m_GlobalData);

  W_SUCCEED_OR_RETURN(m_pVM->Execute(*(pOutput->m_pByteCode), inputs, outputs, inputVertices.GetCount(), m_GlobalData, WExpressionVM::Flags::BestPerformance));

  for (WUInt32 i = 0; i < m_TempData.GetCount(); ++i)
  {
    const WVec4& actual = m_TempData[i];
    const WVec4& expected = expectedOutputs[i];
    if (!actual.IsEqual(expected, 0.001f))
    {
      WLog::Error("Output value mismatch at index {}: Expected {}, but got {}", i, expected, actual);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}
