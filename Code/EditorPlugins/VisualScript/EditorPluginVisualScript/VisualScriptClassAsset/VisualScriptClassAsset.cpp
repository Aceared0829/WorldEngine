#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptClassAssetProperties, 1, WRTTIDefaultAllocator<WVisualScriptClassAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("BaseClass", m_sBaseClass)->AddAttributes(new WDefaultValueAttribute(WStringView("Component")), new WDynamicStringEnumAttribute("ScriptBaseClasses")),
    W_ARRAY_MEMBER_PROPERTY("Variables", m_Variables),
    W_MEMBER_PROPERTY("DumpAST", m_bDumpAST),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptClassAssetDocument, 12, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WVisualScriptClassAssetDocument::WVisualScriptClassAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WVisualScriptClassAssetProperties>(W_DEFAULT_NEW(WVisualScriptNodeManager), sDocumentPath, WAssetDocEngineConnection::None)
{
  m_pObjectAccessor = W_DEFAULT_NEW(WVisualGraphCommandAccessor, GetCommandHistory());
}

WTransformStatus WVisualScriptClassAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  auto pManager = static_cast<WVisualScriptNodeManager*>(GetObjectManager());

  const auto& children = pManager->GetRootObject()->GetChildren();
  WHashedString sBaseClass = pManager->GetScriptBaseClass();

  WStringBuilder sBaseClassName = sBaseClass.GetView();
  if (WRTTI::FindTypeByName(sBaseClassName) == nullptr)
  {
    sBaseClassName.Prepend("W");
    if (WRTTI::FindTypeByName(sBaseClassName) == nullptr)
    {
      return WStatus(WFmt("Invalid base class '{}'", sBaseClassName));
    }
  }

  WStringView sScriptClassName = WPathUtils::GetFileName(GetDocumentPath());

  WVisualScriptCompiler compiler(*pManager);
  compiler.InitModule(sBaseClassName, sScriptClassName);

  WTempHybridArray<const WVisualScriptPin*, 16> pins;
  for (const WDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = WVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return WStatus(W_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    if (WVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      WStringView sFunctionName = WVisualScriptNodeManager::GetNiceFunctionName(pObject);
      W_SUCCEED_OR_RETURN(compiler.AddFunction(sFunctionName, pObject));
    }
  }

  WStringBuilder sDumpPath;
  if (GetProperties()->m_bDumpAST)
  {
    sDumpPath.SetFormat(":appdata/{}_AST.dgml", sScriptClassName);
  }
  W_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  W_SUCCEED_OR_RETURN(compiledModule.Serialize(stream));

  return WStatus(W_SUCCESS);
}

void WVisualScriptClassAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  WExposedParameters* pExposedParams = W_DEFAULT_NEW(WExposedParameters);

  for (const auto& v : GetProperties()->m_Variables)
  {
    if (v.m_TypeDecl.m_Type == WVisualScriptVariableType::Invalid || v.m_TypeDecl.m_bPublic == false)
      continue;

    if (v.m_TypeDecl.m_Type == WVisualScriptVariableType::GameObject || v.m_TypeDecl.m_Type == WVisualScriptVariableType::Component)
    {
      WLog::Error("Variables of type 'GameObject' or 'Component' are currently not supported as exposed parameters.");
      continue;
    }

    WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
    param->m_sName = v.m_sName.GetString();
    param->m_sType = WVisualScriptDataType::GetRtti(static_cast<WVisualScriptDataType::Enum>(v.m_TypeDecl.m_Type.GetValue()))->GetTypeName();
    param->m_DefaultValue = v.m_DefaultValue;
    param->m_Category = WVisualScriptVariableCategory::GetPropertyCategory(v.m_TypeDecl.m_Category);

    if (v.m_bClampRange && v.m_TypeDecl.m_Type >= WVisualScriptVariableType::Byte && v.m_TypeDecl.m_Type <= WVisualScriptVariableType::Double)
    {
      auto pClampValueAttribute = W_DEFAULT_NEW(WClampValueAttribute, v.m_fMinValue, v.m_fMaxValue);
      param->m_Attributes.PushBack(pClampValueAttribute);
    }

    pExposedParams->m_Parameters.PushBack(param);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void WVisualScriptClassAssetDocument::InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void WVisualScriptClassAssetDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void WVisualScriptClassAssetDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void WVisualScriptClassAssetDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.VisualScriptClassGraph");
}

bool WVisualScriptClassAssetDocument::CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_MimeType) const
{
  out_MimeType = "application/WEditor.VisualScriptClassGraph";

  const WVisualGraphObjectManager* pManager = static_cast<const WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool WVisualScriptClassAssetDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  WVisualGraphObjectManager* pManager = static_cast<WVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, WQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
