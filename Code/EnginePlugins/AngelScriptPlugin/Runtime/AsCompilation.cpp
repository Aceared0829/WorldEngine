#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>

class WAsPreprocessor
{
public:
  WStringView m_sRefFilePath;
  WStringView m_sMainCode;
  WSet<WString>* m_pDependencies = nullptr;

  WAsPreprocessor()
  {
    m_Processor.SetFileOpenFunction(WMakeDelegate(&WAsPreprocessor::PreProc_OpenFile, this));
    m_Processor.m_ProcessingEvents.AddEventHandler(WMakeDelegate(&WAsPreprocessor::PreProc_Event, this));
    m_Processor.SetImplicitPragmaOnce(true);
    m_Processor.SetPassThroughLine(true);
  }

  WResult Process(WStringBuilder& ref_sResult)
  {
    const bool bNeedsLineStmts = !m_sMainCode.StartsWith("//#ln");
    const bool bKeepComments = !bNeedsLineStmts;

    auto res = m_Processor.Process(m_sRefFilePath, ref_sResult, bKeepComments, false, bNeedsLineStmts);
    ref_sResult.ReplaceAll("#line", "//#ln");
    return res;
  };

private:
  WResult PreProc_OpenFile(WStringView sAbsFile, WDynamicArray<WUInt8>& out_Content, WTimestamp& out_FileModification)
  {
    if (sAbsFile == m_sRefFilePath)
    {
      out_Content.SetCount(m_sMainCode.GetElementCount());
      WMemoryUtils::RawByteCopy(out_Content.GetData(), m_sMainCode.GetStartPointer(), m_sMainCode.GetElementCount());
      return W_SUCCESS;
    }

    WFileReader file;
    if (file.Open(sAbsFile).Failed())
      return W_FAILURE;

    if (m_pDependencies)
    {
      m_pDependencies->Insert(sAbsFile);
    }

    out_Content.SetCountUninitialized((WUInt32)file.GetFileSize());
    file.ReadBytes(out_Content.GetData(), out_Content.GetCount());
    return W_SUCCESS;
  }

  void PreProc_Event(const WPreprocessor::ProcessingEvent& event)
  {
    switch (event.m_Type)
    {
      case WPreprocessor::ProcessingEvent::Error:
        WLog::Error("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      case WPreprocessor::ProcessingEvent::Warning:
        WLog::Warning("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      default:
        break;
    }
  }

  WPreprocessor m_Processor;
};

void WAngelScriptEngineSingleton::FindCorrectSectionAndLine(const WDynamicArray<WStringView>& lines, WInt32& ref_iLine, WStringView& ref_sSection)
{
  if (ref_iLine - 1 < (WInt32)lines.GetCount())
  {
    --ref_iLine;

    int iStepsBack = 0;

    while (ref_iLine >= 0)
    {
      if (lines[ref_iLine].StartsWith("//#ln"))
      {
        WStringView line = lines[ref_iLine];
        line.TrimWordStart("//#ln ");

        const char* szParsePos;
        WConversionUtils::StringToInt(line, ref_iLine, &szParsePos).AssertSuccess();

        line.SetStartPosition(szParsePos + 1);
        line.Trim("\"");

        ref_sSection = line;
        ref_iLine += iStepsBack - 1;
        break;
      }

      --ref_iLine;
      ++iStepsBack;
    }
  }
}
void WAngelScriptEngineSingleton::CompilerMessageCallback(const asSMessageInfo* msg)
{
  WDynamicArray<WStringView> lines;
  m_sCodeInCompilation.Split(true, lines, "\n");

  WInt32 iLine = msg->row;
  WStringView sSection = msg->section;

  FindCorrectSectionAndLine(lines, iLine, sSection);

  switch (msg->type)
  {
    case asMSGTYPE_ERROR:
      WLog::Error("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
    case asMSGTYPE_WARNING:
      WLog::Warning("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
    case asMSGTYPE_INFORMATION:
      WLog::Info("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
  }
}

asIScriptModule* WAngelScriptEngineSingleton::SetModuleCode(WStringView sModuleName, WStringView sCode, bool bAddExternalSection)
{
  W_LOCK(m_CompilerMutex);

  m_sCodeInCompilation = sCode;

  WStringBuilder tmp;
  asIScriptModule* pModule = m_pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (bAddExternalSection)
  {
    const char* szExternal = R"(
external shared class WAngelScriptClass;
)";

    pModule->AddScriptSection("External", szExternal);
  }

  pModule->AddScriptSection("Main", sCode.GetStartPointer(), sCode.GetElementCount());

  const int res = pModule->Build();
  switch (res)
  {
    case asBUILD_IN_PROGRESS:
      WLog::Error("AS: Another compilation is in progress.");
      break;

    case asINVALID_CONFIGURATION:
      WLog::Error("AS: Invalid Configuration.");
      break;

    case asINIT_GLOBAL_VARS_FAILED:
      WLog::Error("AS: Global Variable initialization failed.");
      break;

    case asNOT_SUPPORTED:
      WLog::Error("AS: Compiler support is disabled in the engine.");
      break;

    case asMODULE_IS_IN_USE:
      WLog::Error("AS: Module is in use.");
      break;

    case asERROR:
      break;
  }

  if (res < 0)
  {
    // TODO AngelScript: Forward compiler errors
    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

WResult WAngelScriptEngineSingleton::PreprocessCode(WStringView sRefFilePath, WStringView sCode, WStringBuilder* out_pProcessedCode, WSet<WString>* out_pDependencies)
{
  WAsPreprocessor asPP;
  asPP.m_sRefFilePath = sRefFilePath;
  asPP.m_sMainCode = sCode;
  asPP.m_pDependencies = out_pDependencies;

  WStringBuilder fullCode;
  if (asPP.Process(fullCode).Failed())
    return W_FAILURE;

  if (out_pProcessedCode)
  {
    *out_pProcessedCode = fullCode;
  }
  return W_SUCCESS;
}

asIScriptModule* WAngelScriptEngineSingleton::CompileModule(WStringView sModuleName, WStringView sMainClass, WStringView sRefFilePath, WStringView sCode, WStringBuilder* out_pProcessedCode, WSet<WString>* out_pDependencies)
{
  WStringBuilder fullCode;
  if (PreprocessCode(sRefFilePath, sCode, &fullCode, out_pDependencies).Failed())
  {
    WLog::Error("Failed to pre-process AngelScript");
    return nullptr;
  }

  if (out_pProcessedCode)
  {
    *out_pProcessedCode = fullCode;
  }

  asIScriptModule* pModule = SetModuleCode(sModuleName, fullCode, true);

  if (pModule == nullptr)
    return nullptr;

  WStringBuilder tmp;
  const asITypeInfo* pClassType = pModule->GetTypeInfoByName(sMainClass.GetData(tmp));

  if (pClassType == nullptr)
  {
    WLog::Error("AngelScript code doesn't contain class '{}'", sMainClass);
    return nullptr;
  }

  if (ValidateModule(pModule).Failed())
  {
    return nullptr;
  }

  return pModule;
}


WResult WAngelScriptEngineSingleton::ValidateModule(asIScriptModule* pModule) const
{
  WResult res = W_SUCCESS;

  for (WUInt32 i = 0; i < pModule->GetGlobalVarCount(); ++i)
  {
    const char* szName;
    int typeId;

    if (pModule->GetGlobalVar(i, &szName, nullptr, &typeId) == asSUCCESS)
    {
      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          WLog::Error("Global variable '{}' uses forbidden type '{}'", szName, pInfo->GetName());
          res = W_FAILURE;
        }
      }
    }
  }

  for (WUInt32 i = 0; i < pModule->GetObjectTypeCount(); ++i)
  {
    const asITypeInfo* pType = pModule->GetObjectTypeByIndex(i);

    for (WUInt32 i2 = 0; i2 < pType->GetPropertyCount(); ++i2)
    {
      const char* szName;
      int typeId;

      pType->GetProperty(i2, &szName, &typeId);

      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          WLog::Error("Property '{}::{}' uses forbidden type '{}'", pType->GetName(), szName, pInfo->GetName());
          res = W_FAILURE;
        }
      }
    }
  }

  return res;
}
