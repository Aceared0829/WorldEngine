#include <GameEngineTest/GameEngineTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_ANGELSCRIPT_SUPPORT

#  include <AngelScript/include/angelscript.h>
#  include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#  include <Core/Messages/CommonMessages.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/CodeUtils/Preprocessor.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

#  include "AngelScriptTest.h"

static WGameEngineTestAngelScript s_GameEngineTestAngelScript;

const char* WGameEngineTestAngelScript::GetTestName() const
{
  return "AngelScript Tests";
}

WGameEngineTestApplication* WGameEngineTestAngelScript::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication_AngelScript);
  return m_pOwnApplication;
}

void WGameEngineTestAngelScript::SetupSubTests()
{
  AddSubTest("Types", SubTests::Types);
  AddSubTest("Strings", SubTests::Strings);
  AddSubTest("Arrays", SubTests::Arrays);
  AddSubTest("EntryPoints", SubTests::EntryPoints);
  AddSubTest("World", SubTests::World);
  AddSubTest("Messaging", SubTests::Messaging);
  AddSubTest("EventMessaging", SubTests::EventMessaging);
  AddSubTest("GameObject", SubTests::GameObject);
  AddSubTest("Physics", SubTests::Physics);
  AddSubTest("Misc", SubTests::Misc);
}

WResult WGameEngineTestAngelScript::InitializeSubTest(WInt32 iIdentifier)
{
  m_pOwnApplication->SubTestBasicsSetup();
  return W_SUCCESS;
}

WTestAppRun WGameEngineTestAngelScript::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::Types:
      m_pOwnApplication->RunTestScript("Tests/Types/TypesTest.as");
      return WTestAppRun::Quit;
    case SubTests::Strings:
      m_pOwnApplication->RunTestScript("Tests/Types/StringsTest.as");
      return WTestAppRun::Quit;
    case SubTests::Arrays:
      m_pOwnApplication->RunTestScript("Tests/Types/ArraysTest.as");
      return WTestAppRun::Quit;
    default:
      return m_pOwnApplication->SubTestBasisExec(GetSubTestName(iIdentifier));
  }
}

//////////////////////////////////////////////////////////////////////////

WGameEngineTestApplication_AngelScript::WGameEngineTestApplication_AngelScript()
  : WGameEngineTestApplication("AngelScript")
{
}

void WGameEngineTestApplication_AngelScript::SubTestBasicsSetup()
{
  LoadScene("AngelScript/AssetCache/Common/Scenes/Main.WBinScene").IgnoreResult();
}

WTestAppRun WGameEngineTestApplication_AngelScript::SubTestBasisExec(const char* szSubTestName)
{
  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  W_LOCK(m_pWorld->GetWriteMarker());

  WGameObject* pTests = nullptr;
  if (m_pWorld->TryGetObjectWithGlobalKey("Tests", pTests) == false)
  {
    W_TEST_FAILURE("Failed to retrieve AngelScript Tests-Object", "");
    return WTestAppRun::Quit;
  }

  const WStringBuilder sMsg(szSubTestName, "Test");

  WMsgGenericEvent msg;
  msg.m_sMessage.Assign(sMsg);
  pTests->SendMessageRecursive(msg);

  if (msg.m_sMessage == WTempHashedString("repeat"))
    return WTestAppRun::Continue;

  W_TEST_STRING(msg.m_sMessage, "done");

  return WTestAppRun::Quit;
}

void WGameEngineTestApplication_AngelScript::RunTestScript(WStringView sScriptPath)
{
  // Load and process test script
  {
    WStringBuilder sTestCode;
    WStringBuilder sProcessedCode;
    {
      WFileReader read;
      if (read.Open(sScriptPath).Failed())
      {
        WLog::Error("Failed to open file '{}'.", sScriptPath);
        return;
      }
      sTestCode.ReadAll(read);
    }
    if (WAngelScriptEngineSingleton::PreprocessCode(sScriptPath, sTestCode, &sProcessedCode, nullptr).Failed())
    {
      WLog::Error("Failed to preprocess code '{}'.", sScriptPath);
      return;
    }
    m_sCode = sProcessedCode;
    m_sCode.Split(true, m_Lines, "\n");
  }

  W_LOCK(m_pWorld->GetWriteMarker());

  auto pAsEngine = WAngelScriptEngineSingleton::GetSingleton();
  asIScriptModule* pModule = pAsEngine->SetModuleCode("ScriptTest", m_sCode, true);
  if (pModule == nullptr)
  {
    WLog::Error("Failed to create AngelScript module.");
    return;
  }
  asIScriptContext* m_pContext = pAsEngine->GetEngine()->CreateContext();
  W_SCOPE_EXIT(m_pContext->Release(););
  AS_CHECK(m_pContext->SetExceptionCallback(asMETHOD(WGameEngineTestApplication_AngelScript, TestScriptExceptionCallback), this, asCALL_THISCALL));

  asIScriptFunction* func = pModule->GetFunctionByName("ExecuteTests");
  m_pContext->Prepare(func);

  W_TEST_INT(m_pContext->Execute(), asEXECUTION_FINISHED);
}

void WGameEngineTestApplication_AngelScript::TestScriptExceptionCallback(asIScriptContext* pContext)
{
  WLog::Error("AS Exception '{}'", pContext->GetExceptionString());

  const WUInt32 uiNumLevels = pContext->GetCallstackSize();
  for (WUInt32 i = 0; i < uiNumLevels; ++i)
  {
    const char* szSection = nullptr;
    const WInt32 iOriginalLine = pContext->GetLineNumber(i, nullptr, &szSection);

    WInt32 iLine = iOriginalLine;
    WStringView sSection = szSection;
    WAngelScriptEngineSingleton::FindCorrectSectionAndLine(m_Lines, iLine, sSection);

    WStringBuilder line("  ");
    if (asIScriptFunction* pFunc = pContext->GetFunction(i))
    {
      if (!WStringUtils::IsNullOrEmpty(pFunc->GetNamespace()))
      {
        line.Append(pFunc->GetNamespace(), "::");
      }

      if (!WStringUtils::IsNullOrEmpty(pFunc->GetObjectName()))
      {
        line.Append(pFunc->GetObjectName(), "::");
      }

      line.AppendFormat("{}() [{}: {}] -> {}", pFunc->GetName(), sSection, iLine, m_Lines[iOriginalLine - 1]);

      WLog::Error(line);
    }
    else
    {
      line.AppendFormat("<nested call> [{}: {}] -> {}", sSection, iLine, m_Lines[iOriginalLine - 1]);
    }
  }
}

#endif
