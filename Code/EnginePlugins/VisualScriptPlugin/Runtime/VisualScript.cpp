#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

WVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(WVisualScriptNodeDescription::Type::Enum nodeType, WVisualScriptDataType::Enum dataType);

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "EntryCall_Coroutine",
    "MessageHandler",
    "MessageHandler_Coroutine",
    "ReflectedFunction",
    "GetReflectedProperty",
    "SetReflectedProperty",
    "InplaceCoroutine",
    "GetScriptOwner",
    "SendMessage",

    "", // FirstBuiltin,

    "Builtin_Constant",
    "Builtin_GetVariable",
    "Builtin_SetVariable",
    "Builtin_IncVariable",
    "Builtin_DecVariable",
    "Builtin_TempVariable",

    "Builtin_Branch",
    "Builtin_Switch",
    "Builtin_WhileLoop",
    "Builtin_ForLoop",
    "Builtin_ForEachLoop",
    "Builtin_ReverseForEachLoop",
    "Builtin_Break",
    "Builtin_Jump",

    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_CompareExec",
    "Builtin_IsValid",
    "Builtin_Select",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",
    "Builtin_Modulo",
    "Builtin_Min",
    "Builtin_Max",
    "Builtin_Clamp",
    "Builtin_Expression",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_ToHashedString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_String_Format",
    "Builtin_String_GetCharacterCount",
    "Builtin_String_IsEmpty",

    "Builtin_MakeArray",
    "Builtin_Array_GetElement",
    "Builtin_Array_SetElement",
    "Builtin_Array_GetCount",
    "Builtin_Array_IsEmpty",
    "Builtin_Array_Clear",
    "Builtin_Array_Contains",
    "Builtin_Array_IndexOf",
    "Builtin_Array_Insert",
    "Builtin_Array_PushBack",
    "Builtin_Array_PushBackRange",
    "Builtin_Array_Remove",
    "Builtin_Array_RemoveAt",

    "Builtin_CreateComponent",
    "Builtin_TryGetComponentOfBaseType",

    "Builtin_StartCoroutine",
    "Builtin_StopCoroutine",
    "Builtin_StopAllCoroutines",
    "Builtin_WaitForAll",
    "Builtin_WaitForAny",
    "Builtin_Yield",

    "", // LastBuiltin,
  };
  static_assert(W_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)WVisualScriptNodeDescription::Type::Count);

  template <typename T>
  WResult WriteNodeArray(WArrayPtr<T> a, WStreamWriter& inout_stream)
  {
    WUInt16 uiCount = static_cast<WUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
WVisualScriptNodeDescription::Type::Enum WVisualScriptNodeDescription::Type::GetConversionType(WVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Bool - WVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Byte - WVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Int - WVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Int64 - WVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Float - WVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (WVisualScriptDataType::Double - WVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (WVisualScriptDataType::IsNumberOrBool(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - WVisualScriptDataType::Bool));

  if (targetDataType == WVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == WVisualScriptDataType::HashedString)
    return Builtin_ToHashedString;

  if (targetDataType == WVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  W_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* WVisualScriptNodeDescription::Type::GetName(Enum type)
{
  W_ASSERT_DEBUG(type >= 0 && static_cast<WUInt32>(type) < W_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void WVisualScriptNodeDescription::AppendUserDataName(WStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptGraphDescription::WVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

WVisualScriptGraphDescription::~WVisualScriptGraphDescription() = default;

static const WTypeVersion s_uiVisualScriptGraphDescriptionVersion = 9;

// static
WResult WVisualScriptGraphDescription::Serialize(WArrayPtr<const WVisualScriptNodeDescription> nodes, const WVisualScriptDataDescription& localDataDesc, WStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  W_SUCCEED_OR_RETURN(localDataDesc.Serialize(inout_stream));

  WDefaultMemoryStreamStorage streamStorage;
  WMemoryStreamWriter stream(&streamStorage);
  WUInt32 additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      W_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      W_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      W_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        WUInt32 uiSize = 0;
        WUInt32 uiAlignment = 0;
        W_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const WUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  W_SUCCEED_OR_RETURN(streamStorage.CopyToStream(inout_stream));

  return W_SUCCESS;
}

WResult WVisualScriptGraphDescription::Deserialize(WStreamReader& inout_stream, const WVisualScriptDataDescription& instanceDataDesc, const WVisualScriptDataDescription& constantDataDesc)
{
  WTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);
  if (uiVersion < s_uiVisualScriptGraphDescriptionVersion)
  {
    WLog::Error("Invalid visual script desc version. Expected >= {} but got {}. Visual Script needs re-export", s_uiVisualScriptGraphDescriptionVersion, uiVersion);
    return W_FAILURE;
  }

  {
    WSharedPtr<WVisualScriptDataDescription> pLocalDataDesc = W_SCRIPT_NEW(WVisualScriptDataDescription);
    W_SUCCEED_OR_RETURN(pLocalDataDesc->Deserialize(inout_stream));
    m_pLocalDataDesc = std::move(pLocalDataDesc);
  }

  {
    WUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  WUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = WMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  WUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  auto GetDataDesc = [&](DataOffset dataOffset) -> const WVisualScriptDataDescription*
  {
    switch (dataOffset.GetSource())
    {
      case DataOffset::Source::Local:
        return m_pLocalDataDesc.Borrow();
      case DataOffset::Source::Instance:
        return &instanceDataDesc;
      case DataOffset::Source::Constant:
        return &constantDataDesc;
        W_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return nullptr;
  };

  auto CalculateDataOffsets = [&](DataOffset* pDataOffsets, WUInt32 uiNumDataOffsets)
  {
    DataOffset* pDataOffsetsEnd = pDataOffsets + uiNumDataOffsets;
    while (pDataOffsets < pDataOffsetsEnd)
    {
      auto& dataOffset = *pDataOffsets;
      dataOffset = GetDataDesc(dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      ++pDataOffsets;
    }
  };

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    W_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    W_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    W_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    CalculateDataOffsets(node.GetInputDataOffsets(), node.m_NumInputDataOffsets);
    CalculateDataOffsets(node.GetOutputDataOffsets(), node.m_NumOutputDataOffsets);

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      W_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  return W_SUCCESS;
}

WScriptMessageDesc WVisualScriptGraphDescription::GetMessageDesc() const
{
  auto pEntryNode = GetNode(0);
  W_ASSERT_DEBUG(pEntryNode != nullptr &&
                    (pEntryNode->m_Type == WVisualScriptNodeDescription::Type::MessageHandler ||
                      pEntryNode->m_Type == WVisualScriptNodeDescription::Type::MessageHandler_Coroutine ||
                      pEntryNode->m_Type == WVisualScriptNodeDescription::Type::SendMessage),
    "Entry node is invalid or not a message handler");

  auto& userData = pEntryNode->GetUserData<NodeUserData_TypeAndProperties>();

  WScriptMessageDesc desc;
  desc.m_pType = userData.m_pType;
  desc.m_Properties = WMakeArrayPtr(userData.m_Properties, userData.m_uiNumProperties);
  return desc;
}

//////////////////////////////////////////////////////////////////////////

WCVarInt cvar_MaxNodeExecutions("VisualScript.MaxNodeExecutions", 100000, WCVarFlags::Default, "The maximum number of nodes executed within a script invocation");

WVisualScriptExecutionContext::WVisualScriptExecutionContext(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc, WAllocator* pAllocator)
  : m_pDesc(pDesc)
  , m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  m_LocalDataStorage.AllocateStorage(pAllocator);
  m_DataStorage[DataOffset::Source::Local] = &m_LocalDataStorage;
}

WVisualScriptExecutionContext::~WVisualScriptExecutionContext()
{
  Deinitialize();
}

void WVisualScriptExecutionContext::Initialize(WVisualScriptInstance& inout_instance, WArrayPtr<WVariant> arguments)
{
  m_pInstance = &inout_instance;

  m_DataStorage[DataOffset::Source::Instance] = inout_instance.GetInstanceDataStorage();
  m_DataStorage[DataOffset::Source::Constant] = inout_instance.GetConstantDataStorage();

  auto pNode = m_pDesc->GetNode(0);
  W_ASSERT_DEV(WVisualScriptNodeDescription::Type::IsEntry(pNode->m_Type), "Invalid entry node");

  for (WUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
}

void WVisualScriptExecutionContext::Deinitialize()
{
  // 0x1 is a marker value to indicate that we are in a yield
  if (m_pCurrentCoroutine > reinterpret_cast<WScriptCoroutine*>(0x1))
  {
    auto pModule = m_pInstance->GetWorld()->GetOrCreateModule<WScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(m_pCurrentCoroutine->GetHandle());
    m_pCurrentCoroutine = nullptr;
  }
}

WVisualScriptExecutionContext::ExecResult WVisualScriptExecutionContext::Execute(WTime deltaTimeSinceLastExecution)
{
  W_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_uiExecutionCounter;
  m_DeltaTimeSinceLastExecution = deltaTimeSinceLastExecution;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  WUInt32 uiCounter = 0;
#endif

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (pNode->m_Function == nullptr)
    {
      WLog::Error("Node '{}' is not supported by runtime and should have been removed by the compiler.", WVisualScriptNodeDescription::Type::GetName(pNode->m_Type));
      return ExecResult::Error();
    }
#endif

    ExecResult result = pNode->m_Function(*this, *pNode);
    if (result.m_NextExecAndState < ExecResult::State::Completed)
    {
      return result;
    }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    ++uiCounter;
    if (uiCounter >= WUInt32(cvar_MaxNodeExecutions))
    {
      WLog::Error("Maximum node executions ({}) reached, execution will be aborted. Does the script contain an infinite loop?", cvar_MaxNodeExecutions);
      return ExecResult::Error();
    }
#endif

    m_uiCurrentNode = pNode->GetExecutionIndex(result.m_NextExecAndState);
    m_pCurrentCoroutine = nullptr;

    pNode = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ExecResult::RunNext(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WVisualScriptSendMessageMode, 1)
  W_ENUM_CONSTANTS(WVisualScriptSendMessageMode::Direct, WVisualScriptSendMessageMode::Recursive, WVisualScriptSendMessageMode::Event)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on


W_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Runtime_VisualScript);
