#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void MakeSubfunctionName(const WDocumentObject* pObject, const WDocumentObject* pEntryObject, WStringBuilder& out_sName)
  {
    WVariant sNameProperty = pObject->GetTypeAccessor().GetValue("Name");
    WUInt32 uiHash = WHashHelper<WUuid>::Hash(pObject->GetGuid());

    out_sName.SetFormat("{}_{}_{}", pEntryObject != nullptr ? WVisualScriptNodeManager::GetNiceFunctionName(pEntryObject) : "", sNameProperty, WArgU(uiHash, 8, true, 16));
  }

  WVisualScriptDataType::Enum FinalizeDataType(WVisualScriptDataType::Enum dataType)
  {
    WVisualScriptDataType::Enum result = dataType;
    if (result == WVisualScriptDataType::EnumValue || result == WVisualScriptDataType::BitflagValue)
      result = WVisualScriptDataType::Int64;

    if (result == WVisualScriptDataType::Resource)
      result = WVisualScriptDataType::String;

    return result;
  }

  using FillUserDataFunction = WResult (*)(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject);

  static WResult FillUserData_CoroutineMode(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("CoroutineMode");
    return W_SUCCESS;
  }

  static WResult FillUserData_ReflectedPropertyOrFunction(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
      inout_astNode.m_sTargetTypeName.Assign(pNodeDesc->m_pTargetType->GetTypeName());

    WVariantArray propertyNames;
    for (auto& pProp : pNodeDesc->m_TargetProperties)
    {
      WHashedString sPropertyName;
      sPropertyName.Assign(pProp->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return W_SUCCESS;
  }

  static WResult FillUserData_DynamicReflectedProperty(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    auto pTargetType = WVisualScriptTypeDeduction::GetReflectedType(pObject);
    auto pTargetProperty = WVisualScriptTypeDeduction::GetReflectedProperty(pObject);
    if (pTargetType == nullptr || pTargetProperty == nullptr)
      return W_FAILURE;

    inout_astNode.m_sTargetTypeName.Assign(pTargetType->GetTypeName());

    WVariantArray propertyNames;
    {
      WHashedString sPropertyName;
      sPropertyName.Assign(pTargetProperty->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return W_SUCCESS;
  }

  static WResult FillUserData_VariableName(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Name");

    WStringView sName = inout_astNode.m_Value.Get<WString>().GetView();

    WVisualScriptVariable v;
    if (static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager())->GetVariable(WTempHashedString(sName), v).Failed())
    {
      WLog::Error("Invalid variable named '{}'", sName);
      return W_FAILURE;
    }

    return W_SUCCESS;
  }

  static WResult FillUserData_Switch(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    inout_astNode.m_DeductedDataType = WVisualScriptDataType::Int64;

    WVariantArray casesVarArray;

    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
    {
      WTempHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
      WReflectionUtils::GetEnumKeysAndValues(pNodeDesc->m_pTargetType, enumKeysAndValues, WReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& keyAndValue : enumKeysAndValues)
      {
        casesVarArray.PushBack(keyAndValue.m_iValue);
      }
    }
    else
    {
      WVariant casesVar = pObject->GetTypeAccessor().GetValue("Cases");
      casesVarArray = casesVar.Get<WVariantArray>();
      for (auto& caseVar : casesVarArray)
      {
        if (caseVar.IsA<WString>())
        {
          inout_astNode.m_DeductedDataType = WVisualScriptDataType::HashedString;
          caseVar = WTempHashedString(caseVar.Get<WString>()).GetHash();
        }
        else if (caseVar.IsA<WHashedString>())
        {
          inout_astNode.m_DeductedDataType = WVisualScriptDataType::HashedString;
          caseVar = caseVar.Get<WHashedString>().GetHash();
        }
      }
    }

    inout_astNode.m_Value = casesVarArray;
    return W_SUCCESS;
  }

  static WResult FillUserData_Builtin_Compare(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Operator");
    return W_SUCCESS;
  }

  static WResult FillUserData_Builtin_Expression(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    auto pManager = static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

    WTempHybridArray<const WVisualScriptPin*, 16> pins;

    WTempHybridArray<WExpression::StreamDesc, 8> inputs;
    pManager->GetInputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& input = inputs.ExpandAndGetRef();
      input.m_sName.Assign(pPin->GetName());
      input.m_DataType = WVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    WTempHybridArray<WExpression::StreamDesc, 8> outputs;
    pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& output = outputs.ExpandAndGetRef();
      output.m_sName.Assign(pPin->GetName());
      output.m_DataType = WVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    WString sExpressionSource = pObject->GetTypeAccessor().GetValue("Expression").Get<WString>();

    WExpressionParser parser;
    WExpressionParser::Options options = {};
    WExpressionAST ast;
    W_SUCCEED_OR_RETURN(parser.Parse(sExpressionSource, inputs, outputs, options, ast));

    WExpressionCompiler compiler;
    WExpressionByteCode byteCode;
    W_SUCCEED_OR_RETURN(compiler.Compile(ast, byteCode));

    inout_astNode.m_Value = byteCode;

    return W_SUCCESS;
  }

  static WResult FillUserData_ComponentTypeName(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    auto typeName = pObject->GetTypeAccessor().GetValue("TypeName");
    const WRTTI* pType = WRTTI::FindTypeByName(typeName.Get<WString>());
    if (pType == nullptr)
    {
      WLog::Error("Invalid type '{}' for GameObject::CreateComponent/TryGetComponentOfBaseType node.", typeName);
      return W_FAILURE;
    }

    inout_astNode.m_sTargetTypeName.Assign(pType->GetTypeName());
    return W_SUCCESS;
  }

  static WResult FillUserData_Builtin_StartCoroutine(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    W_SUCCEED_OR_RETURN(FillUserData_CoroutineMode(inout_astNode, pCompiler, pObject, pEntryObject));

    auto pManager = static_cast<const WVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    WTempHybridArray<const WVisualScriptPin*, 16> pins;
    pManager->GetOutputExecutionPins(pObject, pins);

    const WUInt32 uiCoroutineBodyIndex = 1;
    auto connections = pManager->GetConnections(*pins[uiCoroutineBodyIndex]);
    if (connections.IsEmpty() == false)
    {
      WStringBuilder sFunctionName;
      MakeSubfunctionName(pObject, pEntryObject, sFunctionName);

      WStringBuilder sFullName;
      sFullName.Set(pCompiler->GetCompiledModule().m_sScriptClassName, "::", sFunctionName, "<Coroutine>");

      inout_astNode.m_sTargetTypeName.Assign(sFullName);

      return pCompiler->AddFunction(sFunctionName, connections[0]->GetTargetPin().GetParent(), pObject);
    }

    return W_SUCCESS;
  }

  static FillUserDataFunction s_TypeToFillUserDataFunctions[] = {
    nullptr,                                   // Invalid,
    &FillUserData_CoroutineMode,               // EntryCall,
    &FillUserData_CoroutineMode,               // EntryCall_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // ReflectedFunction,
    &FillUserData_DynamicReflectedProperty,    // GetReflectedProperty,
    &FillUserData_DynamicReflectedProperty,    // SetReflectedProperty,
    &FillUserData_ReflectedPropertyOrFunction, // InplaceCoroutine,
    nullptr,                                   // GetOwner,
    &FillUserData_ReflectedPropertyOrFunction, // SendMessage,

    nullptr,                                   // FirstBuiltin,

    nullptr,                                   // Builtin_Constant,
    &FillUserData_VariableName,                // Builtin_GetVariable,
    &FillUserData_VariableName,                // Builtin_SetVariable,
    &FillUserData_VariableName,                // Builtin_IncVariable,
    &FillUserData_VariableName,                // Builtin_DecVariable,
    nullptr,                                   // Builtin_TempVariable,

    nullptr,                                   // Builtin_Branch,
    &FillUserData_Switch,                      // Builtin_Switch,
    nullptr,                                   // Builtin_WhileLoop,
    nullptr,                                   // Builtin_ForLoop,
    nullptr,                                   // Builtin_ForEachLoop,
    nullptr,                                   // Builtin_ReverseForEachLoop,
    nullptr,                                   // Builtin_Break,
    nullptr,                                   // Builtin_Jump,

    nullptr,                                   // Builtin_And,
    nullptr,                                   // Builtin_Or,
    nullptr,                                   // Builtin_Not,
    &FillUserData_Builtin_Compare,             // Builtin_Compare,
    &FillUserData_Builtin_Compare,             // Builtin_CompareExec,
    nullptr,                                   // Builtin_IsValid,
    nullptr,                                   // Builtin_Select,

    nullptr,                                   // Builtin_Add,
    nullptr,                                   // Builtin_Subtract,
    nullptr,                                   // Builtin_Multiply,
    nullptr,                                   // Builtin_Divide,
    nullptr,                                   // Builtin_Modulo,
    nullptr,                                   // Builtin_Min,
    nullptr,                                   // Builtin_Max,
    nullptr,                                   // Builtin_Clamp,
    &FillUserData_Builtin_Expression,          // Builtin_Expression,

    nullptr,                                   // Builtin_ToBool,
    nullptr,                                   // Builtin_ToByte,
    nullptr,                                   // Builtin_ToInt,
    nullptr,                                   // Builtin_ToInt64,
    nullptr,                                   // Builtin_ToFloat,
    nullptr,                                   // Builtin_ToDouble,
    nullptr,                                   // Builtin_ToString,
    nullptr,                                   // Builtin_ToHashedString,
    nullptr,                                   // Builtin_ToVariant,
    nullptr,                                   // Builtin_Variant_ConvertTo,

    nullptr,                                   // Builtin_String_Format,
    nullptr,                                   // Builtin_String_GetCharacterCount,
    nullptr,                                   // Builtin_String_IsEmpty,

    nullptr,                                   // Builtin_MakeArray
    nullptr,                                   // Builtin_Array_GetElement,
    nullptr,                                   // Builtin_Array_SetElement,
    nullptr,                                   // Builtin_Array_GetCount,
    nullptr,                                   // Builtin_Array_IsEmpty,
    nullptr,                                   // Builtin_Array_Clear,
    nullptr,                                   // Builtin_Array_Contains,
    nullptr,                                   // Builtin_Array_IndexOf,
    nullptr,                                   // Builtin_Array_Insert,
    nullptr,                                   // Builtin_Array_PushBack,
    nullptr,                                   // Builtin_Array_PushBackRange,
    nullptr,                                   // Builtin_Array_Remove,
    nullptr,                                   // Builtin_Array_RemoveAt,

    &FillUserData_ComponentTypeName,           // Builtin_CreateComponent,
    &FillUserData_ComponentTypeName,           // Builtin_TryGetComponentOfBaseType

    &FillUserData_Builtin_StartCoroutine,      // Builtin_StartCoroutine,
    nullptr,                                   // Builtin_StopCoroutine,
    nullptr,                                   // Builtin_StopAllCoroutines,
    nullptr,                                   // Builtin_WaitForAll,
    nullptr,                                   // Builtin_WaitForAny,
    nullptr,                                   // Builtin_Yield,

    nullptr,                                   // LastBuiltin,
  };

  static_assert(W_ARRAY_SIZE(s_TypeToFillUserDataFunctions) == WVisualScriptNodeDescription::Type::Count);

  WResult FillUserData(WVisualScriptCompiler::AstNode& inout_astNode, WVisualScriptCompiler* pCompiler, const WDocumentObject* pObject, const WDocumentObject* pEntryObject)
  {
    if (pObject == nullptr)
      return W_SUCCESS;

    auto nodeType = inout_astNode.m_Type;
    W_ASSERT_DEBUG(nodeType >= 0 && nodeType < W_ARRAY_SIZE(s_TypeToFillUserDataFunctions), "Out of bounds access");
    auto func = s_TypeToFillUserDataFunctions[nodeType];

    if (func != nullptr)
    {
      W_SUCCEED_OR_RETURN(func(inout_astNode, pCompiler, pObject, pEntryObject));
    }

    return W_SUCCESS;
  }

} // namespace

//////////////////////////////////////////////////////////////////////////

WVisualScriptCompiler::CompiledModule::CompiledModule()
  : m_ConstantDataStorage(WSharedPtr<WVisualScriptDataDescription>(&m_ConstantDataDesc, nullptr))
{
  // Prevent the data desc from being deleted by fake shared ptr above
  m_ConstantDataDesc.AddRef();
}

WResult WVisualScriptCompiler::CompiledModule::Serialize(WStreamWriter& inout_stream) const
{
  W_ASSERT_DEV(m_sScriptClassName.IsEmpty() == false, "Invalid script class name");

  WStringDeduplicationWriteContext stringDedup(inout_stream);

  WChunkStreamWriter chunk(stringDedup.Begin());
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Header", 1);
    chunk << m_sBaseClassName;
    chunk << m_sScriptClassName;
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("ConstantData", 1);
    W_SUCCEED_OR_RETURN(m_ConstantDataDesc.Serialize(chunk));
    W_SUCCEED_OR_RETURN(m_ConstantDataStorage.Serialize(chunk));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("InstanceData", 1);
    W_SUCCEED_OR_RETURN(m_InstanceDataDesc.Serialize(chunk));
    W_SUCCEED_OR_RETURN(chunk.WriteHashTable(m_InstanceDataMapping.m_Content));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    // Reverse order so functions that are added later (e.g. coroutines) are loaded first
    for (WUInt32 i = m_Functions.GetCount(); i-- > 0;)
    {
      auto& function = m_Functions[i];

      chunk << function.m_sName;
      chunk << function.m_Type;
      chunk << function.m_CoroutineCreationMode;

      W_SUCCEED_OR_RETURN(WVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, function.m_LocalDataDesc, chunk));
    }

    chunk.EndChunk();
  }

  chunk.EndStream();

  return stringDedup.End();
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptCompiler::WVisualScriptCompiler(WVisualScriptNodeManager& ref_nodeManager)
  : m_NodeManager(ref_nodeManager)
{
}

WVisualScriptCompiler::~WVisualScriptCompiler() = default;

void WVisualScriptCompiler::InitModule(WStringView sBaseClassName, WStringView sScriptClassName)
{
  m_Module.m_sBaseClassName = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

WResult WVisualScriptCompiler::AddFunction(WStringView sName, const WDocumentObject* pEntryObject, const WDocumentObject* pParentObject)
{
  W_ASSERT_DEV(&m_NodeManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      WLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return W_FAILURE;
    }
  }

  auto& function = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;

  {
    auto pObjectWithCoroutineMode = pParentObject != nullptr ? pParentObject : pEntryObject;
    auto mode = pObjectWithCoroutineMode->GetTypeAccessor().GetValue("CoroutineMode");
    if (mode.IsA<WInt64>())
    {
      function.m_CoroutineCreationMode = static_cast<WScriptCoroutineCreationMode::Enum>(mode.Get<WInt64>());
    }
    else
    {
      function.m_CoroutineCreationMode = WScriptCoroutineCreationMode::AllowOverlap;
    }
  }

  m_EntryObjects.PushBack(pEntryObject);
  W_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryObjects.GetCount(), "");

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::Compile(WStringView sDebugAstOutputPath)
{
  W_SUCCEED_OR_RETURN(BuildInstanceDataMapping());

  for (WUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto& function = m_Module.m_Functions[i];
    const WDocumentObject* pEntryObject = m_EntryObjects[i];

    AstNode* pEntryAstNode = BuildExecutionFlow(pEntryObject);
    if (pEntryAstNode == nullptr)
    {
      WLog::Error("Failed to build execution flow for function '{}'", function.m_sName);
      return W_FAILURE;
    }

    function.m_Type = pEntryAstNode->m_Type;

    W_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    W_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_TypeConv");

    W_SUCCEED_OR_RETURN(AssignInstanceVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_InstanceVarsAssigned");

    W_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_Replaced");

    W_SUCCEED_OR_RETURN(CopyOutputsToInputs(pEntryAstNode));
    W_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_04_LocalVarsAssigned");

    W_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_CompilationState.Clear();
  }

  W_SUCCEED_OR_RETURN(FinalizeConstantData());

  return W_SUCCESS;
}

// Ast node creation
//////////////////////////////////////////////////////////////////////////

WVisualScriptCompiler::AstNode& WVisualScriptCompiler::CreateAstNode(WVisualScriptNodeDescription::Type::Enum type, WVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution /*= false*/)
{
  auto& node = m_AstNodes.ExpandAndGetRef();
  node.m_Type = type;
  node.m_DeductedDataType = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

WVisualScriptCompiler::AstNode& WVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode = CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = WUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return jumpNode;
}

WVisualScriptCompiler::AstNode* WVisualScriptCompiler::CreateAstNodeFromObject(const WDocumentObject* pObject, const WVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const WDocumentObject* pEntryObject, bool bImplicitOnly /*= false*/)
{
  if (pNodeDesc == nullptr)
  {
    pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  }
  W_ASSERT_DEV(pNodeDesc != nullptr && pNodeDesc->m_Type != WVisualScriptNodeDescription::Type::GetScriptOwner, "Invalid node type");

  if (bImplicitOnly && !pNodeDesc->m_bImplicitExecution)
    return nullptr;

  auto& astNode = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(m_NodeManager.GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
  astNode.m_pObject = pObject;

  if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
    return nullptr;

  if (pNodeDesc->m_bImplicitExecution)
  {
    m_CompilationState.m_DataObjectToAstNode.Insert(pObject, &astNode);
  }
  else
  {
    m_CompilationState.m_ExecObjectToAstNode.Insert(pObject, &astNode);
  }

  return &astNode;
}

WVisualScriptCompiler::DataInput WVisualScriptCompiler::GetOrCreateDefaultPointerNode(const AstNode& node, const WRTTI* pRtti)
{
  if (pRtti == nullptr)
  {
    pRtti = WRTTI::FindTypeByNameHash(node.m_sTargetTypeName.GetHash());
  }

  const bool bIsGameObject = pRtti == WGetStaticRTTI<WGameObject>() || pRtti == WGetStaticRTTI<WGameObjectHandle>();
  const bool bIsWorld = pRtti == WGetStaticRTTI<WWorld>();

  if (bIsWorld || bIsGameObject)
  {
    DataInput dataInput;
    dataInput.m_pSourceNode = m_CompilationState.m_pGetScriptOwnerNode;
    dataInput.m_uiSourcePinIndex = bIsGameObject ? 1 : 0;
    dataInput.m_DataOffset = m_CompilationState.m_pGetScriptOwnerNode->m_DataOutputs[dataInput.m_uiSourcePinIndex].m_DataOffset;
    return dataInput;
  }

  return DataInput();
}

void WVisualScriptCompiler::MarkAsCoroutine(AstNode* pEntryAstNode)
{
  switch (pEntryAstNode->m_Type)
  {
    case WVisualScriptNodeDescription::Type::EntryCall:
      pEntryAstNode->m_Type = WVisualScriptNodeDescription::Type::EntryCall_Coroutine;
      break;
    case WVisualScriptNodeDescription::Type::MessageHandler:
      pEntryAstNode->m_Type = WVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
      break;
    case WVisualScriptNodeDescription::Type::EntryCall_Coroutine:
    case WVisualScriptNodeDescription::Type::MessageHandler_Coroutine:
      // Already a coroutine
      break;
      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

// Pins, inputs and outputs
//////////////////////////////////////////////////////////////////////////

void WVisualScriptCompiler::AddConstantDataInput(AstNode& node, const WVariant& value)
{
  WVisualScriptDataType::Enum dataType = WVisualScriptDataType::FromVariantType(value.GetType());

  WUInt32 uiIndex = WInvalidIndex;
  if (m_Module.m_ConstantDataToIndex.TryGetValue(value, uiIndex) == false)
  {
    auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
    uiIndex = offsetAndCount.m_uiCount;
    ++offsetAndCount.m_uiCount;

    m_Module.m_ConstantDataToIndex.Insert(value, uiIndex);
  }

  auto& dataInput = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = nullptr;
  dataInput.m_uiSourcePinIndex = 0;
  dataInput.m_DataOffset = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
}

WResult WVisualScriptCompiler::AddConstantDataInput(AstNode& node, const WDocumentObject* pObject, const WVisualScriptPin* pPin, WVisualScriptDataType::Enum dataType)
{
  WStringView sPropertyName = pPin->HasDynamicPinProperty() ? pPin->GetDynamicPinProperty() : pPin->GetName();

  WVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);
  if (value.IsValid() && pPin->HasDynamicPinProperty())
  {
    W_ASSERT_DEBUG(value.IsA<WVariantArray>(), "Implementation error");
    value = value.Get<WVariantArray>()[pPin->GetElementIndex()];
  }

  if (value.IsA<WUuid>())
  {
    value = 0;
  }

  WVisualScriptDataType::Enum valueDataType = WVisualScriptDataType::FromVariantType(value.GetType());
  if (dataType != WVisualScriptDataType::Variant)
  {
    value = value.ConvertTo(WVisualScriptDataType::GetVariantType(dataType));
    if (value.IsValid() == false)
    {
      WLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), WVisualScriptDataType::GetName(valueDataType), WVisualScriptDataType::GetName(dataType));
      return W_FAILURE;
    }
  }

  AddConstantDataInput(node, value);

  return W_SUCCESS;
}

void WVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, WUInt32 uiSourcePinIndex, WVisualScriptDataType::Enum dataType)
{
  auto& dataInput = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = pSourceNode;
  dataInput.m_uiSourcePinIndex = uiSourcePinIndex;
  dataInput.m_DataOffset.m_uiType = dataType;
}

void WVisualScriptCompiler::AddDataOutput(AstNode& node, WVisualScriptDataType::Enum dataType)
{
  auto& dataOutput = node.m_DataOutputs.ExpandAndGetRef();
  dataOutput.m_DataOffset = DataOffset(m_CompilationState.m_uiNextLocalVarId++, dataType, DataOffset::Source::Local);
}

WVisualScriptCompiler::DataOutput& WVisualScriptCompiler::GetDataOutputFromInput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_DataOutputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_DataOutputs[dataInput.m_uiSourcePinIndex];
  }

  W_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

void WVisualScriptCompiler::ConnectExecution(AstNode& sourceNode, AstNode& targetNode, WUInt32 uiSourcePinIndex /*= WInvalidIndex*/)
{
  if (uiSourcePinIndex == WInvalidIndex)
  {
    // New connection
    ExecInput execInput;
    execInput.m_pSourceNode = &sourceNode;
    execInput.m_uiSourcePinIndex = sourceNode.m_ExecOutputs.GetCount();

    targetNode.m_ExecInputs.PushBack(execInput);

    ExecOutput execOutput;
    execOutput.m_pTargetNode = &targetNode;

    sourceNode.m_ExecOutputs.PushBack(execOutput);
  }
  else
  {
    ExecInput execInput;
    execInput.m_pSourceNode = &sourceNode;
    execInput.m_uiSourcePinIndex = uiSourcePinIndex;

    targetNode.m_ExecInputs.PushBack(execInput);

    sourceNode.m_ExecOutputs.EnsureCount(uiSourcePinIndex + 1);
    auto& execOutput = sourceNode.m_ExecOutputs[uiSourcePinIndex];
    execOutput.m_pTargetNode = &targetNode;
  }
}

void WVisualScriptCompiler::DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, WUInt32 uiSourcePinIndex)
{
  ExecInput execInputToRemove;
  execInputToRemove.m_pSourceNode = &sourceNode;
  execInputToRemove.m_uiSourcePinIndex = uiSourcePinIndex;
  W_VERIFY(targetNode.m_ExecInputs.RemoveAndCopy(execInputToRemove), "");

  sourceNode.m_ExecOutputs[uiSourcePinIndex].m_pTargetNode = nullptr;
}

void WVisualScriptCompiler::ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  for (auto& execInput : node.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, firstNewNode, execInput.m_uiSourcePinIndex);
  }
  node.m_ExecInputs.Clear();

  ConnectExecution(lastNewNode, node);
}

void WVisualScriptCompiler::ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  W_ASSERT_DEV(node.m_ExecOutputs.GetCount() == 1, "This function only works for nodes with a single exec output pin");
  AstNode* pNodeAfter = node.m_ExecOutputs[0].m_pTargetNode;

  ConnectExecution(node, firstNewNode, 0);

  pNodeAfter->m_ExecInputs.Clear();
  ConnectExecution(lastNewNode, *pNodeAfter);
}

void WVisualScriptCompiler::ReplaceExecution(AstNode& oldNode, AstNode& newNode)
{
  W_ASSERT_DEBUG(newNode.m_ExecInputs.IsEmpty() && newNode.m_ExecOutputs.IsEmpty(), "New node must not have any connections yet");

  for (auto& execInput : oldNode.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, newNode, execInput.m_uiSourcePinIndex);
  }
  oldNode.m_ExecInputs.Clear();

  for (WUInt32 i = 0; i < oldNode.m_ExecOutputs.GetCount(); ++i)
  {
    AstNode* pTargetNode = oldNode.m_ExecOutputs[i].m_pTargetNode;
    if (pTargetNode != nullptr)
    {
      DisconnectExecution(oldNode, *pTargetNode, i);
      ConnectExecution(newNode, *pTargetNode, i);
    }
    else
    {
      newNode.m_ExecOutputs.PushBack({nullptr});
    }
  }
  oldNode.m_ExecOutputs.Clear();
}

WVisualScriptCompiler::DataOffset WVisualScriptCompiler::GetInstanceDataOffset(WHashedString sName, WVisualScriptDataType::Enum dataType)
{
  WVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    WLog::Error("Invalid variable named '{}'", sName);
    return DataOffset();
  }

  W_ASSERT_DEBUG(instanceData.m_DataOffset.m_uiType == dataType, "Data type mismatch");
  return instanceData.m_DataOffset;
}

// Compilation steps
//////////////////////////////////////////////////////////////////////////

WResult WVisualScriptCompiler::BuildInstanceDataMapping()
{
  WTempHybridArray<WVisualScriptVariable, 16> variables;
  m_NodeManager.GetAllVariables(variables);

  for (auto& variable : variables)
  {
    auto dataType = variable.m_TypeDecl.GetDataType();

    W_ASSERT_DEV(dataType == WVisualScriptDataType::Variant ||
                    ((dataType == WVisualScriptDataType::GameObject || dataType == WVisualScriptDataType::Component || dataType == WVisualScriptDataType::TypedPointer) && variable.m_DefaultValue.IsValid() == false) ||
                    dataType == WVisualScriptDataType::FromVariantType(variable.m_DefaultValue.GetType()),
      "Data type mismatch");

    auto& offsetAndCount = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];

    WVisualScriptInstanceData instanceData;
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType = dataType;
    instanceData.m_DataOffset.m_uiSource = DataOffset::Source::Instance;
    instanceData.m_DefaultValue = variable.m_DefaultValue;

    ++offsetAndCount.m_uiCount;

    m_Module.m_InstanceDataMapping.m_Content.Insert(variable.m_sName, instanceData);
  }

  return W_SUCCESS;
}

WVisualScriptCompiler::AstNode* WVisualScriptCompiler::BuildExecutionFlow(const WDocumentObject* pEntryObject)
{
  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject, nullptr, pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (WVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(WVisualScriptNodeDescription::Type::EntryCall);
    ConnectExecution(astNode, *pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  WTempHybridArray<const WVisualScriptPin*, 16> pins;
  WTempHybridArray<const WDocumentObject*, 64> objectStack;
  objectStack.PushBack(pEntryObject);

  while (objectStack.IsEmpty() == false)
  {
    const WDocumentObject* pObject = objectStack.PeekBack();
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    W_VERIFY(m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (WVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_NodeManager.GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_NodeManager.GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_ExecOutputs.PushBack({nullptr});
        continue;
      }

      W_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const WDocumentObject* pNextObject = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pNextObject, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextObject, nullptr, pEntryObject);
        if (pNextAstNode == nullptr)
          return nullptr;

        objectStack.PushBack(pNextObject);
      }

      ConnectExecution(*pAstNode, *pNextAstNode);
    }
  }

  // Always add a GetScriptOwner node directly after the entry node
  {
    W_ASSERT_DEBUG(m_CompilationState.m_pGetScriptOwnerNode == nullptr, "");

    auto& getScriptOwnerNode = CreateAstNode(WVisualScriptNodeDescription::Type::GetScriptOwner);
    AddDataOutput(getScriptOwnerNode, WVisualScriptDataType::TypedPointer);
    AddDataOutput(getScriptOwnerNode, WVisualScriptDataType::GameObject);
    AddDataOutput(getScriptOwnerNode, WVisualScriptDataType::Component);

    m_CompilationState.m_pGetScriptOwnerNode = &getScriptOwnerNode;

    ExecuteAfter(*pEntryAstNode, getScriptOwnerNode, getScriptOwnerNode);
  }

  return pEntryAstNode;
}

WResult WVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, AstNode*& out_pFirstDataNode, AstNode*& out_pLastDataNode)
{
  if (pEntryAstNode->m_pObject == nullptr)
    return W_SUCCESS;

  WTempHybridArray<const WVisualScriptPin*, 16> pins;

  struct ObjectContext
  {
    const WDocumentObject* m_pObject = nullptr;
    const WVisualScriptNodeRegistry::NodeDesc* m_pNodeDesc = nullptr;
  };

  WTempHybridArray<ObjectContext, 32> objectStack;
  objectStack.PushBack({pEntryAstNode->m_pObject, WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pEntryAstNode->m_pObject->GetType())});

  // Start from scratch for each data stack
  m_CompilationState.m_DataObjectToAstNode.Clear();
  m_CompilationState.m_DataObjectToAstNode.Insert(pEntryAstNode->m_pObject, pEntryAstNode);

  WTempHybridArray<AstNode*, 32> dataStack;

  while (objectStack.IsEmpty() == false)
  {
    auto& objCtx = objectStack.PeekBack();
    const WDocumentObject* pObject = objCtx.m_pObject;
    const WVisualScriptNodeRegistry::NodeDesc* pNodeDesc = objCtx.m_pNodeDesc;
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    W_VERIFY(m_CompilationState.m_DataObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (pAstNode != pEntryAstNode)
    {
      dataStack.PushBack(pAstNode);
    }

    m_NodeManager.GetInputDataPins(pObject, pins);
    WUInt32 uiNextInputPinIndex = 0;

    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.IsExecutionPin())
        continue;

      AstNode* pAstNodeToAddInput = pAstNode;
      bool bArrayInput = false;
      if (pinDesc.m_bReplaceWithArray && pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
      {
        const WAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
        if (pProp == nullptr)
          return W_FAILURE;

        if (pProp->GetCategory() == WPropertyCategory::Array)
        {
          auto pMakeArrayAstNode = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          pMakeArrayAstNode->m_pObject = pObject;
          AddDataOutput(*pMakeArrayAstNode, WVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, WVisualScriptDataType::Array);

          dataStack.PushBack(pMakeArrayAstNode);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        if (pPin->GetDynamicPinProperty() != pinDesc.m_sDynamicPinProperty)
          break;

        WVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == WVisualScriptDataType::Invalid)
        {
          WLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return W_FAILURE;
        }

        auto connections = m_NodeManager.GetConnections(*pPin);
        if (pPin->IsRequired() && targetDataType != WVisualScriptDataType::GameObject && connections.IsEmpty())
        {
          WLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return W_FAILURE;
        }

        auto dataInputType = bArrayInput ? WVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (WVisualScriptDataType::IsPointer(dataInputType))
          {
            auto defaultInput = GetOrCreateDefaultPointerNode(*pAstNode, pPin->GetDataType());
            AddDataInput(*pAstNodeToAddInput, defaultInput.m_pSourceNode, defaultInput.m_uiSourcePinIndex, defaultInput.m_DataOffset.GetType());

            if (defaultInput.m_pSourceNode != nullptr)
            {
              if (defaultInput.m_pSourceNode->m_ExecInputs.IsEmpty())
              {
                dataStack.RemoveAndCopy(defaultInput.m_pSourceNode);
                dataStack.PushBack(defaultInput.m_pSourceNode);
              }
            }
          }
          else
          {
            W_SUCCEED_OR_RETURN(AddConstantDataInput(*pAstNodeToAddInput, pObject, pPin, dataInputType));
          }
        }
        else
        {
          auto& sourcePin = static_cast<const WVisualScriptPin&>(connections[0]->GetSourcePin());
          const WDocumentObject* pSourceObject = sourcePin.GetParent();
          auto pSourceNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pSourceObject->GetType());

          if (pSourceNodeDesc->m_Type == WVisualScriptNodeDescription::Type::GetScriptOwner)
          {
            AstNode* pSourceAstNode = m_CompilationState.m_pGetScriptOwnerNode;
            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
          else if (pSourceNodeDesc->m_Type == WVisualScriptNodeDescription::Type::Builtin_Constant)
          {
            WVariant value = pSourceObject->GetTypeAccessor().GetValue("Value");
            AddConstantDataInput(*pAstNodeToAddInput, value);
          }
          else
          {
            AstNode* pSourceAstNode;
            if (m_CompilationState.m_DataObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
            {
              pSourceAstNode = CreateAstNodeFromObject(pSourceObject, pSourceNodeDesc, nullptr, true);
              if (pSourceAstNode != nullptr)
              {
                objectStack.PushBack({pSourceObject, pSourceNodeDesc});
              }
              else if (pSourceAstNode == nullptr)
              {
                if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
                {
                  WLog::Error("The source node '{}' is not executed in the current function.", GetNiceTypeName(pSourceObject));
                  return W_FAILURE;
                }
              }
            }

            WVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
            if (sourceDataType == WVisualScriptDataType::Invalid)
            {
              WLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
              return W_FAILURE;
            }

            if (sourcePin.CanConvertTo(*pPin) == false)
            {
              WLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
              return W_FAILURE;
            }

            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
        }

        ++uiNextInputPinIndex;
      }
    }

    m_NodeManager.GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      AddDataOutput(*pAstNode, FinalizeDataType(pPin->GetResolvedScriptDataType()));
    }
  }

  // Traverse in topological order and connect executions
  WTempHybridArray<AstNode*, 32> sortedDataStack;

  while (dataStack.IsEmpty() == false)
  {
    // Find next node with all data dependencies already in the sorted stack
    AstNode* nextNode = nullptr;
    for (WUInt32 i = dataStack.GetCount(); i-- > 0;)
    {
      AstNode* nextNodeCandiate = dataStack[i];
      const bool bValidCandidate = [&]()
      {
        m_NodeManager.GetInputDataPins(nextNodeCandiate->m_pObject, pins);
        for (auto pPin : pins)
        {
          auto connections = m_NodeManager.GetConnections(*pPin);
          for (auto pConnection : connections)
          {
            const WDocumentObject* pSourceObject = pConnection->GetSourcePin().GetParent();

            AstNode* pSourceAstNode = nullptr;
            if (!m_CompilationState.m_DataObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode))
            {
              // Not part of the data stack so we can ignore it
              continue;
            }

            if (!sortedDataStack.Contains(pSourceAstNode))
            {
              return false;
            }
          }
        }

        return true;
      }();

      if (bValidCandidate)
      {
        nextNode = nextNodeCandiate;
        dataStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (nextNode == nullptr)
    {
      W_REPORT_FAILURE("Data connection corrupted or loop detected");
      return W_FAILURE;
    }

    if (sortedDataStack.IsEmpty() == false)
    {
      ConnectExecution(*sortedDataStack.PeekBack(), *nextNode);
    }
    sortedDataStack.PushBack(nextNode);
  }

  if (sortedDataStack.IsEmpty())
  {
    out_pFirstDataNode = nullptr;
    out_pLastDataNode = nullptr;
  }
  else
  {
    out_pFirstDataNode = sortedDataStack[0];
    out_pLastDataNode = sortedDataStack.PeekBack();
  }

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      if (pAstNode->m_bImplicitExecution)
        return VisitorResult::Continue;

      AstNode* pFirstDataNode = nullptr;
      AstNode* pLastDataNode = nullptr;
      if (BuildDataStack(pAstNode, pFirstDataNode, pLastDataNode).Failed())
        return VisitorResult::Error;

      if (pFirstDataNode == nullptr || pLastDataNode == nullptr)
        return VisitorResult::Continue;

      // Connect data stack
      ExecuteBefore(*pAstNode, *pFirstDataNode, *pLastDataNode);

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  auto InsertBuiltinTypeConversion = [&](AstNode& node, DataInput& dataInput, WVisualScriptDataType::Enum inputDataType, WVisualScriptDataType::Enum outputDataType) -> AstNode&
  {
    auto nodeType = WVisualScriptNodeDescription::Type::GetConversionType(inputDataType);

    auto& conversionNode = CreateAstNode(nodeType, outputDataType, true);
    AddDataInput(conversionNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, outputDataType);
    AddDataOutput(conversionNode, inputDataType);

    dataInput.m_pSourceNode = &conversionNode;
    dataInput.m_uiSourcePinIndex = 0;

    ExecuteBefore(node, conversionNode, conversionNode);

    return conversionNode;
  };

  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnected() == false)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);

        auto inputDataType = dataInput.m_DataOffset.GetType();
        auto outputDataType = dataOutput.m_DataOffset.GetType();

        if (dataOutput.m_DataOffset.GetType() != inputDataType)
        {
          if (WVisualScriptDataType::IsNumberOrBool(outputDataType) && WVisualScriptDataType::IsVector(inputDataType))
          {
            AstNode* pSourceNode = dataInput.m_pSourceNode;
            WUInt32 uiSourcePinIndex = dataInput.m_uiSourcePinIndex;
            if (outputDataType != WVisualScriptDataType::Float)
            {
              auto& conversionNode = InsertBuiltinTypeConversion(*pAstNode, dataInput, WVisualScriptDataType::Float, outputDataType);
              pSourceNode = &conversionNode;
              uiSourcePinIndex = 0;
            }

            auto& makeVecXNode = CreateAstNode(WVisualScriptNodeDescription::Type::ReflectedFunction, inputDataType, true);
            const WRTTI* pRtti = WVisualScriptDataType::GetRtti(inputDataType);
            makeVecXNode.m_sTargetTypeName.Assign(pRtti->GetTypeName());

            WVariantArray a;
            a.PushBack(WMakeHashedString("Make"));
            makeVecXNode.m_Value = a;

            AddDataInput(makeVecXNode, pSourceNode, uiSourcePinIndex, WVisualScriptDataType::Float);
            AddDataInput(makeVecXNode, pSourceNode, uiSourcePinIndex, WVisualScriptDataType::Float);
            if (inputDataType >= WVisualScriptDataType::Vector3)
              AddDataInput(makeVecXNode, pSourceNode, uiSourcePinIndex, WVisualScriptDataType::Float);
            if (inputDataType == WVisualScriptDataType::Vector4)
              AddDataInput(makeVecXNode, pSourceNode, uiSourcePinIndex, WVisualScriptDataType::Float);

            AddDataOutput(makeVecXNode, inputDataType);

            dataInput.m_pSourceNode = &makeVecXNode;
            dataInput.m_uiSourcePinIndex = 0;

            ExecuteBefore(*pAstNode, makeVecXNode, makeVecXNode);
          }
          else if (outputDataType == WVisualScriptDataType::Vector3 && inputDataType == WVisualScriptDataType::Transform)
          {
            auto& makeTransformNode = CreateAstNode(WVisualScriptNodeDescription::Type::ReflectedFunction, inputDataType, true);
            makeTransformNode.m_sTargetTypeName.Assign("WTransform");

            WVariantArray a;
            a.PushBack(WMakeHashedString("Make"));
            makeTransformNode.m_Value = a;

            AddDataInput(makeTransformNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, WVisualScriptDataType::Vector3);
            AddConstantDataInput(makeTransformNode, WQuat::MakeIdentity());
            AddConstantDataInput(makeTransformNode, WVec3(1));
            AddDataOutput(makeTransformNode, WVisualScriptDataType::Transform);

            dataInput.m_pSourceNode = &makeTransformNode;
            dataInput.m_uiSourcePinIndex = 0;

            ExecuteBefore(*pAstNode, makeTransformNode, makeTransformNode);
          }
          else
          {
            InsertBuiltinTypeConversion(*pAstNode, dataInput, inputDataType, outputDataType);
          }
        }
      }

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::ReplaceLoop(AstNode* pLoopNode)
{
  AstNode* pLoopInitStart = nullptr;
  AstNode* pLoopInitEnd = nullptr;

  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd = nullptr;

  DataInput conditionDataInput;
  conditionDataInput.m_uiSourcePinIndex = 0;
  conditionDataInput.m_DataOffset.m_uiType = WVisualScriptDataType::Bool;

  AstNode* pLoopIncrement = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex = nullptr;

  AstNode* pLoopBody = pLoopNode->m_ExecOutputs[0].m_pTargetNode;
  AstNode* pLoopCompleted = pLoopNode->m_ExecOutputs[1].m_pTargetNode;

  auto loopType = pLoopNode->m_Type;

  if (loopType == WVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    conditionDataInput = pLoopNode->m_DataInputs[0];

    if (conditionDataInput.m_pSourceNode != nullptr)
    {
      pLoopConditionEnd = conditionDataInput.m_pSourceNode;

      pLoopConditionStart = conditionDataInput.m_pSourceNode;
      while (pLoopConditionStart->m_ExecInputs.GetCount() == 1 && pLoopConditionStart->m_ExecInputs[0].m_pSourceNode->m_bImplicitExecution)
      {
        pLoopConditionStart = pLoopConditionStart->m_ExecInputs[0].m_pSourceNode;
      }
    }
  }
  else if (loopType == WVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_DataInputs[0];
    auto& lastIndexInput = pLoopNode->m_DataInputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_ToInt, WVisualScriptDataType::Int);
      pLoopInitStart->m_DataInputs.PushBack(firstIndexInput);
      AddDataOutput(*pLoopInitStart, WVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;
      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Compare, WVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = WInt64(WComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, WVisualScriptDataType::Int);
      pLoopConditionStart->m_DataInputs.PushBack(lastIndexInput);
      AddDataOutput(*pLoopConditionStart, WVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrement = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Add, WVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, WVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Dummy input that is not used at runtime but prevents the lastIndexInput from being re-used across the loop's lifetime
      pLoopIncrement->m_DataInputs.PushBack(lastIndexInput);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }
  }
  else if (loopType == WVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == WVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse = (loopType == WVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto& arrayInput = pLoopNode->m_DataInputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopInitStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopInitStart, WVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Subtract, WVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, WVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitEnd, 1);
      AddDataOutput(*pLoopInitEnd, WVisualScriptDataType::Int);

      ConnectExecution(*pLoopInitStart, *pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_ToInt, WVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitStart, 0);
      AddDataOutput(*pLoopInitStart, WVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    if (isReverse)
    {
      pLoopConditionStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Compare, WVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = WInt64(WComparisonOperator::GreaterEqual);
      AddDataInput(*pLoopConditionStart, pLoopIndex, 0, WVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopConditionStart, 0);
      AddDataOutput(*pLoopConditionStart, WVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopConditionStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopConditionStart, WVisualScriptDataType::Int);

      pLoopConditionEnd = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Compare, WVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = WInt64(WComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, WVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, WVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, WVisualScriptDataType::Bool);

      ConnectExecution(*pLoopConditionStart, *pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? WVisualScriptNodeDescription::Type::Builtin_Subtract : WVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrement = &CreateAstNode(incType, WVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, WVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      pLoopIncrement->m_DataInputs.PushBack(arrayInput);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }

    pLoopElement = &CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Array_GetElement, WVisualScriptDataType::Invalid, true);
    pLoopElement->m_DataInputs.PushBack(arrayInput);
    AddDataInput(*pLoopElement, pLoopIndex, 0, WVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, WVisualScriptDataType::Variant);
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  {
    if (conditionDataInput.m_pSourceNode == nullptr && conditionDataInput.m_DataOffset.IsLocal())
    {
      conditionDataInput.m_pSourceNode = pLoopConditionEnd;
    }

    auto& branchNode = CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Branch);
    branchNode.m_DataInputs.PushBack(conditionDataInput);

    ReplaceExecution(*pLoopNode, branchNode);

    if (pLoopConditionStart == nullptr)
    {
      pLoopConditionStart = &branchNode;
    }
    else if (pLoopConditionEnd->m_ExecOutputs.IsEmpty())
    {
      ExecuteBefore(branchNode, *pLoopConditionStart, *pLoopConditionEnd);
    }

    if (pLoopInitStart != nullptr)
    {
      ExecuteBefore(*pLoopConditionStart, *pLoopInitStart, *pLoopInitEnd);
    }

    if (pLoopElement != nullptr && pLoopBody != nullptr)
    {
      ExecuteBefore(*pLoopBody, *pLoopElement, *pLoopElement);
    }
  }

  AstNode* pJumpNode = &CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrement != nullptr)
  {
    ConnectExecution(*pLoopIncrement, *pJumpNode);
    pJumpNode = pLoopIncrement;
  }

  WTempHybridArray<AstNode*, 8> nodesConnectedToBreak;

  if (TraverseAstDepthFirst(pLoopBody,
        [&](AstNode*& pAstNode)
        {
          W_ASSERT_DEV(WVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) == false, "Nested Loops should have been resolved already");

          for (WUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
          {
            auto& execOutput = pAstNode->m_ExecOutputs[i];
            if (execOutput.m_pTargetNode == nullptr)
            {
              ConnectExecution(*pAstNode, *pJumpNode, i);
            }
            else if (execOutput.m_pTargetNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_Break)
            {
              // handle breaks after this traversal, otherwise we could end up traversing to nodes outside the loop
              nodesConnectedToBreak.PushBack(pAstNode);
            }
          }

          for (auto& dataInput : pAstNode->m_DataInputs)
          {
            if (loopType == WVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
                loopType == WVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
            {
              if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
              {
                dataInput.m_pSourceNode = pLoopElement;
              }
              else if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 1)
              {
                dataInput.m_pSourceNode = pLoopIndex;
                dataInput.m_uiSourcePinIndex = 0;
              }
            }
            else
            {
              if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
              {
                dataInput.m_pSourceNode = pLoopIndex;
              }
            }
          }

          return VisitorResult::Continue;
        })
        .Failed())
  {
    return W_FAILURE;
  }

  // Go through all loop body nodes and check if they have data connections to nodes outside the loop.
  // If so add the data input to the jump node to prevent register re-use inside the loop.
  if (pLoopBody != nullptr)
  {
    m_CompilationState.m_VisitedNodes.Insert(pLoopBody);
  }

  for (auto pAstNode : m_CompilationState.m_VisitedNodes)
  {
    if (pAstNode == pLoopIncrement)
      continue;

    for (auto& dataInput : pAstNode->m_DataInputs)
    {
      auto pSourceNode = dataInput.m_pSourceNode;
      if (pSourceNode == nullptr || pSourceNode == pLoopIndex || pSourceNode == pLoopElement)
        continue;

      if (!m_CompilationState.m_VisitedNodes.Contains(dataInput.m_pSourceNode) && pJumpNode->m_DataInputs.Contains(dataInput) == false)
      {
        pJumpNode->m_DataInputs.PushBack(dataInput);
      }
    }
  }

  // Remove break nodes from the execution flow and connect them to the loop completed node
  for (auto pAstNode : nodesConnectedToBreak)
  {
    for (WUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];
      if (execOutput.m_pTargetNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_Break)
      {
        DisconnectExecution(*pAstNode, *execOutput.m_pTargetNode, i);
        if (pLoopCompleted != nullptr)
        {
          ConnectExecution(*pAstNode, *pLoopCompleted, i);
        }
      }
    }
  }

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  WTempHybridArray<AstNode*, 64> unsupportedNodes;

  if (TraverseAstDepthFirst(pEntryAstNode,
        [&](AstNode*& pAstNode)
        {
          if (WVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) || pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_CompareExec)
          {
            W_ASSERT_DEBUG(unsupportedNodes.Contains(pAstNode) == false, "");
            unsupportedNodes.PushBack(pAstNode);
          }

          return VisitorResult::Continue;
        })
        .Failed())
  {
    return W_FAILURE;
  }

  // Replace unsupported nodes backwards so that we replace inner loops first, order doesn't matter for the other nodes
  for (WUInt32 i = unsupportedNodes.GetCount(); i-- > 0;)
  {
    AstNode* pAstNode = unsupportedNodes[i];

    if (pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_CompareExec)
    {
      auto& compareNode = CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Compare, pAstNode->m_DeductedDataType, true);
      compareNode.m_Value = pAstNode->m_Value;
      compareNode.m_DataInputs = pAstNode->m_DataInputs;
      AddDataOutput(compareNode, WVisualScriptDataType::Bool);

      auto& branchNode = CreateAstNode(WVisualScriptNodeDescription::Type::Builtin_Branch);
      AddDataInput(branchNode, &compareNode, 0, WVisualScriptDataType::Bool);

      ReplaceExecution(*pAstNode, branchNode);
      ExecuteBefore(branchNode, compareNode, compareNode);
    }
    else if (WVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type))
    {
      W_SUCCEED_OR_RETURN(ReplaceLoop(pAstNode));
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::AssignInstanceVariables(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        W_ASSERT_DEBUG(pSourceNode != nullptr, "Data input source node should be always valid for local inputs");

        if (pSourceNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_GetVariable)
        {
          auto& dataOutput = GetDataOutputFromInput(dataInput);

          WHashedString sName;
          sName.Assign(pSourceNode->m_Value.Get<WString>());

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
          dataInput.m_DataOffset = GetInstanceDataOffset(sName, dataOutput.m_DataOffset.GetType());

          // Remove the GetVariable node from the execution flow
          if (pSourceNode->m_ExecOutputs.IsEmpty() == false)
          {
            AstNode* pNodeAfterGetVariable = pSourceNode->m_ExecOutputs[0].m_pTargetNode;
            pNodeAfterGetVariable->m_ExecInputs.Clear();

            for (auto& execInput : pSourceNode->m_ExecInputs)
            {
              ConnectExecution(*execInput.m_pSourceNode, *pNodeAfterGetVariable, execInput.m_uiSourcePinIndex);
            }

            pSourceNode->m_ExecInputs.Clear();
            pSourceNode->m_ExecOutputs.Clear();
          }
        }
      }

      if (pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_SetVariable ||
          pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_IncVariable ||
          pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_DecVariable)
      {
        WHashedString sName;
        sName.Assign(pAstNode->m_Value.Get<WString>());

        DataOffset dataOffset = GetInstanceDataOffset(sName, pAstNode->m_DeductedDataType);

        if (pAstNode->m_Type != WVisualScriptNodeDescription::Type::Builtin_SetVariable)
        {
          if (pAstNode->m_DataInputs.IsEmpty())
          {
            auto& dataInput = pAstNode->m_DataInputs.ExpandAndGetRef();
            dataInput.m_DataOffset = dataOffset;
          }
        }

        W_ASSERT_DEBUG(pAstNode->m_DataOutputs.GetCount() > 0, "");
        auto& dataOutput = pAstNode->m_DataOutputs[0];
        dataOutput.m_DataOffset = dataOffset;
      }

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, WVisualScriptDataDescription& inout_localDataDesc)
{
  m_CompilationState.m_LiveLocalVars.Reserve(m_CompilationState.m_uiNextLocalVarId);
  for (WUInt32 i = 0; i < m_CompilationState.m_uiNextLocalVarId; ++i)
  {
    auto& liveLocalVar = m_CompilationState.m_LiveLocalVars.ExpandAndGetRef();
    liveLocalVar.m_uiId = i;
  }

  WUInt32 uiNodeIndex = 0;

  WResult r = TraverseAstTopologicalOrder(pEntryAstNode,
    [&](const AstNode* pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        const WUInt32 id = dataInput.m_DataOffset.m_uiByteOffset;
        auto& liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

        liveLocalVar.m_uiEnd = WMath::Max(liveLocalVar.m_uiEnd, uiNodeIndex);
      }

      for (auto& dataOutput : pAstNode->m_DataOutputs)
      {
        if (dataOutput.IsValidAndLocal() == false)
          continue;

        const WUInt32 id = dataOutput.m_DataOffset.m_uiByteOffset;
        auto& liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

        liveLocalVar.m_uiId = id;
        liveLocalVar.m_DataOffset = dataOutput.m_DataOffset;
        liveLocalVar.m_uiStart = WMath::Min(liveLocalVar.m_uiStart, uiNodeIndex);
      }

      ++uiNodeIndex;
      return VisitorResult::Continue;
    });
  if (r.Failed())
    return W_FAILURE;

  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort lifetime by start index
  m_CompilationState.m_LiveLocalVars.Sort(
    [](const LiveLocalVar& a, const LiveLocalVar& b)
    {
      if (a.m_uiStart != b.m_uiStart)
        return a.m_uiStart < b.m_uiStart;

      return a.m_uiId < b.m_uiId;
    });

  // Assign local vars
  WTempHybridArray<LiveLocalVar, 64> activeIntervals;
  WTempHybridArray<DataOffset, 64> freeDataOffsets;

  for (auto& liveInterval : m_CompilationState.m_LiveLocalVars)
  {
    // Expire old intervals with less comparison instead of less equal (as in the original paper) so we don't end up using the same data as input and output
    for (WUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd < liveInterval.m_uiStart)
      {
        freeDataOffsets.PushBack(activeInterval.m_DataOffset);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Unused var
    if (liveInterval.m_uiEnd <= liveInterval.m_uiStart)
    {
      liveInterval.m_DataOffset = {};
      continue;
    }

    // Allocate local var
    {
      DataOffset dataOffset;
      dataOffset.m_uiType = liveInterval.m_DataOffset.m_uiType;

      for (WUInt32 i = 0; i < freeDataOffsets.GetCount(); ++i)
      {
        auto freeDataOffset = freeDataOffsets[i];
        if (freeDataOffset.m_uiType == dataOffset.m_uiType)
        {
          dataOffset = freeDataOffset;
          freeDataOffsets.RemoveAtAndSwap(i);
          break;
        }
      }

      if (dataOffset.IsValid() == false)
      {
        W_ASSERT_DEBUG(dataOffset.GetType() < WVisualScriptDataType::Count, "Invalid data type");
        auto& offsetAndCount = inout_localDataDesc.m_PerTypeInfo[dataOffset.m_uiType];
        dataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
        ++offsetAndCount.m_uiCount;
      }

      liveInterval.m_DataOffset = dataOffset;
    }

    activeIntervals.PushBack(liveInterval);
  }

  // Sort by id and copy back to outputs and inputs
  m_CompilationState.m_LiveLocalVars.Sort([](const LiveLocalVar& a, const LiveLocalVar& b)
    { return a.m_uiId < b.m_uiId; });

  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        const WUInt32 id = dataInput.m_DataOffset.m_uiByteOffset;
        W_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
        dataInput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
      }

      for (auto& dataOutput : pAstNode->m_DataOutputs)
      {
        if (dataOutput.IsValidAndLocal() == false)
          continue;

        const WUInt32 id = dataOutput.m_DataOffset.m_uiByteOffset;
        W_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
        dataOutput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
      }

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::CopyOutputsToInputs(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnected() == false)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);
        dataInput.m_DataOffset.m_uiByteOffset = dataOutput.m_DataOffset.m_uiByteOffset;
        dataInput.m_DataOffset.m_uiSource = dataOutput.m_DataOffset.m_uiSource;
        W_ASSERT_DEBUG(dataInput.m_DataOffset.GetType() == WVisualScriptDataType::Variant || dataInput.m_DataOffset.GetType() == dataOutput.m_DataOffset.GetType(), "");
      }

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::BuildNodeDescriptions(AstNode* pEntryAstNode, WDynamicArray<WVisualScriptNodeDescription>& out_NodeDescriptions)
{
  WHashTable<const AstNode*, WUInt32> astNodeToNodeDescIndices;
  out_NodeDescriptions.Clear();

  auto CreateNodeDesc = [&](const AstNode& astNode, WUInt32& out_uiNodeDescIndex) -> WResult
  {
    out_uiNodeDescIndex = out_NodeDescriptions.GetCount();

    auto& nodeDesc = out_NodeDescriptions.ExpandAndGetRef();
    nodeDesc.m_Type = astNode.m_Type;
    nodeDesc.m_DeductedDataType = astNode.m_DeductedDataType;
    nodeDesc.m_sTargetTypeName = astNode.m_sTargetTypeName;
    nodeDesc.m_Value = astNode.m_Value;

    for (auto& dataInput : astNode.m_DataInputs)
    {
      nodeDesc.m_InputDataOffsets.PushBack(dataInput.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_DataOutputs)
    {
      nodeDesc.m_OutputDataOffsets.PushBack(dataOutput.m_DataOffset);
    }

    nodeDesc.m_ExecutionIndices.SetCount(astNode.m_ExecOutputs.GetCount(), WSmallInvalidIndex);

    W_VERIFY(astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex) == false, "");
    return W_SUCCESS;
  };

  return TraverseAstTopologicalOrder(pEntryAstNode,
    [&](const AstNode* pAstNode)
    {
      WUInt32 uiTargetIndex = 0;

      // Jump nodes should not end up in the final node descriptions
      if (pAstNode->m_Type == WVisualScriptNodeDescription::Type::Builtin_Jump)
      {
        WUInt64 uiPtr = pAstNode->m_Value.Get<WUInt64>();
        const AstNode* pTargetAstNode = *reinterpret_cast<const AstNode**>(&uiPtr);

        if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiTargetIndex) == false)
          return VisitorResult::Error;
      }
      else
      {
        if (CreateNodeDesc(*pAstNode, uiTargetIndex).Failed())
          return VisitorResult::Error;
      }

      for (auto& execInput : pAstNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode == nullptr)
          continue;

        WUInt32 uiSourceIndex = 0;
        W_VERIFY(astNodeToNodeDescIndices.TryGetValue(execInput.m_pSourceNode, uiSourceIndex), "Topological sort failed");

        auto pSourceNodeDesc = &out_NodeDescriptions[uiSourceIndex];
        pSourceNodeDesc->m_ExecutionIndices[execInput.m_uiSourcePinIndex] = uiTargetIndex;
      }

      return VisitorResult::Continue;
    });
}

WResult WVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataStorage.AllocateStorage(WFoundation::GetDefaultAllocator());

  for (auto& it : m_Module.m_ConstantDataToIndex)
  {
    const WVariant& value = it.Key();
    WUInt32 uiIndex = it.Value();

    auto scriptDataType = WVisualScriptDataType::FromVariantType(value.GetType());
    if (scriptDataType == WVisualScriptDataType::Invalid)
    {
      scriptDataType = WVisualScriptDataType::Variant;
    }

    auto dataOffset = m_Module.m_ConstantDataDesc.GetOffset(scriptDataType, uiIndex, DataOffset::Source::Constant);

    m_Module.m_ConstantDataStorage.SetDataFromVariant(dataOffset, value, 0);
  }

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::TraverseAstDepthFirst(AstNode* pEntryAstNode, WDelegate<VisitorResult(AstNode*& pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return W_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  WTempHybridArray<AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pOldAstNode = pAstNode;

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return W_FAILURE;

    // Check whether the node has been replaced by a new one and if so mark the new one as visited as well
    if (pAstNode != pOldAstNode)
    {
      W_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");
    }

    for (WUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Insert(execOutput.m_pTargetNode))
        continue;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      bool bExecInputFound = false;
      for (auto& execInput : execOutput.m_pTargetNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode == pAstNode && execInput.m_uiSourcePinIndex == i)
        {
          bExecInputFound = true;
          break;
        }
      }
      W_ASSERT_DEBUG(bExecInputFound, "Execution connection corrupted");
#endif

      nodeStack.PushBack(execOutput.m_pTargetNode);
    }
  }

  return W_SUCCESS;
}

WResult WVisualScriptCompiler::TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, WDelegate<VisitorResult(const AstNode* pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return W_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  WTempHybridArray<const AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    // Find next node with all execution dependencies visited
    const AstNode* pAstNode = nullptr;
    for (WUInt32 i = nodeStack.GetCount(); i-- > 0;)
    {
      const AstNode* pCandidateAstNode = nodeStack[i];
      bool bAllVisited = true;
      for (auto& execInput : pCandidateAstNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode != nullptr && m_CompilationState.m_VisitedNodes.Contains(execInput.m_pSourceNode) == false)
        {
          bAllVisited = false;
          break;
        }
      }

      if (bAllVisited)
      {
        pAstNode = pCandidateAstNode;
        nodeStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (pAstNode == nullptr)
    {
      W_REPORT_FAILURE("Execution connection corrupted or loop detected");
      return W_FAILURE;
    }

    W_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return W_FAILURE;

    // Since we use a stack we need to iterate the execution outputs backwards to ensure that they are processed in order.
    // Strictly speaking this is not necessary but improves data locality especially for loops,
    // which in the end results in shorter variable lifetimes and thus in fewer local variables.
    for (WUInt32 i = pAstNode->m_ExecOutputs.GetCount(); i-- > 0;)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Contains(execOutput.m_pTargetNode))
        continue;

      if (nodeStack.Contains(execOutput.m_pTargetNode) == false)
      {
        nodeStack.PushBack(execOutput.m_pTargetNode);
      }
    }
  }

  return W_SUCCESS;
}

void WVisualScriptCompiler::DumpAST(AstNode* pEntryAstNode, WStringView sOutputPath, WStringView sFunctionName, WStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  WDGMLGraph dgmlGraph;
  {
    WHashTable<const AstNode*, WUInt32> nodeCache;
    WHashTable<WUInt64, WString> connectionCache;
    WStringBuilder sb;
    WUInt32 uiNodeIndex = 0;

    // First build all the dgml graph nodes
    TraverseAstTopologicalOrder(pEntryAstNode,
      [&](const AstNode* pAstNode)
      {
        WStringView sTypeName = WVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
        sb.SetFormat("{} (id: {})", sTypeName, uiNodeIndex);
        if (pAstNode->m_sTargetTypeName.IsEmpty() == false)
        {
          sb.Append("\n", pAstNode->m_sTargetTypeName);
        }
        if (pAstNode->m_DeductedDataType != WVisualScriptDataType::Invalid)
        {
          sb.Append("\nDataType: ", WVisualScriptDataType::GetName(pAstNode->m_DeductedDataType));
        }
        sb.AppendFormat("\nImplicitExec: {}", pAstNode->m_bImplicitExecution);
        if (pAstNode->m_Value.IsValid())
        {
          sb.AppendFormat("\nValue: {}", pAstNode->m_Value);
        }

        float colorX = WSimdRandom::FloatZeroToOne(WSimdVec4i(WHashingUtils::StringHash(sTypeName))).x();

        WDGMLGraph::NodeDesc nd;
        nd.m_Color = WColorScheme::LightUI(colorX);
        WUInt32 uiGraphNode = dgmlGraph.AddNode(sb, &nd);
        W_VERIFY(nodeCache.Insert(pAstNode, uiGraphNode) == false, "");

        ++uiNodeIndex;
        return VisitorResult::Continue;
      })
      .IgnoreResult();

    // Then build all the connections
    for (auto it : nodeCache)
    {
      const AstNode* pAstNode = it.Key();
      const WUInt32 uiGraphNode = it.Value();

      for (auto& execInput : pAstNode->m_ExecInputs)
      {
        WUInt32 uiSourceGraphNode = 0;
        W_VERIFY(nodeCache.TryGetValue(execInput.m_pSourceNode, uiSourceGraphNode), "");

        WUInt64 uiConnectionKey = uiSourceGraphNode | WUInt64(uiGraphNode) << 32;
        WString& sLabel = connectionCache[uiConnectionKey];

        WStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        sb.AppendFormat("Exec{}", execInput.m_uiSourcePinIndex);
        sLabel = sb;
      }

      for (WUInt32 i = 0; i < pAstNode->m_DataInputs.GetCount(); ++i)
      {
        auto& dataInput = pAstNode->m_DataInputs[i];
        if (dataInput.m_pSourceNode == nullptr)
          continue;

        // Exclude GetScriptOwner connections as they create too much noise
        if (dataInput.m_pSourceNode->m_Type == WVisualScriptNodeDescription::Type::GetScriptOwner)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);

        WUInt32 uiSourceGraphNode = 0;
        W_VERIFY(nodeCache.TryGetValue(dataInput.m_pSourceNode, uiSourceGraphNode), "");

        WUInt64 uiConnectionKey = uiSourceGraphNode | WUInt64(uiGraphNode) << 32;
        WString& sLabel = connectionCache[uiConnectionKey];

        WStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        const WUInt32 uiOutputId = dataOutput.m_DataOffset.m_uiByteOffset;
        const WUInt32 uiInputId = dataInput.m_DataOffset.m_uiByteOffset;
        sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, WVisualScriptDataType::GetName(dataOutput.m_DataOffset.GetType()), uiOutputId, i, WVisualScriptDataType::GetName(dataInput.m_DataOffset.GetType()), uiInputId);
        sLabel = sb;
      }
    }

    for (auto& it : connectionCache)
    {
      WUInt32 uiSource = it.Key() & 0xFFFFFFFF;
      WUInt32 uiTarget = it.Key() >> 32;

      dgmlGraph.AddConnection(uiSource, uiTarget, it.Value());
    }
  }

  WStringView sExt = sOutputPath.GetFileExtension();
  WStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  WDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    WLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    WLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}

void WVisualScriptCompiler::DumpGraph(WArrayPtr<const WVisualScriptNodeDescription> nodeDescriptions, WStringView sOutputPath, WStringView sFunctionName, WStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  WDGMLGraph dgmlGraph;
  {
    WStringBuilder sTmp;
    for (WUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      auto& nodeDesc = nodeDescriptions[i];

      WStringView sTypeName = WVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      sTmp.AppendFormat(" (id: {})", i);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), WVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_Module.m_ConstantDataToIndex)
          {
            auto scriptDataType = WVisualScriptDataType::FromVariantType(it.Key().GetType());
            if (scriptDataType == dataOffset.GetType() && it.Value() == dataOffset.m_uiByteOffset)
            {
              sTmp.AppendFormat(" ({})", it.Key());
              break;
            }
          }
        }
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        sTmp.AppendFormat("\n Output {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), WVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);
      }

      float colorX = WSimdRandom::FloatZeroToOne(WSimdVec4i(WHashingUtils::StringHash(sTypeName))).x();

      WDGMLGraph::NodeDesc nd;
      nd.m_Color = WColorScheme::LightUI(colorX);

      dgmlGraph.AddNode(sTmp, &nd);
    }

    for (WUInt32 uiCurrentIndex = 0; uiCurrentIndex < nodeDescriptions.GetCount(); ++uiCurrentIndex)
    {
      auto& executionIndices = nodeDescriptions[uiCurrentIndex].m_ExecutionIndices;
      for (WUInt32 uiExecIndex = 0; uiExecIndex < executionIndices.GetCount(); ++uiExecIndex)
      {
        const WUInt32 uiNextIndex = executionIndices[uiExecIndex];
        if (uiNextIndex == WSmallInvalidIndex)
          continue;

        sTmp.SetFormat("Exec{}", uiExecIndex);
        dgmlGraph.AddConnection(uiCurrentIndex, uiNextIndex, sTmp);
      }
    }
  }

  WStringView sExt = sOutputPath.GetFileExtension();
  WStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  WDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    WLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    WLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}
