#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WAngelScriptCodeMode, 1)
  W_ENUM_CONSTANTS(WAngelScriptCodeMode::Inline, WAngelScriptCodeMode::FromFile)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptParameter, 1, WRTTIDefaultAllocator<WAngelScriptParameter>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Declaration", m_sDeclaration)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptAssetProperties, 1, WRTTIDefaultAllocator<WAngelScriptAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Source", WAngelScriptCodeMode, m_CodeMode),
    W_MEMBER_PROPERTY("SourceFile", m_sScriptFile)->AddAttributes(new WFileBrowserAttribute("Select Script", "*.as", {}, "AngelScript")),
    W_MEMBER_PROPERTY("ClassName", m_sClassName)->AddAttributes(new WDefaultValueAttribute("ScriptObject")),
    W_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters)->AddAttributes(new WContainerAttribute(false, false, false)),
    W_ARRAY_MEMBER_PROPERTY("Dependencies", m_Dependencies)->AddAttributes(new WContainerAttribute(false, false, false), new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Code", m_sCode)->AddAttributes(new WHiddenAttribute(), new WDefaultValueAttribute("class ScriptObject : WAngelScriptClass\n\
{\n\t// int PublicIntVar = 0;\n\n\tvoid OnSimulationStarted()\n\t{\n\t\t// WLog::Info(\"Simulation Started\");\n\t}\n\n\t// void Update() { }\n\n\t// void OnMsgTriggerTriggered(WMsgTriggerTriggered@ msg) { }\n}")),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptAssetDocument, 3, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAngelScriptAssetDocument::WAngelScriptAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WAngelScriptAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple)
{
}

WTransformStatus WAngelScriptAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return WTransformStatus("");
}

class WAsPreprocessor2
{
public:
  WStringBuilder m_sRefFilePath;
  WStringBuilder m_sMainCode;
  WSet<WString> m_Dependencies;

  WAsPreprocessor2()
  {
    m_Processor.SetFileOpenFunction(WMakeDelegate(&WAsPreprocessor2::PreProc_OpenFile, this));
    m_Processor.SetImplicitPragmaOnce(true);
  }

  WResult Process()
  {
    WStringBuilder sResult;
    return m_Processor.Process(m_sRefFilePath, sResult, false);
  };

private:
  WResult PreProc_OpenFile(WStringView sAbsFile, WDynamicArray<WUInt8>& out_Content, WTimestamp& out_FileModification)
  {
    if (sAbsFile == m_sRefFilePath)
    {
      out_Content.SetCount(m_sMainCode.GetElementCount());
      WMemoryUtils::RawByteCopy(out_Content.GetData(), m_sMainCode.GetData(), m_sMainCode.GetElementCount());
      return W_SUCCESS;
    }

    WFileReader file;
    if (file.Open(sAbsFile).Failed())
      return W_FAILURE;

    m_Dependencies.Insert(sAbsFile);

    out_Content.SetCountUninitialized((WUInt32)file.GetFileSize());
    file.ReadBytes(out_Content.GetData(), out_Content.GetCount());
    return W_SUCCESS;
  }

  WPreprocessor m_Processor;
};

WTransformStatus WAngelScriptAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  SyncInfos();

  const bool bCanModify = !transformFlags.IsSet(WTransformFlags::BackgroundProcessing);

  auto pProps = GetProperties();

  WAsPreprocessor2 preProc;

  if (pProps->m_CodeMode == WAngelScriptCodeMode::Inline)
  {
    preProc.m_sRefFilePath = GetDocumentPath();
    preProc.m_sMainCode = pProps->m_sCode;

    WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(preProc.m_sRefFilePath);
  }
  else
  {
    preProc.m_sRefFilePath = pProps->m_sScriptFile;

    WFileReader file;
    if (file.Open(pProps->m_sScriptFile).Succeeded())
    {
      preProc.m_sMainCode.ReadAll(file);
    }
  }

  if (preProc.Process().Succeeded())
  {
    WTempHybridArray<WString, 16> newDeps;
    for (const auto& str : preProc.m_Dependencies)
    {
      newDeps.PushBack(str);
    }

    if (pProps->m_Dependencies != newDeps)
    {
      if (!bCanModify)
        return WTransformResult::NeedsImport;

      auto pPropObj = GetPropertyObject();

      WCommandHistory* history = GetCommandHistory();
      WObjectCommandAccessor accessor(history);

      accessor.StartTransaction("Update Dependencies");

      // clear the entire array
      accessor.ClearByName(pPropObj, "Dependencies").AssertSuccess();

      const WAbstractProperty* pPropDeps = pPropObj->GetType()->FindPropertyByName("Dependencies");

      // and fill it again
      for (WUInt32 clip = 0; clip < newDeps.GetCount(); ++clip)
      {
        accessor.InsertValue(pPropObj, pPropDeps, newDeps[clip], -1).AssertSuccess();
      }

      accessor.FinishTransaction();
    }
  }

  return WAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

void WAngelScriptAssetDocument::SyncInfos()
{
  auto pProps = GetProperties();

  if (pProps->m_CodeMode == WAngelScriptCodeMode::FromFile)
  {
    WDocumentConfigMsgToEngine cfg;
    cfg.m_sWhatToDo = "InputFile";
    cfg.m_sValue = pProps->m_sScriptFile;
    GetEditorEngineConnection()->SendMessage(&cfg);
  }
  else
  {
    {
      WStringBuilder sStartFile = GetDocumentPath();
      WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sStartFile);
      sStartFile.Prepend(":inline:");

      WDocumentConfigMsgToEngine cfg;
      cfg.m_sWhatToDo = "InputFile";
      cfg.m_sValue = sStartFile;
      GetEditorEngineConnection()->SendMessage(&cfg);
    }

    {
      WDocumentConfigMsgToEngine cfg;
      cfg.m_sWhatToDo = "Code";
      cfg.m_sValue = pProps->m_sCode;
      GetEditorEngineConnection()->SendMessage(&cfg);
    }
  }

  {
    WDocumentConfigMsgToEngine cfg;
    cfg.m_sWhatToDo = "Class";
    cfg.m_sValue = pProps->m_sClassName;
    GetEditorEngineConnection()->SendMessage(&cfg);
  }
}

void WAngelScriptAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  WExposedParameters* pExposedParams = W_DEFAULT_NEW(WExposedParameters);

  for (const auto& p : GetProperties()->m_Parameters)
  {
    if (p.m_bExpose == false || p.m_sName.IsEmpty() || !p.m_DefaultValue.IsValid())
      continue;

    WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
    param->m_sName = p.m_sName;
    param->m_DefaultValue = p.m_DefaultValue;

    // TODO AngelScript: support resource handles and pass through the necessary attributes
    //
    // if (p.m_DefaultValue.IsA<WString>())
    //{
    //   param->m_Attributes.PushBack(new WAssetBrowserAttribute("CompatibleAsset_Material"));
    // }

    pExposedParams->m_Parameters.PushBack(param);
  }

  for (const auto& p : GetProperties()->m_Dependencies)
  {
    pInfo->m_TransformDependencies.Insert(p);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void WAngelScriptAssetDocument::OpenExternalEditor()
{
  WStringBuilder sScriptFile(GetProperties()->m_sScriptFile);

  if (GetProperties()->m_CodeMode != WAngelScriptCodeMode::FromFile)
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("The source for this asset is 'inline'. To be able to view it in an external program, it has to be moved into a dedicated file.");

    ShowDocumentStatus("Can't open script file, source code is 'inline'.");
    return;
  }

  if (!WFileSystem::ExistsFile(sScriptFile))
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation(WFmt("Can't find the file '{}'.\nTo create a script file click the button next to 'SourceFile'.", sScriptFile));

    ShowDocumentStatus("Script file doesn't exist.");
    return;
  }

  WStringBuilder sScriptFileAbs;
  if (WFileSystem::ResolvePath(sScriptFile, &sScriptFileAbs, nullptr).Failed())
    return;

  {
    QStringList args;

    args.append(WMakeQString(WToolsProject::GetSingleton()->GetProjectDirectory()));
    args.append(sScriptFileAbs.GetData());

    if (WQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      WQtUiServices::OpenFileInDefaultProgram(sScriptFileAbs).IgnoreResult();
    }
  }
}

void WAngelScriptAssetDocument::SyncExposedParameters()
{
  SyncInfos();

  WDocumentConfigMsgToEngine cfg;
  cfg.m_sWhatToDo = "SyncExposedParams";
  GetEditorEngineConnection()->SendMessage(&cfg);
}

void WAngelScriptAssetDocument::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WAngelScriptAssetProperties>())
  {
    const WInt64 sourceMode = e.m_pObject->GetTypeAccessor().GetValue("Source").ConvertTo<WInt64>();

    auto& props = *e.m_pPropertyStates;

    props["SourceFile"].m_Visibility = (sourceMode == WAngelScriptCodeMode::Inline) ? WPropertyUiState::Invisible : WPropertyUiState::Default;
  }
}
