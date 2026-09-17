#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererTest/Advanced/GpuPipelineTest.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  using Connectivity = WRenderPipelinePinConnection::Connectivity;

  struct RecordedPin
  {
    Connectivity m_Connectivity = Connectivity::None;
    WUInt32 m_uiHandleId = 0;
  };

  struct RecordedPass
  {
    WString m_sName;
    WHybridArray<RecordedPin, 4> m_Inputs;
    WHybridArray<RecordedPin, 4> m_Outputs;
  };

  WDynamicArray<RecordedPass>* s_pExecutionOrder = nullptr;

  RecordedPin MakeRecordedPin(const WRenderPipelinePinConnection& connection)
  {
    RecordedPin pin;
    pin.m_Connectivity = connection.m_Connectivity;
    if (connection.m_Connectivity == Connectivity::Texture)
      pin.m_uiHandleId = connection.m_TextureHandle.GetInternalID().m_Data;
    else if (connection.m_Connectivity == Connectivity::Buffer)
      pin.m_uiHandleId = connection.m_BufferHandle.GetInternalID().m_Data;

    return pin;
  }

  void RecordPass(WStringView sName, WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<const WRenderPipelinePinConnection> outputs)
  {
    RecordedPass& recorded = s_pExecutionOrder->ExpandAndGetRef();
    recorded.m_sName = sName;

    for (const WRenderPipelinePinConnection& connection : inputs)
    {
      recorded.m_Inputs.PushBack(MakeRecordedPin(connection));
    }
    for (const WRenderPipelinePinConnection& connection : outputs)
    {
      recorded.m_Outputs.PushBack(MakeRecordedPin(connection));
    }
  }

  class WGpuPipelineTestSourcePass : public WRenderPipelinePass
  {
    W_ADD_DYNAMIC_REFLECTION(WGpuPipelineTestSourcePass, WRenderPipelinePass);

  public:
    WGpuPipelineTestSourcePass()
      : WRenderPipelinePass("Source", true)
    {
    }

    // Creates a real resource per output so that pass-through forwarding can be verified by handle identity.
    virtual WStatus AddRenderPasses(const WViewData&, const WCamera&, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override
    {
      for (WRenderPipelinePinConnection& output : outputs)
      {
        if (output.m_Connectivity == Connectivity::Texture)
        {
          WGALTextureCreationDescription desc;
          desc.SetAsRenderTarget(4, 4, WGALResourceFormat::RGBAUByteNormalized);
          output = WRenderPipelinePinConnection(Connectivity::Texture, ref_graph.CreateTexture(desc));
        }
        else if (output.m_Connectivity == Connectivity::Buffer)
        {
          WGALBufferCreationDescription desc;
          desc.m_uiTotalSize = 256;
          output = WRenderPipelinePinConnection(Connectivity::Buffer, ref_graph.CreateBuffer(desc));
        }
      }

      RecordPass(GetName(), inputs, outputs);
      return W_SUCCESS;
    }

    WRenderPipelineNodeOutputPin m_Output;
    WRenderPipelineNodeBufferOutputPin m_BufferOutput;
  };

  class WGpuPipelineTestPass : public WRenderPipelinePass
  {
    W_ADD_DYNAMIC_REFLECTION(WGpuPipelineTestPass, WRenderPipelinePass);

  public:
    WGpuPipelineTestPass()
      : WRenderPipelinePass("Pass", true)
    {
    }

    virtual WStatus AddRenderPasses(const WViewData&, const WCamera&, WRenderGraph&, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return W_SUCCESS;
    }

    WRenderPipelineNodeInputPin m_Input;
    WRenderPipelineNodeOutputPin m_Output;
    WRenderPipelineNodeBufferInputPin m_BufferInput;
    WRenderPipelineNodeBufferOutputPin m_BufferOutput;
  };

  class WGpuPipelineTestSinkPass : public WRenderPipelinePass
  {
    W_ADD_DYNAMIC_REFLECTION(WGpuPipelineTestSinkPass, WRenderPipelinePass);

  public:
    WGpuPipelineTestSinkPass()
      : WRenderPipelinePass("Sink", true)
    {
    }

    virtual WStatus AddRenderPasses(const WViewData&, const WCamera&, WRenderGraph&, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return W_SUCCESS;
    }

    WRenderPipelineNodeInputPin m_InputA;
    WRenderPipelineNodeInputPin m_InputB;
    WRenderPipelineNodeBufferInputPin m_BufferInputA;
    WRenderPipelineNodeBufferInputPin m_BufferInputB;
    WInt32 m_iValue = 0;
  };

  class WGpuPipelineTestPassThroughPass : public WRenderPipelinePass
  {
    W_ADD_DYNAMIC_REFLECTION(WGpuPipelineTestPassThroughPass, WRenderPipelinePass);

  public:
    WGpuPipelineTestPassThroughPass()
      : WRenderPipelinePass("PassThrough", true)
    {
    }

    virtual WStatus AddRenderPasses(const WViewData&, const WCamera&, WRenderGraph&, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return W_SUCCESS;
    }

    WRenderPipelineNodePassThroughPin m_Pin;
    WRenderPipelineNodeBufferPassThroughPin m_BufferPin;
  };

  // clang-format off
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGpuPipelineTestSourcePass, 1, WRTTIDefaultAllocator<WGpuPipelineTestSourcePass>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Output", m_Output),
      W_MEMBER_PROPERTY("BufferOutput", m_BufferOutput),
    }
    W_END_PROPERTIES;
  }
  W_END_DYNAMIC_REFLECTED_TYPE;

  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGpuPipelineTestPass, 1, WRTTIDefaultAllocator<WGpuPipelineTestPass>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Input", m_Input),
      W_MEMBER_PROPERTY("Output", m_Output),
      W_MEMBER_PROPERTY("BufferInput", m_BufferInput),
      W_MEMBER_PROPERTY("BufferOutput", m_BufferOutput),
    }
    W_END_PROPERTIES;
  }
  W_END_DYNAMIC_REFLECTED_TYPE;

  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGpuPipelineTestSinkPass, 1, WRTTIDefaultAllocator<WGpuPipelineTestSinkPass>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("InputA", m_InputA),
      W_MEMBER_PROPERTY("InputB", m_InputB),
      W_MEMBER_PROPERTY("BufferInputA", m_BufferInputA),
      W_MEMBER_PROPERTY("BufferInputB", m_BufferInputB),
      W_MEMBER_PROPERTY("Value", m_iValue),
    }
    W_END_PROPERTIES;
  }
  W_END_DYNAMIC_REFLECTED_TYPE;

  W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGpuPipelineTestPassThroughPass, 1, WRTTIDefaultAllocator<WGpuPipelineTestPassThroughPass>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Pin", m_Pin),
      W_MEMBER_PROPERTY("BufferPin", m_BufferPin),
    }
    W_END_PROPERTIES;
  }
  W_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  template <typename PassType>
  WUInt32 AddPass(WDynamicArray<WUniquePtr<WRenderPipelinePass>>& ref_passes, const char* szName)
  {
    WUniquePtr<PassType> pPass = W_DEFAULT_NEW(PassType);
    pPass->SetName(szName);
    const WUInt32 uiIndex = ref_passes.GetCount();
    ref_passes.PushBack(std::move(pPass));
    return uiIndex;
  }

  void Connect(WDynamicArray<WRenderPipelineResourceLoaderConnection>& ref_connections, WUInt32 uiSource, const char* szSourcePin, WUInt32 uiTarget, const char* szTargetPin)
  {
    auto& connection = ref_connections.ExpandAndGetRef();
    connection.m_uiSource = uiSource;
    connection.m_uiTarget = uiTarget;
    connection.m_sSourcePin = szSourcePin;
    connection.m_sTargetPin = szTargetPin;
  }

  WUniquePtr<WRenderPipelinePassGraph> CreatePipeline(WDynamicArray<WUniquePtr<WRenderPipelinePass>>&& passes, WDynamicArray<WRenderPipelineResourceLoaderConnection>& ref_connections)
  {
    WDynamicArray<WUniquePtr<WExtractor>> extractors;
    return W_DEFAULT_NEW(WRenderPipelinePassGraph, std::move(passes), std::move(extractors), ref_connections);
  }

  void CompileAndExecute(WRenderPipelinePassGraph& ref_pipeline, WRenderGraph& ref_graph, WDynamicArray<RecordedPass>& ref_executionOrder)
  {
    W_TEST_RESULT(ref_pipeline.CullDeadPasses());
    W_TEST_RESULT(ref_pipeline.SortPasses());

    ref_executionOrder.Clear();
    s_pExecutionOrder = &ref_executionOrder;
    WViewData viewData;
    WCamera camera;
    W_TEST_BOOL(ref_pipeline.AddRenderPasses(viewData, camera, ref_graph).Succeeded());
    s_pExecutionOrder = nullptr;
  }

  void TestExecutionOrder(const WDynamicArray<RecordedPass>& executionOrder, WArrayPtr<const char* const> expectedOrder)
  {
    W_TEST_INT(executionOrder.GetCount(), expectedOrder.GetCount());
    if (executionOrder.GetCount() != expectedOrder.GetCount())
      return;

    for (WUInt32 i = 0; i < expectedOrder.GetCount(); ++i)
    {
      W_TEST_STRING(executionOrder[i].m_sName, expectedOrder[i]);
    }
  }

  WUInt32 GetPassIndex(const WDynamicArray<RecordedPass>& executionOrder, const char* szName)
  {
    for (WUInt32 i = 0; i < executionOrder.GetCount(); ++i)
    {
      if (executionOrder[i].m_sName == szName)
        return i;
    }

    W_TEST_FAILURE("Pass not found", "No pass named '{}' was executed.", szName);
    return WInvalidIndex;
  }

  void TestExecutedBefore(const WDynamicArray<RecordedPass>& executionOrder, const char* szFirst, const char* szSecond)
  {
    W_TEST_BOOL(GetPassIndex(executionOrder, szFirst) < GetPassIndex(executionOrder, szSecond));
  }

  RecordedPin GetInput(const WDynamicArray<RecordedPass>& executionOrder, const char* szName, WUInt32 uiPin)
  {
    const WUInt32 uiPass = GetPassIndex(executionOrder, szName);
    if (uiPass == WInvalidIndex || !W_TEST_BOOL(uiPin < executionOrder[uiPass].m_Inputs.GetCount()))
      return {};

    return executionOrder[uiPass].m_Inputs[uiPin];
  }

  RecordedPin GetOutput(const WDynamicArray<RecordedPass>& executionOrder, const char* szName, WUInt32 uiPin)
  {
    const WUInt32 uiPass = GetPassIndex(executionOrder, szName);
    if (uiPass == WInvalidIndex || !W_TEST_BOOL(uiPin < executionOrder[uiPass].m_Outputs.GetCount()))
      return {};

    return executionOrder[uiPass].m_Outputs[uiPin];
  }

  void TestPinsEqual(const RecordedPin& lhs, const RecordedPin& rhs, Connectivity expectedConnectivity)
  {
    W_TEST_BOOL(lhs.m_Connectivity == expectedConnectivity);
    W_TEST_BOOL(rhs.m_Connectivity == expectedConnectivity);
    W_TEST_INT(lhs.m_uiHandleId, rhs.m_uiHandleId);
  }

  struct InlineTestData
  {
    enum class Mode
    {
      Basic,
      Forward,
      Mixed,
      BufferForward,
      MixedBoundaries,
      Extractors,
      InvalidConnection,
      Failure,
    };

    Mode m_Mode = Mode::Basic;

    WStatus Import(WStringView, WDynamicArray<WUniquePtr<WRenderPipelinePass>>& out_passes, WDynamicArray<WUniquePtr<WExtractor>>& out_extractors, WDynamicArray<WRenderPipelineResourceLoaderConnection>& out_connections)
    {
      W_IGNORE_UNUSED(out_extractors);

      if (m_Mode == Mode::Failure)
        return WStatus("Test failure");

      if (m_Mode == Mode::Extractors)
      {
        out_extractors.PushBack(W_DEFAULT_NEW(WVisibleObjectsExtractor));
        out_extractors.PushBack(W_DEFAULT_NEW(WSelectedObjectsExtractor));
        return W_SUCCESS;
      }

      auto AddImportedPass = [&out_passes](WRenderPipelinePass* pPass, const char* szName)
      {
        pPass->SetName(szName);
        out_passes.PushBack(WUniquePtr<WRenderPipelinePass>(pPass, WFoundation::GetDefaultAllocator()));
      };

      if (m_Mode == Mode::Forward)
      {
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureInputNode), "Input");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureOutputNode), "Output");
        Connect(out_connections, 0, "Value", 1, "Value");
        return W_SUCCESS;
      }

      if (m_Mode == Mode::BufferForward)
      {
        AddImportedPass(W_DEFAULT_NEW(WSubGraphBufferInputNode), "Input");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphBufferOutputNode), "Output");
        Connect(out_connections, 0, "Value", 1, "Value");
        return W_SUCCESS;
      }

      if (m_Mode == Mode::MixedBoundaries)
      {
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureInputNode), "TextureInput");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphBufferInputNode), "BufferInput");
        AddImportedPass(W_DEFAULT_NEW(WGpuPipelineTestPass), "Internal");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureOutputNode), "TextureOutput");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphBufferOutputNode), "BufferOutput");
        Connect(out_connections, 0, "Value", 2, "Input");
        Connect(out_connections, 1, "Value", 2, "BufferInput");
        Connect(out_connections, 2, "Output", 3, "Value");
        Connect(out_connections, 2, "BufferOutput", 4, "Value");
        return W_SUCCESS;
      }

      if (m_Mode == Mode::Mixed)
      {
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureInputNode), "Input");
        AddImportedPass(W_DEFAULT_NEW(WGpuPipelineTestPass), "Internal");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureOutputNode), "Forwarded");
        AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureOutputNode), "Processed");
        Connect(out_connections, 0, "Value", 1, "Input");
        Connect(out_connections, 0, "Value", 2, "Value");
        Connect(out_connections, 1, "Output", 3, "Value");
        return W_SUCCESS;
      }

      AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureInputNode), "Input");
      AddImportedPass(W_DEFAULT_NEW(WGpuPipelineTestPass), "First");
      AddImportedPass(W_DEFAULT_NEW(WGpuPipelineTestPass), "Second");
      AddImportedPass(W_DEFAULT_NEW(WSubGraphTextureOutputNode), "Output");

      if (m_Mode == Mode::InvalidConnection)
      {
        Connect(out_connections, 0, "Value", 10, "Input");
        return W_SUCCESS;
      }

      Connect(out_connections, 0, "Value", 1, "Input");
      Connect(out_connections, 0, "Value", 2, "Input");
      Connect(out_connections, 1, "Output", 3, "Value");
      return W_SUCCESS;
    }
  };

  WUniquePtr<WSubGraphNode> CreateSubGraph(const char* szPipeline)
  {
    WUniquePtr<WSubGraphNode> pSubGraph = W_DEFAULT_NEW(WSubGraphNode);
    pSubGraph->m_sPipeline = szPipeline;
    return pSubGraph;
  }
} // namespace

static WGpuPipelineTest s_GpuPipelineTest;

void WGpuPipelineTest::SetupSubTests()
{
  AddSubTest("DeadPassCulling", SubTests::ST_DeadPassCulling);
  AddSubTest("DependencySorting", SubTests::ST_DependencySorting);
  AddSubTest("PassThroughSorting", SubTests::ST_PassThroughSorting);
  AddSubTest("CycleDetection", SubTests::ST_CycleDetection);
  AddSubTest("TextureSwitch", SubTests::ST_TextureSwitch);
  AddSubTest("SubGraphInlining", SubTests::ST_SubGraphInlining);
  AddSubTest("SubGraphInliningErrors", SubTests::ST_SubGraphInliningErrors);
  AddSubTest("ViewBlackboard", SubTests::ST_ViewBlackboard);
  AddSubTest("BufferPassThroughSorting", SubTests::ST_BufferPassThroughSorting);
  AddSubTest("MixedPassThrough", SubTests::ST_MixedPassThrough);
  AddSubTest("MixedPinIndexing", SubTests::ST_MixedPinIndexing);
  AddSubTest("BufferSwitch", SubTests::ST_BufferSwitch);
  AddSubTest("SubGraphBufferInlining", SubTests::ST_SubGraphBufferInlining);
  AddSubTest("IncompatiblePinConnection", SubTests::ST_IncompatiblePinConnection);
  AddSubTest("SharedSourceSwitch", SubTests::ST_SharedSourceSwitch);
}

WResult WGpuPipelineTest::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));
  m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("GpuPipelineTest");
  return W_SUCCESS;
}

WResult WGpuPipelineTest::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_pRenderGraph = nullptr;
  return WGraphicsTest::DeInitializeSubTest(iIdentifier);
}

WTestAppRun WGpuPipelineTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_DeadPassCulling:
      DeadPassCulling();
      break;
    case SubTests::ST_DependencySorting:
      DependencySorting();
      break;
    case SubTests::ST_PassThroughSorting:
      PassThroughSorting();
      break;
    case SubTests::ST_CycleDetection:
      CycleDetection();
      break;
    case SubTests::ST_TextureSwitch:
      TextureSwitch();
      break;
    case SubTests::ST_SubGraphInlining:
      SubGraphInlining();
      break;
    case SubTests::ST_SubGraphInliningErrors:
      SubGraphInliningErrors();
      break;
    case SubTests::ST_ViewBlackboard:
      ViewBlackboard();
      break;
    case SubTests::ST_BufferPassThroughSorting:
      BufferPassThroughSorting();
      break;
    case SubTests::ST_MixedPassThrough:
      MixedPassThrough();
      break;
    case SubTests::ST_MixedPinIndexing:
      MixedPinIndexing();
      break;
    case SubTests::ST_BufferSwitch:
      BufferSwitch();
      break;
    case SubTests::ST_SubGraphBufferInlining:
      SubGraphBufferInlining();
      break;
    case SubTests::ST_IncompatiblePinConnection:
      IncompatiblePinConnection();
      break;
    case SubTests::ST_SharedSourceSwitch:
      SharedSourceSwitch();
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return WTestAppRun::Quit;
}

void WGpuPipelineTest::DeadPassCulling()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiDeadSource = AddPass<WGpuPipelineTestSourcePass>(passes, "DeadSource");
  const WUInt32 uiDeadPass = AddPass<WGpuPipelineTestPass>(passes, "DeadPass");
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiLivePass = AddPass<WGpuPipelineTestPass>(passes, "LivePass");
  const WUInt32 uiLiveSource = AddPass<WGpuPipelineTestSourcePass>(passes, "LiveSource");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiDeadSource, "Output", uiDeadPass, "Input");
  Connect(connections, uiLiveSource, "Output", uiLivePass, "Input");
  Connect(connections, uiLivePass, "Output", uiSink, "InputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"LiveSource", "LivePass", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void WGpuPipelineTest::DependencySorting()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiBranchB = AddPass<WGpuPipelineTestPass>(passes, "BranchB");
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");
  const WUInt32 uiBranchA = AddPass<WGpuPipelineTestPass>(passes, "BranchA");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiBranchA, "Input");
  Connect(connections, uiSource, "Output", uiBranchB, "Input");
  Connect(connections, uiBranchA, "Output", uiSink, "InputA");
  Connect(connections, uiBranchB, "Output", uiSink, "InputB");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "BranchA", "BranchB", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void WGpuPipelineTest::PassThroughSorting()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiPassThrough = AddPass<WGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const WUInt32 uiReader = AddPass<WGpuPipelineTestSinkPass>(passes, "Reader");
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPassThrough, "Pin");
  Connect(connections, uiSource, "Output", uiReader, "InputA");
  Connect(connections, uiPassThrough, "Pin", uiSink, "InputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Reader", "PassThrough", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void WGpuPipelineTest::CycleDetection()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiPassA = AddPass<WGpuPipelineTestPass>(passes, "PassA");
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiPassB = AddPass<WGpuPipelineTestPass>(passes, "PassB");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiPassA, "Output", uiPassB, "Input");
  Connect(connections, uiPassB, "Output", uiPassA, "Input");
  Connect(connections, uiPassB, "Output", uiSink, "InputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  W_TEST_RESULT(pPipeline->CullDeadPasses());

  WTestLogInterface log;
  WTestLogSystemScope logSystemScope(&log, true);
  log.ExpectMessage("GPU pipeline contains a cycle", WLogMsgType::ErrorMsg);
  log.ExpectMessage("Failed to sort pass", WLogMsgType::ErrorMsg, 3);
  W_TEST_BOOL(pPipeline->SortPasses().Failed());
}

void WGpuPipelineTest::TextureSwitch()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSourceA = AddPass<WGpuPipelineTestSourcePass>(passes, "SourceA");
  const WUInt32 uiSourceB = AddPass<WGpuPipelineTestSourcePass>(passes, "SourceB");

  WUniquePtr<WTextureSwitchPass> pSwitch = W_DEFAULT_NEW(WTextureSwitchPass);
  pSwitch->SetName("Switch");
  pSwitch->m_sBlackboardProperty = "Quality";
  pSwitch->m_Values.PushBack(10);
  pSwitch->m_Values.PushBack(20);
  const WUInt32 uiSwitch = passes.GetCount();
  passes.PushBack(std::move(pSwitch));

  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSourceA, "Output", uiSwitch, "10");
  Connect(connections, uiSourceB, "Output", uiSwitch, "20");
  Connect(connections, uiSwitch, "Output", uiSink, "InputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  W_TEST_INT(pPipeline->GetSwitches().GetCount(), 1);
  W_TEST_STRING(pPipeline->GetSwitches()[0].m_sBlackboardProperty.GetString(), "Quality");

  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedDefaultOrder[] = {"SourceA", "Sink"};
  TestExecutionOrder(executionOrder, expectedDefaultOrder);

  W_TEST_BOOL(pPipeline->SetSwitchValue(0, 20));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedSwitchedOrder[] = {"SourceB", "Sink"};
  TestExecutionOrder(executionOrder, expectedSwitchedOrder);
  W_TEST_BOOL(!pPipeline->SetSwitchValue(0, 20));

  W_TEST_BOOL(pPipeline->SetSwitchValue(0, 42));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  TestExecutionOrder(executionOrder, expectedDefaultOrder);
  W_TEST_BOOL(!pPipeline->SetSwitchToDefault(0));
}

void WGpuPipelineTest::SharedSourceSwitch()
{
  // One source feeding two switches. This graph has no cycle, so sorting must succeed.
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");

  WUniquePtr<WTextureSwitchPass> pSwitchX = W_DEFAULT_NEW(WTextureSwitchPass);
  pSwitchX->SetName("SwitchX");
  pSwitchX->m_sBlackboardProperty = "QualityX";
  pSwitchX->m_Values.PushBack(10);
  const WUInt32 uiSwitchX = passes.GetCount();
  passes.PushBack(std::move(pSwitchX));

  WUniquePtr<WTextureSwitchPass> pSwitchY = W_DEFAULT_NEW(WTextureSwitchPass);
  pSwitchY->SetName("SwitchY");
  pSwitchY->m_sBlackboardProperty = "QualityY";
  pSwitchY->m_Values.PushBack(10);
  const WUInt32 uiSwitchY = passes.GetCount();
  passes.PushBack(std::move(pSwitchY));

  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiSwitchX, "10");
  Connect(connections, uiSource, "Output", uiSwitchY, "10");
  Connect(connections, uiSwitchX, "Output", uiSink, "InputA");
  Connect(connections, uiSwitchY, "Output", uiSink, "InputB");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);

  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedOrder[] = {"Source", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void WGpuPipelineTest::SubGraphInlining()
{
  {
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> rootPasses;
    const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(rootPasses, "Source");
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("Basic");
    const WUInt32 uiSubGraph = 1;
    const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(rootPasses, "Sink");

    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(rootPasses[uiSource].Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(rootPasses[uiSink].Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, uiSource, "Output", uiSubGraph, "Input");
    Connect(connections, uiSubGraph, "Output", 2, "InputA");

    InlineTestData data;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 4);
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[0])->GetName(), "Source");
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[1])->GetName(), "Sink");
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[2])->GetName(), "First");
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[3])->GetName(), "Second");
    W_TEST_INT(ownedPasses.GetCount(), 4);
    W_TEST_INT(connections.GetCount(), 3);
    W_TEST_INT(connections[0].m_uiSource, 0);
    W_TEST_INT(connections[0].m_uiTarget, 2);
    W_TEST_STRING(connections[0].m_sTargetPin, "Input");
    W_TEST_INT(connections[1].m_uiSource, 2);
    W_TEST_INT(connections[1].m_uiTarget, 1);
    W_TEST_STRING(connections[1].m_sSourcePin, "Output");
  }

  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("Forward");
    WUniquePtr<WGpuPipelineTestSinkPass> pSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Output", 2, "InputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Forward;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 2);
    W_TEST_INT(connections.GetCount(), 1);
    W_TEST_INT(connections[0].m_uiSource, 0);
    W_TEST_INT(connections[0].m_uiTarget, 1);
    W_TEST_STRING(connections[0].m_sSourcePin, "Output");
    W_TEST_STRING(connections[0].m_sTargetPin, "InputA");
  }

  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("Mixed");
    WUniquePtr<WGpuPipelineTestSinkPass> pForwardSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WUniquePtr<WGpuPipelineTestSinkPass> pProcessedSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pForwardSink.Borrow());
    nodes.PushBack(pProcessedSink.Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Forwarded", 2, "InputA");
    Connect(connections, 1, "Processed", 3, "InputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Mixed;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 4);
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[3])->GetName(), "Internal");
    W_TEST_INT(connections.GetCount(), 3);
    W_TEST_INT(connections[0].m_uiSource, 0);
    W_TEST_INT(connections[0].m_uiTarget, 3);
    W_TEST_INT(connections[1].m_uiSource, 0);
    W_TEST_INT(connections[1].m_uiTarget, 1);
    W_TEST_INT(connections[2].m_uiSource, 3);
    W_TEST_INT(connections[2].m_uiTarget, 2);
  }

  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pFirstSubGraph = CreateSubGraph("First");
    WUniquePtr<WSubGraphNode> pSecondSubGraph = CreateSubGraph("Second");
    WUniquePtr<WGpuPipelineTestSinkPass> pSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pFirstSubGraph.Borrow());
    nodes.PushBack(pSecondSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Output", 2, "Input");
    Connect(connections, 2, "Output", 3, "InputA");

    InlineTestData data;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 6);
    W_TEST_INT(ownedPasses.GetCount(), 8);
    W_TEST_INT(connections.GetCount(), 5);
    for (WRenderPipelineNode* pNode : nodes)
    {
      W_TEST_BOOL(WDynamicCast<WSubGraphNode*>(pNode) == nullptr);
    }
  }

  {
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("Extractors");
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSubGraph.Borrow());

    WUniquePtr<WVisibleObjectsExtractor> pRootExtractor = W_DEFAULT_NEW(WVisibleObjectsExtractor);
    WDynamicArray<WExtractor*> extractors;
    extractors.PushBack(pRootExtractor.Borrow());

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Extractors;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 0);
    W_TEST_INT(extractors.GetCount(), 2);
    W_TEST_BOOL(extractors[0] == pRootExtractor.Borrow());
    W_TEST_BOOL(extractors[1]->GetDynamicRTTI() == WGetStaticRTTI<WSelectedObjectsExtractor>());
    W_TEST_INT(ownedExtractors.GetCount(), 1);
    W_TEST_BOOL(ownedExtractors[0].Borrow() == extractors[1]);
  }
}

void WGpuPipelineTest::SubGraphInliningErrors()
{
  auto Run = [](InlineTestData::Mode mode, const char* szInputPin)
  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("Error");
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, szInputPin);

    InlineTestData data;
    data.m_Mode = mode;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    return WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data));
  };

  W_TEST_BOOL(Run(InlineTestData::Mode::Failure, "Input").Failed());
  W_TEST_BOOL(Run(InlineTestData::Mode::InvalidConnection, "Input").Failed());
  W_TEST_BOOL(Run(InlineTestData::Mode::Basic, "Missing").Failed());
}

void WGpuPipelineTest::ViewBlackboard()
{
  // SourceA -> Switch("10") , SourceB -> Switch("20") , Switch -> Sink.InputA
  // "Sink.Value" drives a regular property mapping, "Quality" drives the switch mapping.
  WRenderPipelineResourceDescriptor desc;
  {
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
    AddPass<WGpuPipelineTestSourcePass>(passes, "SourceA");
    AddPass<WGpuPipelineTestSourcePass>(passes, "SourceB");

    {
      WUniquePtr<WTextureSwitchPass> pSwitch = W_DEFAULT_NEW(WTextureSwitchPass);
      pSwitch->SetName("Switch");
      pSwitch->m_sBlackboardProperty = "Quality";
      pSwitch->m_Values.PushBack(10);
      pSwitch->m_Values.PushBack(20);
      passes.PushBack(std::move(pSwitch));
    }

    AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

    WDynamicArray<const WRenderPipelinePass*> passPointers;
    for (const WUniquePtr<WRenderPipelinePass>& pPass : passes)
    {
      passPointers.PushBack(pPass.Borrow());
    }

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 2, "10");
    Connect(connections, 1, "Output", 2, "20");
    Connect(connections, 2, "Output", 3, "InputA");

    WMemoryStreamContainerWrapperStorage<WDynamicArray<WUInt8>> storage(&desc.m_SerializedPipeline);
    WMemoryStreamWriter writer(&storage);
    W_TEST_RESULT(WRenderPipelineResourceLoader::ExportPipeline(passPointers, {}, connections, writer));
  }

  WRenderPipelineResourceHandle hPipeline = WResourceManager::CreateResource<WRenderPipelineResource>("ViewBlackboardTestPipeline", std::move(desc), "ViewBlackboardTestPipeline");

  WWorldDesc worldDesc("ViewBlackboardTestWorld");
  WWorld world(worldDesc);

  WView* pView = nullptr;
  const WViewHandle hView = WRenderWorld::CreateView("ViewBlackboardTest", pView);
  W_TEST_BOOL(pView != nullptr);
  if (pView == nullptr)
    return;

  W_SCOPE_EXIT(WRenderWorld::DeleteView(hView));

  const WSharedPtr<WBlackboard>& pWorldBlackboard = world.GetBlackboard();
  WSharedPtr<WBlackboard> pViewBlackboard = WBlackboard::Create("ViewBlackboardTestView");

  pWorldBlackboard->SetEntryValue("Sink.Value", 1);
  pWorldBlackboard->SetEntryValue("Quality", 20);

  pView->SetWorld(&world);
  pView->SetBlackboard(pViewBlackboard);
  pView->SetRenderPipelineResource(hPipeline);

  if (!W_TEST_BOOL(pView->m_pRenderPipeline != nullptr))
    return;

  auto GetPropertyValue = [&]() -> WInt32
  {
    auto* pSink = static_cast<WGpuPipelineTestSinkPass*>(pView->m_pRenderPipeline->GetPassByName("Sink"));
    return pSink != nullptr ? pSink->m_iValue : -1;
  };

  auto GetSwitchIndex = [&]() -> WInt32
  {
    auto* pSwitch = static_cast<WSwitchBasePass*>(pView->m_pRenderPipeline->GetPassByName("Switch"));
    return pSwitch != nullptr ? static_cast<WInt32>(pSwitch->m_uiSelectedValueIndex) : -1;
  };

  auto RemoveEntry = [](const WSharedPtr<WBlackboard>& pBlackboard, const char* szName)
  {
    WHashedString sName;
    sName.Assign(szName);
    pBlackboard->RemoveEntry(sName);
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "World blackboard only")
  {
    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 1);
    W_TEST_INT(GetSwitchIndex(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "View blackboard overrides world blackboard")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 2);
    pViewBlackboard->SetEntryValue("Quality", 10);

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 2);
    W_TEST_INT(GetSwitchIndex(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "View blackboard value change")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 3);
    pViewBlackboard->SetEntryValue("Quality", 20);

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 3);
    W_TEST_INT(GetSwitchIndex(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "World blackboard does not override view blackboard")
  {
    pWorldBlackboard->SetEntryValue("Sink.Value", 100);
    pWorldBlackboard->SetEntryValue("Quality", 10);

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 3);
    W_TEST_INT(GetSwitchIndex(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Removing view entries falls back to world blackboard")
  {
    RemoveEntry(pViewBlackboard, "Sink.Value");
    RemoveEntry(pViewBlackboard, "Quality");

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 100);
    W_TEST_INT(GetSwitchIndex(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Detaching the view blackboard falls back to world blackboard")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 7);
    pViewBlackboard->SetEntryValue("Quality", 20);

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 7);
    W_TEST_INT(GetSwitchIndex(), 1);

    pView->SetBlackboard(WSharedPtr<WBlackboard>());

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 100);
    W_TEST_INT(GetSwitchIndex(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Removing world entries restores the default value")
  {
    RemoveEntry(pWorldBlackboard, "Sink.Value");
    RemoveEntry(pWorldBlackboard, "Quality");

    pView->EnsureUpToDate();
    W_TEST_INT(GetPropertyValue(), 0);
    W_TEST_INT(GetSwitchIndex(), 0);
  }
}

void WGpuPipelineTest::BufferPassThroughSorting()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiPassThrough = AddPass<WGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const WUInt32 uiReader = AddPass<WGpuPipelineTestSinkPass>(passes, "Reader");
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "BufferOutput", uiPassThrough, "BufferPin");
  Connect(connections, uiSource, "BufferOutput", uiReader, "BufferInputA");
  Connect(connections, uiPassThrough, "BufferPin", uiSink, "BufferInputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Reader", "PassThrough", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  const RecordedPin sourceBuffer = GetOutput(executionOrder, "Source", 1);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Reader", 2), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetOutput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void WGpuPipelineTest::MixedPassThrough()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");
  const WUInt32 uiPassThrough = AddPass<WGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const WUInt32 uiTextureReader = AddPass<WGpuPipelineTestSinkPass>(passes, "TextureReader");
  const WUInt32 uiBufferReader = AddPass<WGpuPipelineTestSinkPass>(passes, "BufferReader");
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPassThrough, "Pin");
  Connect(connections, uiSource, "Output", uiTextureReader, "InputA");
  Connect(connections, uiSource, "BufferOutput", uiPassThrough, "BufferPin");
  Connect(connections, uiSource, "BufferOutput", uiBufferReader, "BufferInputA");
  Connect(connections, uiPassThrough, "Pin", uiSink, "InputA");
  Connect(connections, uiPassThrough, "BufferPin", uiSink, "BufferInputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  W_TEST_INT(executionOrder.GetCount(), 5);

  // Both resources may be modified in place, so both readers have to run before the pass-through.
  TestExecutedBefore(executionOrder, "Source", "TextureReader");
  TestExecutedBefore(executionOrder, "Source", "BufferReader");
  TestExecutedBefore(executionOrder, "TextureReader", "PassThrough");
  TestExecutedBefore(executionOrder, "BufferReader", "PassThrough");
  TestExecutedBefore(executionOrder, "PassThrough", "Sink");

  const RecordedPin sourceTexture = GetOutput(executionOrder, "Source", 0);
  TestPinsEqual(sourceTexture, GetInput(executionOrder, "PassThrough", 0), Connectivity::Texture);
  TestPinsEqual(sourceTexture, GetOutput(executionOrder, "PassThrough", 0), Connectivity::Texture);
  TestPinsEqual(sourceTexture, GetInput(executionOrder, "Sink", 0), Connectivity::Texture);

  const RecordedPin sourceBuffer = GetOutput(executionOrder, "Source", 1);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetOutput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void WGpuPipelineTest::MixedPinIndexing()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");
  const WUInt32 uiPass = AddPass<WGpuPipelineTestPass>(passes, "Pass");
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPass, "Input");
  Connect(connections, uiSource, "BufferOutput", uiPass, "BufferInput");
  Connect(connections, uiPass, "Output", uiSink, "InputA");
  Connect(connections, uiPass, "BufferOutput", uiSink, "BufferInputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Pass", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  // Texture and buffer pins are indexed independently, in declaration order.
  TestPinsEqual(GetOutput(executionOrder, "Source", 0), GetInput(executionOrder, "Pass", 0), Connectivity::Texture);
  TestPinsEqual(GetOutput(executionOrder, "Source", 1), GetInput(executionOrder, "Pass", 1), Connectivity::Buffer);
  W_TEST_BOOL(GetOutput(executionOrder, "Pass", 0).m_Connectivity == Connectivity::Texture);
  W_TEST_BOOL(GetOutput(executionOrder, "Pass", 1).m_Connectivity == Connectivity::Buffer);
  W_TEST_BOOL(GetInput(executionOrder, "Sink", 0).m_Connectivity == Connectivity::Texture);
  W_TEST_BOOL(GetInput(executionOrder, "Sink", 1).m_Connectivity == Connectivity::None);
  W_TEST_BOOL(GetInput(executionOrder, "Sink", 2).m_Connectivity == Connectivity::Buffer);
  W_TEST_BOOL(GetInput(executionOrder, "Sink", 3).m_Connectivity == Connectivity::None);
}

void WGpuPipelineTest::BufferSwitch()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSourceA = AddPass<WGpuPipelineTestSourcePass>(passes, "SourceA");
  const WUInt32 uiSourceB = AddPass<WGpuPipelineTestSourcePass>(passes, "SourceB");

  WUniquePtr<WBufferSwitchPass> pSwitch = W_DEFAULT_NEW(WBufferSwitchPass);
  pSwitch->SetName("Switch");
  pSwitch->m_sBlackboardProperty = "Quality";
  pSwitch->m_Values.PushBack(10);
  pSwitch->m_Values.PushBack(20);
  const WUInt32 uiSwitch = passes.GetCount();
  passes.PushBack(std::move(pSwitch));

  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSourceA, "BufferOutput", uiSwitch, "10");
  Connect(connections, uiSourceB, "BufferOutput", uiSwitch, "20");
  Connect(connections, uiSwitch, "Output", uiSink, "BufferInputA");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  W_TEST_INT(pPipeline->GetSwitches().GetCount(), 1);

  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedDefaultOrder[] = {"SourceA", "Sink"};
  TestExecutionOrder(executionOrder, expectedDefaultOrder);
  TestPinsEqual(GetOutput(executionOrder, "SourceA", 1), GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);

  W_TEST_BOOL(pPipeline->SetSwitchValue(0, 20));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedSwitchedOrder[] = {"SourceB", "Sink"};
  TestExecutionOrder(executionOrder, expectedSwitchedOrder);
  TestPinsEqual(GetOutput(executionOrder, "SourceB", 1), GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void WGpuPipelineTest::SubGraphBufferInlining()
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer boundary forwarded directly")
  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("BufferForward");
    WUniquePtr<WGpuPipelineTestSinkPass> pSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "BufferOutput", 1, "Input");
    Connect(connections, 1, "Output", 2, "BufferInputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::BufferForward;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 2);
    W_TEST_INT(connections.GetCount(), 1);
    W_TEST_INT(connections[0].m_uiSource, 0);
    W_TEST_INT(connections[0].m_uiTarget, 1);
    W_TEST_STRING(connections[0].m_sSourcePin, "BufferOutput");
    W_TEST_STRING(connections[0].m_sTargetPin, "BufferInputA");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Texture and buffer boundaries in one sub-graph")
  {
    WUniquePtr<WGpuPipelineTestSourcePass> pSource = W_DEFAULT_NEW(WGpuPipelineTestSourcePass);
    WUniquePtr<WSubGraphNode> pSubGraph = CreateSubGraph("MixedBoundaries");
    WUniquePtr<WGpuPipelineTestSinkPass> pSink = W_DEFAULT_NEW(WGpuPipelineTestSinkPass);
    WDynamicArray<WRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "TextureInput");
    Connect(connections, 0, "BufferOutput", 1, "BufferInput");
    Connect(connections, 1, "TextureOutput", 2, "InputA");
    Connect(connections, 1, "BufferOutput", 2, "BufferInputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::MixedBoundaries;
    WDynamicArray<WUniquePtr<WRenderPipelinePass>> ownedPasses;
    WDynamicArray<WExtractor*> extractors;
    WDynamicArray<WUniquePtr<WExtractor>> ownedExtractors;
    W_TEST_BOOL(WRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, WMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    W_TEST_INT(nodes.GetCount(), 3);
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[1])->GetName(), "Sink");
    W_TEST_STRING(WDynamicCast<WRenderPipelinePass*>(nodes[2])->GetName(), "Internal");
    W_TEST_INT(connections.GetCount(), 4);

    auto HasConnection = [&connections](WUInt32 uiSource, const char* szSourcePin, WUInt32 uiTarget, const char* szTargetPin) -> bool
    {
      for (const WRenderPipelineResourceLoaderConnection& connection : connections)
      {
        if (connection.m_uiSource == uiSource && connection.m_sSourcePin == szSourcePin && connection.m_uiTarget == uiTarget && connection.m_sTargetPin == szTargetPin)
          return true;
      }
      return false;
    };

    W_TEST_BOOL(HasConnection(0, "Output", 2, "Input"));
    W_TEST_BOOL(HasConnection(0, "BufferOutput", 2, "BufferInput"));
    W_TEST_BOOL(HasConnection(2, "Output", 1, "InputA"));
    W_TEST_BOOL(HasConnection(2, "BufferOutput", 1, "BufferInputA"));
  }
}

void WGpuPipelineTest::IncompatiblePinConnection()
{
  WDynamicArray<WUniquePtr<WRenderPipelinePass>> passes;
  const WUInt32 uiSource = AddPass<WGpuPipelineTestSourcePass>(passes, "Source");
  const WUInt32 uiSink = AddPass<WGpuPipelineTestSinkPass>(passes, "Sink");

  WDynamicArray<WRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiSink, "BufferInputA");
  Connect(connections, uiSource, "BufferOutput", uiSink, "InputA");
  Connect(connections, uiSource, "Output", uiSink, "InputB");

  WUniquePtr<WRenderPipelinePassGraph> pPipeline;
  {
    WTestLogInterface log;
    WTestLogSystemScope logSystemScope(&log, true);
    log.ExpectMessage("texture pins can't be connected to buffer pins", WLogMsgType::ErrorMsg, 2);

    pPipeline = CreatePipeline(std::move(passes), connections);
  }

  // The mismatched connections are dropped, the compatible one still works.
  WDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  W_TEST_BOOL(GetInput(executionOrder, "Sink", 0).m_Connectivity == Connectivity::None);
  W_TEST_BOOL(GetInput(executionOrder, "Sink", 2).m_Connectivity == Connectivity::None);
  TestPinsEqual(GetOutput(executionOrder, "Source", 0), GetInput(executionOrder, "Sink", 1), Connectivity::Texture);
}