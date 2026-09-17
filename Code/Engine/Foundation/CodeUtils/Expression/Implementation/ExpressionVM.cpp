#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/CodeUtils/Expression/Implementation/ExpressionVMOperations.h>
#include <Foundation/Logging/Log.h>

WExpressionVM::WExpressionVM()
{
  RegisterDefaultFunctions();
}
WExpressionVM::~WExpressionVM() = default;

void WExpressionVM::RegisterFunction(const WExpressionFunction& func)
{
  W_ASSERT_DEV(func.m_Desc.m_uiNumRequiredInputs <= func.m_Desc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", func.m_Desc.m_uiNumRequiredInputs, func.m_Desc.m_InputTypes.GetCount());

  WUInt32 uiFunctionIndex = m_Functions.GetCount();
  m_FunctionNamesToIndex.Insert(func.m_Desc.GetMangledName(), uiFunctionIndex);

  m_Functions.PushBack(func);
}

void WExpressionVM::UnregisterFunction(const WExpressionFunction& func)
{
  WUInt32 uiFunctionIndex = 0;
  if (m_FunctionNamesToIndex.Remove(func.m_Desc.GetMangledName(), &uiFunctionIndex))
  {
    m_Functions.RemoveAtAndSwap(uiFunctionIndex);
    if (uiFunctionIndex != m_Functions.GetCount())
    {
      m_FunctionNamesToIndex[m_Functions[uiFunctionIndex].m_Desc.GetMangledName()] = uiFunctionIndex;
    }
  }
}

WResult WExpressionVM::Execute(const WExpressionByteCode& byteCode, WArrayPtr<const WProcessingStream> inputs,
  WArrayPtr<WProcessingStream> outputs, WUInt32 uiNumInstances, const WExpression::GlobalData& globalData, WBitflags<Flags> flags)
{
  if (flags.IsSet(Flags::ScalarizeStreams))
  {
    W_SUCCEED_OR_RETURN(ScalarizeStreams(inputs, m_ScalarizedInputs));
    W_SUCCEED_OR_RETURN(ScalarizeStreams(outputs, m_ScalarizedOutputs));

    inputs = m_ScalarizedInputs;
    outputs = m_ScalarizedOutputs;
  }
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  else
  {
    AreStreamsScalarized(inputs).AssertSuccess("Input streams are not scalarized");
    AreStreamsScalarized(outputs).AssertSuccess("Output streams are not scalarized");
  }
#endif

  W_SUCCEED_OR_RETURN(MapStreams(byteCode.GetInputs(), inputs, "Input", uiNumInstances, flags, m_MappedInputs));
  W_SUCCEED_OR_RETURN(MapStreams(byteCode.GetOutputs(), outputs, "Output", uiNumInstances, flags, m_MappedOutputs));

  W_SUCCEED_OR_RETURN(MapFunctions(byteCode.GetFunctions(), globalData));

  const WUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * ((uiNumInstances + 3) / 4);
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  // Execute bytecode
  const WExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCodeStart();
  const WExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  ExecutionContext context;
  context.m_pRegisters = m_Registers.GetData();
  context.m_uiNumInstances = uiNumInstances;
  context.m_uiNumSimd4Instances = (uiNumInstances + 3) / 4;
  context.m_Inputs = m_MappedInputs;
  context.m_Outputs = m_MappedOutputs;
  context.m_Functions = m_MappedFunctions;
  context.m_pGlobalData = &globalData;

  while (pByteCode < pByteCodeEnd)
  {
    WExpressionByteCode::OpCode::Enum opCode = WExpressionByteCode::GetOpCode(pByteCode);

    OpFunc func = s_Simd4Funcs[opCode];
    if (func != nullptr)
    {
      func(pByteCode, context);
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
      WLog::Error("Unknown OpCode '{}'. Execution aborted.", opCode);
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

void WExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction(WDefaultExpressionFunctions::s_RandomFunc);
  RegisterFunction(WDefaultExpressionFunctions::s_PerlinNoiseFunc);
}

WResult WExpressionVM::ScalarizeStreams(WArrayPtr<const WProcessingStream> streams, WDynamicArray<WProcessingStream>& out_ScalarizedStreams)
{
  out_ScalarizedStreams.Clear();

  for (auto& stream : streams)
  {
    const WUInt32 uiNumElements = WExpressionAST::DataType::GetElementCount(WExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements == 1)
    {
      out_ScalarizedStreams.PushBack(stream);
    }
    else
    {
      WStringBuilder sNewName;
      WHashedString sNewNameHashed;
      auto data = WMakeArrayPtr((WUInt8*)(stream.GetData()), static_cast<WUInt32>(stream.GetDataSize()));
      auto elementDataType = static_cast<WProcessingStream::DataType>((WUInt32)stream.GetDataType() & ~3u);

      for (WUInt32 i = 0; i < uiNumElements; ++i)
      {
        sNewName.Set(stream.GetName(), ".", WExpressionAST::VectorComponent::GetName(static_cast<WExpressionAST::VectorComponent::Enum>(i)));
        sNewNameHashed.Assign(sNewName);

        auto newData = data.GetSubArray(i * WProcessingStream::GetDataTypeSize(elementDataType));

        out_ScalarizedStreams.PushBack(WProcessingStream(sNewNameHashed, newData, elementDataType, stream.GetElementStride()));
      }
    }
  }

  return W_SUCCESS;
}

WResult WExpressionVM::AreStreamsScalarized(WArrayPtr<const WProcessingStream> streams)
{
  for (auto& stream : streams)
  {
    const WUInt32 uiNumElements = WExpressionAST::DataType::GetElementCount(WExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements > 1)
    {
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}


WResult WExpressionVM::ValidateStream(const WProcessingStream& stream, const WExpression::StreamDesc& streamDesc, WStringView sStreamType, WUInt32 uiNumInstances)
{
  // verify stream data type
  if (stream.GetDataType() != streamDesc.m_DataType)
  {
    WLog::Error("{} stream '{}' expects data of type '{}' or a compatible type. Given type '{}' is not compatible.", sStreamType, streamDesc.m_sName, WProcessingStream::GetDataTypeName(streamDesc.m_DataType), WProcessingStream::GetDataTypeName(stream.GetDataType()));
    return W_FAILURE;
  }

  // verify stream size
  WUInt32 uiElementSize = stream.GetElementSize();
  WUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  if (stream.GetDataSize() < uiExpectedSize)
  {
    WLog::Error("{} stream '{}' data size must be {} bytes or more. Only {} bytes given", sStreamType, streamDesc.m_sName, uiExpectedSize, stream.GetDataSize());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

template <typename T>
WResult WExpressionVM::MapStreams(WArrayPtr<const WExpression::StreamDesc> streamDescs, WArrayPtr<T> streams, WStringView sStreamType, WUInt32 uiNumInstances, WBitflags<Flags> flags, WDynamicArray<T*>& out_MappedStreams)
{
  out_MappedStreams.Clear();
  out_MappedStreams.Reserve(streamDescs.GetCount());

  if (flags.IsSet(Flags::MapStreamsByName))
  {
    for (auto& streamDesc : streamDescs)
    {
      bool bFound = false;

      for (WUInt32 i = 0; i < streams.GetCount(); ++i)
      {
        auto& stream = streams[i];
        if (stream.GetName() == streamDesc.m_sName)
        {
          W_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));

          out_MappedStreams.PushBack(&stream);
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        WLog::Error("Bytecode expects an {} stream '{}'", sStreamType, streamDesc.m_sName);
        return W_FAILURE;
      }
    }
  }
  else
  {
    if (streams.GetCount() != streamDescs.GetCount())
      return W_FAILURE;

    for (WUInt32 i = 0; i < streams.GetCount(); ++i)
    {
      auto& stream = streams.GetPtr()[i];

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      auto& streamDesc = streamDescs.GetPtr()[i];
      W_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));
#endif

      out_MappedStreams.PushBack(&stream);
    }
  }

  return W_SUCCESS;
}

WResult WExpressionVM::MapFunctions(WArrayPtr<const WExpression::FunctionDesc> functionDescs, const WExpression::GlobalData& globalData)
{
  m_MappedFunctions.Clear();
  m_MappedFunctions.Reserve(functionDescs.GetCount());

  for (auto& functionDesc : functionDescs)
  {
    WUInt32 uiFunctionIndex = 0;
    if (!m_FunctionNamesToIndex.TryGetValue(functionDesc.m_sName, uiFunctionIndex))
    {
      WLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionDesc.m_sName);
      return W_FAILURE;
    }

    auto& registeredFunction = m_Functions[uiFunctionIndex];

    // verify signature
    if (functionDesc.m_InputTypes != registeredFunction.m_Desc.m_InputTypes || functionDesc.m_OutputType != registeredFunction.m_Desc.m_OutputType)
    {
      WLog::Error("Signature for registered function '{}' does not match the expected signature from bytecode", functionDesc.m_sName);
      return W_FAILURE;
    }

    if (registeredFunction.m_ValidateGlobalDataFunc != nullptr)
    {
      if (registeredFunction.m_ValidateGlobalDataFunc(globalData).Failed())
      {
        WLog::Error("Global data validation for function '{0}' failed.", functionDesc.m_sName);
        return W_FAILURE;
      }
    }

    m_MappedFunctions.PushBack(&registeredFunction);
  }

  return W_SUCCESS;
}
