#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  void DumpAST(const WExpressionAST& ast, WStringView sAssetName, WStringView sOutputName)
  {
    WDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    WStringBuilder sFileName;
    sFileName.SetFormat(":appdata/{0}_{1}_AST.dgml", sAssetName, sOutputName);

    WDGMLGraphWriter dgmlGraphWriter;
    W_IGNORE_UNUSED(dgmlGraphWriter);
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      WLog::Info("AST was dumped to: {0}", sFileName);
    }
    else
    {
      WLog::Error("Failed to dump AST to: {0}", sFileName);
    }
  }

  static const char* s_szSphereAssetId = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";     // Base/Prefabs/Sphere.WPrefab
  static const char* s_szBWGradientAssetId = "{ 3834b7d0-5a3f-140d-31d8-3a2bf48b09bd }"; // Base/Textures/BlackWhiteGradient.WColorGradientAsset

} // namespace

////////////////////////////////////////////////////////////////

struct DocObjAndOutput
{
  W_DECLARE_POD_TYPE();

  const WDocumentObject* m_pObject;
  const char* m_szOutputName;
};

template <>
struct WHashHelper<DocObjAndOutput>
{
  W_ALWAYS_INLINE static WUInt32 Hash(const DocObjAndOutput& value)
  {
    const WUInt32 hashA = WHashHelper<const void*>::Hash(value.m_pObject);
    const WUInt32 hashB = WHashHelper<const void*>::Hash(value.m_szOutputName);
    return WHashingUtils::CombineHashValues32(hashA, hashB);
  }

  W_ALWAYS_INLINE static bool Equal(const DocObjAndOutput& a, const DocObjAndOutput& b)
  {
    return a.m_pObject == b.m_pObject && a.m_szOutputName == b.m_szOutputName;
  }
};

struct WProcGenGraphAssetDocument::GenerateContext
{
  GenerateContext(const WDocumentObjectManager* pManager)
    : m_ObjectWriter(&m_AbstractObjectGraph, pManager)
    , m_RttiConverter(&m_AbstractObjectGraph, &m_RttiConverterContext)
  {
  }

  WAbstractObjectGraph m_AbstractObjectGraph;
  WDocumentObjectConverterWriter m_ObjectWriter;
  WRttiConverterContext m_RttiConverterContext;
  WRttiConverterReader m_RttiConverter;
  WHashTable<const WDocumentObject*, WUniquePtr<WProcGenNodeBase>> m_DocObjToProcGenNodeTable;
  WHashTable<DocObjAndOutput, WExpressionAST::Node*> m_DocObjAndOutputToASTNodeTable;
  WProcGenNodeBase::GraphContext m_GraphContext;
};

////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenGraphAssetProperties, 1, WRTTIDefaultAllocator<WProcGenGraphAssetProperties>)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("DebugPrefab", m_sDebugPrefab)->AddAttributes(new WDefaultValueAttribute(WStringView(s_szSphereAssetId)), new WAssetBrowserAttribute("CompatibleAsset_Prefab", WDependencyFlags::None)),
      W_MEMBER_PROPERTY("DebugFootprint", m_fDebugFootprint)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, WVariant())),
      W_MEMBER_PROPERTY("DebugAlignToNormal", m_fDebugAlignToNormal)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
      W_MEMBER_PROPERTY("DebugColorGradient", m_sDebugColorGradient)->AddAttributes(new WDefaultValueAttribute(WStringView(s_szBWGradientAssetId)), new WAssetBrowserAttribute("CompatibleAsset_Data_Gradient", WDependencyFlags::None)),
      W_ENUM_MEMBER_PROPERTY("DebugPlacementPattern", WProcPlacementPattern, m_DebugPlacementPattern)->AddAttributes(new WDefaultValueAttribute(WProcPlacementPattern::RegularGrid)),
      W_MEMBER_PROPERTY("DebugSurface", m_sDebugSurface)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Surface", WDependencyFlags::None)),
    }
    W_END_PROPERTIES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenGraphAssetDocument, 10, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WProcGenGraphAssetDocument::WProcGenGraphAssetDocument(WStringView sDocumentPath)
  : WAssetDocument(sDocumentPath, W_DEFAULT_NEW(WProcGenNodeManager), WAssetDocEngineConnection::None)
{
}

void WProcGenGraphAssetDocument::SetDebugPin(const WVisualGraphPin* pDebugPin)
{
  m_pDebugPin = pDebugPin;

  if (m_pDebugPin != nullptr)
  {
    UpdateDebugNode();
  }

  WDocumentObjectPropertyEvent e;
  e.m_EventType = WDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_sProperty = "DebugPin";

  GetObjectManager()->m_PropertyEvents.Broadcast(e);
}

void WProcGenGraphAssetDocument::UpdateDebugNode()
{
  m_pDebugNode.Clear();

  auto pPropertiesObject = GetObjectManager()->GetRootObject()->GetChildren()[0];
  if (pPropertiesObject->GetType() != WGetStaticRTTI<WProcGenGraphAssetProperties>())
    return;

  auto& typeAccessor = pPropertiesObject->GetTypeAccessor();

  m_pDebugNode = W_DEFAULT_NEW(WProcGen_PlacementOutput);
  m_pDebugNode->m_sName = "Debug";
  m_pDebugNode->m_ObjectsToPlace.PushBack(typeAccessor.GetValue("DebugPrefab").ConvertTo<WString>());
  m_pDebugNode->m_fFootprint = typeAccessor.GetValue("DebugFootprint").ConvertTo<float>();
  m_pDebugNode->m_fAlignToNormal = typeAccessor.GetValue("DebugAlignToNormal").ConvertTo<float>();
  m_pDebugNode->m_sColorGradient = typeAccessor.GetValue("DebugColorGradient").ConvertTo<WString>();
  m_pDebugNode->m_PlacementPattern = static_cast<WProcPlacementPattern::Enum>(typeAccessor.GetValue("DebugPlacementPattern").ConvertTo<WUInt32>());
  m_pDebugNode->m_sSurface = typeAccessor.GetValue("DebugSurface").ConvertTo<WString>();
}

WStatus WProcGenGraphAssetDocument::WriteAsset(WStreamWriter& inout_stream, const WPlatformProfile* pAssetProfile, bool bAllowDebug) const
{
  GenerateContext context(GetObjectManager());

  WDynamicArray<const WDocumentObject*> placementNodes;
  WDynamicArray<const WDocumentObject*> vertexColorNodes;
  GetAllOutputNodes(placementNodes, vertexColorNodes);

  const bool bDebug = bAllowDebug && (m_pDebugPin != nullptr) && (m_pDebugNode != nullptr);

  WStringDeduplicationWriteContext stringDedupContext(inout_stream);

  WChunkStreamWriter chunk(stringDedupContext.Begin());
  chunk.BeginStream(1);

  WExpressionCompiler compiler;

  auto WriteByteCode = [&](const WDocumentObject* pOutputNode) -> WStatus
  {
    context.m_GraphContext.m_VolumeTagSetIndices.Clear();

    if (pOutputNode->GetType()->IsDerivedFrom<WProcGen_PlacementOutput>())
    {
      context.m_GraphContext.m_OutputType = WProcGenNodeBase::GraphContext::Placement;
    }
    else if (pOutputNode->GetType()->IsDerivedFrom<WProcGen_VertexColorOutput>())
    {
      context.m_GraphContext.m_OutputType = WProcGenNodeBase::GraphContext::Color;
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
      return WStatus("Unknown output type");
    }

    WExpressionAST ast;
    GenerateExpressionAST(pOutputNode, "", context, ast);
    context.m_DocObjAndOutputToASTNodeTable.Clear();

    if (false)
    {
      WStringBuilder sDocumentPath = GetDocumentPath();
      WStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
      WStringView sOutputName = pOutputNode->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();

      DumpAST(ast, sAssetName, sOutputName);
    }

    WExpressionByteCode byteCode;
    if (compiler.Compile(ast, byteCode).Failed())
    {
      return WStatus("Compilation failed");
    }

    W_SUCCEED_OR_RETURN(byteCode.Save(chunk));

    return WStatus(W_SUCCESS);
  };

  {
    chunk.BeginChunk("PlacementOutputs", 9);

    if (!bDebug)
    {
      chunk << placementNodes.GetCount();

      for (auto pPlacementNode : placementNodes)
      {
        W_SUCCEED_OR_RETURN(WriteByteCode(pPlacementNode));

        auto pPGNode = context.m_DocObjToProcGenNodeTable.GetValue(pPlacementNode);
        auto pPlacementOutput = WStaticCast<WProcGen_PlacementOutput*>(pPGNode->Borrow());

        pPlacementOutput->CopyValuesFromContext(context.m_GraphContext);
        pPlacementOutput->Save(chunk);
      }
    }
    else
    {
      WUInt32 uiNumNodes = 1;
      chunk << uiNumNodes;

      context.m_GraphContext.m_VolumeTagSetIndices.Clear();
      context.m_GraphContext.m_CurveIndices.Clear();
      context.m_GraphContext.m_OutputType = WProcGenNodeBase::GraphContext::Placement;

      WExpressionAST ast;
      GenerateDebugExpressionAST(context, ast);
      context.m_DocObjAndOutputToASTNodeTable.Clear();

      WExpressionByteCode byteCode;
      if (compiler.Compile(ast, byteCode).Failed())
      {
        return WStatus("Debug Compilation failed");
      }

      W_SUCCEED_OR_RETURN(byteCode.Save(chunk));

      m_pDebugNode->CopyValuesFromContext(context.m_GraphContext);
      m_pDebugNode->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexColorOutputs", 3);

    chunk << vertexColorNodes.GetCount();

    for (auto pVertexColorNode : vertexColorNodes)
    {
      W_SUCCEED_OR_RETURN(WriteByteCode(pVertexColorNode));

      auto pPGNode = context.m_DocObjToProcGenNodeTable.GetValue(pVertexColorNode);
      auto pVertexColorOutput = WStaticCast<WProcGen_VertexColorOutput*>(pPGNode->Borrow());

      pVertexColorOutput->CopyValuesFromContext(context.m_GraphContext);
      pVertexColorOutput->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SharedData", 1);

    context.m_GraphContext.m_SharedData.Save(chunk);

    chunk.EndChunk();
  }

  chunk.EndStream();
  W_SUCCEED_OR_RETURN(stringDedupContext.End());

  return WStatus(W_SUCCESS);
}

void WProcGenGraphAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  auto pRoot = this->GetObjectManager()->GetRootObject();
  if (pRoot->GetChildren().IsEmpty() == false && pRoot->GetChildren()[0]->GetType() != WGetStaticRTTI<WProcGenGraphAssetProperties>())
  {
    WDocumentObject* pObject = this->GetObjectManager()->CreateObject(WGetStaticRTTI<WProcGenGraphAssetProperties>());
    this->GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);
  }

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void WProcGenGraphAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (m_pDebugPin == nullptr)
  {
    WDynamicArray<const WDocumentObject*> placementNodes;
    WDynamicArray<const WDocumentObject*> vertexColorNodes;
    GetAllOutputNodes(placementNodes, vertexColorNodes);

    for (auto pPlacementNode : placementNodes)
    {
      auto& typeAccessor = pPlacementNode->GetTypeAccessor();

      WUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
      for (WUInt32 i = 0; i < uiNumObjects; ++i)
      {
        WVariant prefab = typeAccessor.GetValue("Objects", i);
        if (prefab.IsA<WString>())
        {
          pInfo->m_PackageDependencies.Insert(prefab.Get<WString>());
          pInfo->m_ThumbnailDependencies.Insert(prefab.Get<WString>());
        }
      }

      WVariant colorGradient = typeAccessor.GetValue("ColorGradient");
      if (colorGradient.IsA<WString>())
      {
        pInfo->m_PackageDependencies.Insert(colorGradient.Get<WString>());
        pInfo->m_ThumbnailDependencies.Insert(colorGradient.Get<WString>());
      }
    }
  }
  else
  {
    pInfo->m_PackageDependencies.Insert(s_szSphereAssetId);
    pInfo->m_PackageDependencies.Insert(s_szBWGradientAssetId);

    pInfo->m_ThumbnailDependencies.Insert(s_szSphereAssetId);
    pInfo->m_ThumbnailDependencies.Insert(s_szBWGradientAssetId);
  }
}

WTransformStatus WProcGenGraphAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteAsset(stream, pAssetProfile, false);
}

void WProcGenGraphAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.ProcGenGraph");
}

bool WProcGenGraphAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const
{
  out_MimeType = "application/WEditor.ProcGenGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool WProcGenGraphAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void WProcGenGraphAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void WProcGenGraphAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void WProcGenGraphAssetDocument::GetAllOutputNodes(WDynamicArray<const WDocumentObject*>& placementNodes, WDynamicArray<const WDocumentObject*>& vertexColorNodes) const
{
  const WRTTI* pPlacementOutputRtti = WGetStaticRTTI<WProcGen_PlacementOutput>();
  const WRTTI* pVertexColorOutputRtti = WGetStaticRTTI<WProcGen_VertexColorOutput>();

  placementNodes.Clear();
  vertexColorNodes.Clear();

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const WDocumentObject* pObject : children)
  {
    if (pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>())
    {
      const WRTTI* pRtti = pObject->GetTypeAccessor().GetType();
      if (pRtti->IsDerivedFrom(pPlacementOutputRtti))
      {
        placementNodes.PushBack(pObject);
      }
      else if (pRtti->IsDerivedFrom(pVertexColorOutputRtti))
      {
        vertexColorNodes.PushBack(pObject);
      }
    }
  }
}

void WProcGenGraphAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

WExpressionAST::Node* WProcGenGraphAssetDocument::GenerateExpressionAST(const WDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, WExpressionAST& out_Ast) const
{
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());

  auto inputPins = pManager->GetInputPins(outputNode);

  WTempHybridArray<WExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(inputPins.GetCount());

  for (WUInt32 i = 0; i < inputPins.GetCount(); ++i)
  {
    auto connections = pManager->GetConnections(*inputPins[i]);
    W_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const WVisualGraphPin& pinSource = connections[0]->GetSourcePin();

    DocObjAndOutput key = {pinSource.GetParent(), pinSource.GetName()};
    WExpressionAST::Node* astNode;
    if (!context.m_DocObjAndOutputToASTNodeTable.TryGetValue(key, astNode))
    {
      // recursively generate all dependent code
      astNode = GenerateExpressionAST(pinSource.GetParent(), pinSource.GetName(), context, out_Ast);

      context.m_DocObjAndOutputToASTNodeTable.Insert(key, astNode);
    }

    inputAstNodes[i] = astNode;
  }

  WProcGenNodeBase* cachedPGNode = nullptr;
  if (auto pCachedPGNode = context.m_DocObjToProcGenNodeTable.GetValue(outputNode))
  {
    cachedPGNode = pCachedPGNode->Borrow();
  }
  else
  {
    WAbstractObjectNode* pAbstractNode = context.m_ObjectWriter.AddObjectToGraph(outputNode);
    auto newPGNode = context.m_RttiConverter.CreateObjectFromNode(pAbstractNode).Cast<WProcGenNodeBase>();
    cachedPGNode = newPGNode;

    context.m_DocObjToProcGenNodeTable.Insert(outputNode, newPGNode);
  }

  return cachedPGNode->GenerateExpressionASTNode(WTempHashedString(szOutputName), inputAstNodes, out_Ast, context.m_GraphContext);
}

WExpressionAST::Node* WProcGenGraphAssetDocument::GenerateDebugExpressionAST(GenerateContext& context, WExpressionAST& out_Ast) const
{
  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  W_ASSERT_DEV(m_pDebugPin != nullptr, "");

  const WVisualGraphPin* pPinSource = m_pDebugPin;
  if (pPinSource->GetType() == WVisualGraphPin::Type::Input)
  {
    auto connections = pManager->GetConnections(*pPinSource);
    W_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      return nullptr;

    pPinSource = &connections[0]->GetSourcePin();
    W_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");
  }

  WTempHybridArray<WExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(4); // placement output node has 4 inputs

  // Recursively generate all dependent code and pretend it is connected to the color index input of the debug placement output node.
  inputAstNodes[2] = GenerateExpressionAST(pPinSource->GetParent(), pPinSource->GetName(), context, out_Ast);

  return m_pDebugNode->GenerateExpressionASTNode("", inputAstNodes, out_Ast, context.m_GraphContext);
}

void WProcGenGraphAssetDocument::DumpSelectedOutput(bool bAst, bool bDisassembly) const
{
  const WDocumentObject* pSelectedNode = nullptr;

  const auto& selection = GetSelectionManager()->GetSelection();
  if (!selection.IsEmpty())
  {
    pSelectedNode = selection[0];
    if (!pSelectedNode->GetType()->IsDerivedFrom<WProcGenOutput>())
    {
      pSelectedNode = nullptr;
    }
  }

  if (pSelectedNode == nullptr)
  {
    WLog::Error("No valid output node selected.");
    return;
  }

  GenerateContext context(GetObjectManager());
  if (pSelectedNode->GetType()->IsDerivedFrom<WProcGen_PlacementOutput>())
  {
    context.m_GraphContext.m_OutputType = WProcGenNodeBase::GraphContext::Placement;
  }
  else if (pSelectedNode->GetType()->IsDerivedFrom<WProcGen_VertexColorOutput>())
  {
    context.m_GraphContext.m_OutputType = WProcGenNodeBase::GraphContext::Color;
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return;
  }

  WExpressionAST ast;
  GenerateExpressionAST(pSelectedNode, "", context, ast);

  WStringBuilder sDocumentPath = GetDocumentPath();
  WStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
  WStringView sOutputName = pSelectedNode->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();

  if (bAst)
  {
    DumpAST(ast, sAssetName, sOutputName);
  }

  WExpressionByteCode byteCode;
  WExpressionCompiler compiler;
  if (compiler.Compile(ast, byteCode).Failed())
  {
    WLog::Error("Compiling expression failed");
    return;
  }

  if (bAst)
  {
    WStringBuilder sOutputName2 = sOutputName;
    sOutputName2.Append("_Opt");

    DumpAST(ast, sAssetName, sOutputName2);
  }

  if (bDisassembly)
  {
    WStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    WStringBuilder sFileName;
    sFileName.SetFormat(":appdata/{0}_{1}_ByteCode.txt", sAssetName, sOutputName);

    WFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      WLog::Info("Disassembly was dumped to: {0}", sFileName);
    }
    else
    {
      WLog::Error("Failed to dump Disassembly to: {0}", sFileName);
    }
  }
}
