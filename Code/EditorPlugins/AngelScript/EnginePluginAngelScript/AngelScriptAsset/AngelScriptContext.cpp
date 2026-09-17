#include <EnginePluginAngelScript/EnginePluginAngelScriptPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <EnginePluginAngelScript/AngelScriptAsset/AngelScriptContext.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptDocumentContext, 1, WRTTIDefaultAllocator<WAngelScriptDocumentContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "AngelScript"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WAtomicInteger32 s_iCompileCounter;

WAngelScriptDocumentContext::WAngelScriptDocumentContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

WAngelScriptDocumentContext::~WAngelScriptDocumentContext() = default;

WStatus WAngelScriptDocumentContext::ExportDocument(const WExportDocumentMsgToEngine* pMsg)
{
  WLogSystemToBuffer logBuffer;
  logBuffer.SetLogLevel(WLogMsgType::ErrorMsg);

  WLogSystemScope logScope(&logBuffer);

  WStringBuilder sCode;
  asIScriptModule* pModule = CompileModule(sCode, nullptr);

  if (pModule == nullptr)
  {
    return WStatus(logBuffer.m_sBuffer.GetView());
  }

  WTempHybridArray<WUInt8, 1024 * 8> bytecode;
  WAngelScriptUtils::SaveByteCode(pModule, bytecode);
  pModule->Discard();

  WDeferredFileWriter out;
  out.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    WAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(out).AssertSuccess();

    WUInt8 uiVersion = 4;
    out << uiVersion;
  }

  WUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  WCompressedStreamWriterZstd stream(&out, 0, WCompressedStreamWriterZstd::Compression::Average);
#else
  WStreamWriter& stream = out;
#endif

  // write uncompressed
  out << uiCompressionMode;

  // write the rest compressed
  stream << m_sClass;
  stream.WriteArray(bytecode).AssertSuccess();
  stream << sCode;

  return WStatus(W_SUCCESS);
}

void WAngelScriptDocumentContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (const WDocumentConfigMsgToEngine* pMsg2 = WDynamicCast<const WDocumentConfigMsgToEngine*>(pMsg))
  {
    if (pMsg2->m_sWhatToDo == "InputFile")
    {
      m_sInputFile = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "Code")
    {
      m_sCode = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "Class")
    {
      m_sClass = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "SyncExposedParams")
    {
      SyncExposedParameters();
    }
    else if (pMsg2->m_sWhatToDo == "RetrieveScriptInfos")
    {
      RetrieveScriptInfos(pMsg2->m_sValue);
    }
  }

  WEngineProcessDocumentContext::HandleMessage(pMsg);
}

WEngineProcessViewContext* WAngelScriptDocumentContext::CreateViewContext()
{
  W_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void WAngelScriptDocumentContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_ASSERT_NOT_IMPLEMENTED;
}

class WLogSystemNull : public WLogInterface
{
public:
  void HandleLogMessage(const WLoggingEventData& le) override
  {
  }
};

void WAngelScriptDocumentContext::SyncExposedParameters()
{
  WLogSystemNull logNull;
  WLogSystemScope scope(&logNull); // disable logging

  WStringBuilder sCode;
  WSet<WString> dependencies;
  asIScriptModule* pModule = CompileModule(sCode, &dependencies);
  if (pModule == nullptr)
    return;

  W_SCOPE_EXIT(pModule->Discard());

  const asITypeInfo* pClassType = pModule->GetTypeInfoByName(m_sClass);

  asIScriptContext* pContext = pModule->GetEngine()->CreateContext();
  W_SCOPE_EXIT(pContext->Release());

  AS_CHECK(pContext->Prepare(pClassType->GetFactoryByIndex(0)));
  AS_CHECK(pContext->Execute());
  asIScriptObject* pInstance = (asIScriptObject*)pContext->GetReturnObject();
  pInstance->AddRef();
  W_SCOPE_EXIT(pInstance->Release());

  WStringBuilder sTypeName;

  {
    WSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncExposedParams_Clear";
    SendProcessMessage(&msg);
  }

  for (WUInt32 idx = 0; idx < pClassType->GetPropertyCount(); ++idx)
  {
    const char* szName;
    int typeId;

    bool isPrivate = false, isProtected = false, isReference = false;
    pClassType->GetProperty(idx, &szName, &typeId, &isPrivate, &isProtected, nullptr, &isReference);

    if (isPrivate || isProtected)
      continue;

    WVariant defVal;
    if (WAngelScriptUtils::ReadFromAsTypeAtLocation(pModule->GetEngine(), typeId, pInstance->GetAddressOfProperty(idx), defVal).Succeeded())
    {
      WSimpleDocumentConfigMsgToEditor msg;
      msg.m_DocumentGuid = m_DocumentGuid;
      msg.m_sWhatToDo = "SyncExposedParams_Add";
      msg.m_sPayload = szName;
      msg.m_PayloadValue = defVal;
      SendProcessMessage(&msg);
    }
  }

  for (const auto& dep : dependencies)
  {
    WSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncDependencies_Add";
    msg.m_sPayload = dep;
    SendProcessMessage(&msg);
  }

  {
    WSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncExposedParams_Finish";
    SendProcessMessage(&msg);
  }
}

asIScriptModule* WAngelScriptDocumentContext::CompileModule(WStringBuilder& out_sCode, WSet<WString>* out_pDependencies)
{
  WStringBuilder sCode, sInputFile;

  if (m_sInputFile.StartsWith(":inline:"))
  {
    sCode = m_sCode;
    sInputFile = m_sInputFile;
    sInputFile.TrimWordStart(":inline:");
  }
  else
  {
    if (m_sInputFile.IsEmpty())
    {
      WLog::Error("No AngelScript file specified.");
      return nullptr;
    }

    WFileReader file;
    if (file.Open(m_sInputFile).Failed())
    {
      WLog::Error("Failed to open script file '{}'.", m_sInputFile);
      return nullptr;
    }

    sCode.ReadAll(file);

    sInputFile = m_sInputFile;
  }

  if (sCode.IsEmpty())
  {
    WLog::Error("Script code is empty.");
    return nullptr;
  }

  WStringBuilder sTempName;
  sTempName.SetFormat("asTempModule-{}", s_iCompileCounter.Increment());

  auto pAs = WAngelScriptEngineSingleton::GetSingleton();
  auto pModule = pAs->CompileModule(sTempName, m_sClass, sInputFile, sCode, &out_sCode, out_pDependencies);

  if (pModule == nullptr)
    return nullptr;

  if (pAs->ValidateModule(pModule).Failed())
  {
    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

static void WriteSet(WStringView sFile, const WSet<WString>& set)
{
  WFileWriter writer;
  if (writer.Open(sFile).Failed())
    return;

  const char* szLineBreak = "\n";

  for (const WString& sItem : set)
  {
    if (sItem.IsEmpty())
      continue;

    writer.WriteBytes(sItem.GetData(), sItem.GetElementCount()).AssertSuccess();
    writer.WriteBytes(szLineBreak, 1).AssertSuccess();
  }
}

void WAngelScriptDocumentContext::RetrieveScriptInfos(WStringView sBasePath)
{
  auto pEngine = WAngelScriptEngineSingleton::GetSingleton()->GetEngine();

  {
    WAsInfos infos;
    WAngelScriptUtils::RetrieveAsInfos(pEngine, infos);

    WStringBuilder sFullPath;

    sFullPath.SetPath(sBasePath, "Types.asgen");
    WriteSet(sFullPath, infos.m_Types);

    sFullPath.SetPath(sBasePath, "Namespaces.asgen");
    WriteSet(sFullPath, infos.m_Namespaces);

    sFullPath.SetPath(sBasePath, "GlobalFunctions.asgen");
    WriteSet(sFullPath, infos.m_GlobalFunctions);

    sFullPath.SetPath(sBasePath, "Methods.asgen");
    WriteSet(sFullPath, infos.m_Methods);

    sFullPath.SetPath(sBasePath, "Properties.asgen");
    WriteSet(sFullPath, infos.m_Properties);

    sFullPath.SetPath(sBasePath, "Enums.asgen");
    WriteSet(sFullPath, infos.m_EnumValues);

    sFullPath.SetPath(sBasePath, "AllDeclarations.asgen");
    WriteSet(sFullPath, infos.m_AllDeclarations);

    sFullPath.SetPath(sBasePath, "NotRegisteredDecls.asgen");
    WriteSet(sFullPath, WAngelScriptEngineSingleton::GetSingleton()->GetNotRegistered());
  }

  {
    WStringBuilder sPredef;
    WAngelScriptUtils::GenerateAsPredefinedFile(pEngine, sPredef);

    WStringBuilder sFullPath;
    sFullPath.SetPath(sBasePath, "../../as.predefined");
    WFileWriter file;
    if (file.Open(sFullPath).Succeeded())
    {
      file.WriteBytes(sPredef.GetData(), sPredef.GetElementCount()).AssertSuccess();
    }
  }
}
