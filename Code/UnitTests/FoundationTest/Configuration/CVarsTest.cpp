#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(Configuration);

#define WCVarValueDefault WCVarValue::Default
#define WCVarValueStored WCVarValue::Stored
#define WCVarValueRestart WCVarValue::DelayedSync

// Interestingly using 'WCVarValue::Default' directly inside a macro does not work. (?!)
#define CHECK_CVAR(var, Current, Default, Stored, Restart)      \
  W_TEST_BOOL(var != nullptr);                                 \
  if (var != nullptr)                                           \
  {                                                             \
    W_TEST_BOOL(var->GetValue() == Current);                   \
    W_TEST_BOOL(var->GetValue(WCVarValueDefault) == Default); \
    W_TEST_BOOL(var->GetValue(WCVarValueStored) == Stored);   \
    W_TEST_BOOL(var->GetValue(WCVarValueRestart) == Restart); \
  }

static WInt32 iChangedValue = 0;
static WInt32 iChangedRestart = 0;

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

static void ChangedCVar(const WCVarEvent& e)
{
  switch (e.m_EventType)
  {
    case WCVarEvent::ValueChanged:
      ++iChangedValue;
      break;
    case WCVarEvent::DelayedSyncValueChanged:
      ++iChangedRestart;
      break;
    default:
      break;
  }
}

#endif

W_CREATE_SIMPLE_TEST(Configuration, CVars)
{
  iChangedValue = 0;
  iChangedRestart = 0;

  // setup the filesystem
  // we need it to test the storing of cvars (during plugin reloading)

  WStringBuilder sOutputFolder1 = WTestFramework::GetInstance()->GetAbsOutputPath();

  W_TEST_BOOL(WFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "test", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

  // Delete all cvar setting files
  {
    WStringBuilder sConfigFile;

    sConfigFile = ":output/CVars/CVars_" WFoundationTest_Plugin1 ".cfg";

    WFileSystem::DeleteFile(sConfigFile.GetData());

    sConfigFile = ":output/CVars/CVars_" WFoundationTest_Plugin2 ".cfg";

    WFileSystem::DeleteFile(sConfigFile.GetData());
  }

  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Int2");
  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102");

  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Float2");
  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102.2");

  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Bool2");
  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("false");

  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_String2");
  WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("test1c");

  WCVar::SetStorageFolder(":output/CVars");
  WCVar::LoadCVars(); // should do nothing (no settings files available)

  W_TEST_BLOCK(WTestBlock::Enabled, "SaveCVarsToFile and LoadCVarsFromFile again")
  {
    const char* cvarConfigFileDir = WTestFramework::GetInstance()->GetAbsOutputPath();
    W_TEST_BOOL_MSG(WFileSystem::AddDataDirectory(cvarConfigFileDir, "CVarsTest", "CVarConfigTempDir", WDataDirUsage::AllowWrites) == W_SUCCESS, "Failed to mount data dir '%s'", cvarConfigFileDir);
    WStringView cvarConfigFile = ":CVarConfigTempDir/CVars.cfg";

    WCVarInt testCVarInt("testCVarInt", 0, WCVarFlags::Default, "Test");
    WCVarFloat testCVarFloat("testCVarFloat", 0.0f, WCVarFlags::Default, "Test");
    WCVarBool testCVarBool("testCVarBool", false, WCVarFlags::Save, "Test");
    WCVarString testCVarString("testCVarString", "", WCVarFlags::Save, "Test");

    // ignore save flag = false
    {
      testCVarInt = 481516;
      testCVarFloat = 23.42f;
      testCVarBool = true;
      testCVarString = "Hello World!";

      bool bIgnoreSaveFlag = false;
      WCVar::SaveCVarsToFile(cvarConfigFile, bIgnoreSaveFlag);
      W_TEST_BOOL(WFileSystem::ExistsFile(cvarConfigFile) == W_SUCCESS);

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      WDynamicArray<WCVar*> outCVars;
      constexpr bool bOnlyNewOnes = false;
      constexpr bool bSetAsCurrentValue = true;
      WCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      W_TEST_INT(testCVarInt, 0);
      W_TEST_FLOAT(testCVarFloat, 0.0f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(testCVarBool == true);
      W_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      W_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarBool));
      W_TEST_BOOL(outCVars.Contains(&testCVarString));

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      // Even if we ignore the save flag the result should be same as above since we only stored CVars with the save flag in the file.
      bIgnoreSaveFlag = true;
      WCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      W_TEST_INT(testCVarInt, 0);
      W_TEST_FLOAT(testCVarFloat, 0.0f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(testCVarBool == true);
      W_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      W_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarBool));
      W_TEST_BOOL(outCVars.Contains(&testCVarString));

      WFileSystem::DeleteFile(cvarConfigFile);
    }

    // ignore save flag = true
    {
      testCVarInt = 481516;
      testCVarFloat = 23.42f;
      testCVarBool = true;
      testCVarString = "Hello World!";

      bool bIgnoreSaveFlag = true;
      WCVar::SaveCVarsToFile(cvarConfigFile, bIgnoreSaveFlag);
      W_TEST_BOOL(WFileSystem::ExistsFile(cvarConfigFile) == W_SUCCESS);

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      WDynamicArray<WCVar*> outCVars;
      constexpr bool bOnlyNewOnes = false;
      constexpr bool bSetAsCurrentValue = true;
      // Check whether the save flag is correctly checked during load now that we have saved all CVars to the file.
      bIgnoreSaveFlag = false;
      WCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      W_TEST_INT(testCVarInt, 0);
      W_TEST_FLOAT(testCVarFloat, 0.0f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(testCVarBool == true);
      W_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      W_TEST_BOOL(outCVars.Contains(&testCVarInt) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarFloat) == false);
      W_TEST_BOOL(outCVars.Contains(&testCVarBool));
      W_TEST_BOOL(outCVars.Contains(&testCVarString));

      testCVarInt = 0;
      testCVarFloat = 0.0f;
      testCVarBool = false;
      testCVarString = "";

      // Now load all cvars stored in the file.
      bIgnoreSaveFlag = true;
      WCVar::LoadCVarsFromFile(cvarConfigFile, bOnlyNewOnes, bSetAsCurrentValue, bIgnoreSaveFlag, &outCVars);

      W_TEST_INT(testCVarInt, 481516);
      W_TEST_FLOAT(testCVarFloat, 23.42f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(testCVarBool == true);
      W_TEST_STRING(testCVarString.GetValue(), "Hello World!");

      W_TEST_BOOL(outCVars.Contains(&testCVarInt));
      W_TEST_BOOL(outCVars.Contains(&testCVarFloat));
      W_TEST_BOOL(outCVars.Contains(&testCVarBool));
      W_TEST_BOOL(outCVars.Contains(&testCVarString));

      WFileSystem::DeleteFile(cvarConfigFile);
    }


    W_TEST_BOOL(WFileSystem::RemoveDataDirectory("CVarConfigTempDir"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "No Plugin Loaded")
  {
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String") == nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test2_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_String") == nullptr);
  }

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

  W_TEST_BLOCK(WTestBlock::Enabled, "Plugin1 Loaded")
  {
    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin1) == W_SUCCESS);

    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String") != nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int2") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float2") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool2") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String2") != nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test2_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_String") == nullptr);

    WPlugin::UnloadAllPlugins();
  }

#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String") == nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test2_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_String") == nullptr);
  }

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

  W_TEST_BLOCK(WTestBlock::Enabled, "Plugin2 Loaded")
  {
    // Plugin2 should automatically load Plugin1 with it

    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin2) == W_SUCCESS);

    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String") != nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test2_Int") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Float") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Bool") != nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_String") != nullptr);

    WPlugin::UnloadAllPlugins();
  }

#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test1_String") == nullptr);

    W_TEST_BOOL(WCVar::FindCVarByName("test2_Int") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Float") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_Bool") == nullptr);
    W_TEST_BOOL(WCVar::FindCVarByName("test2_String") == nullptr);
  }

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Value Test")
  {
    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin2) == W_SUCCESS);

    // CVars from Plugin 1
    {
      WCVarInt* pInt = (WCVarInt*)WCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 11, 11, 11, 11);

      if (pInt)
      {
        W_TEST_BOOL(pInt->GetType() == WCVarType::Int);
        W_TEST_BOOL(pInt->GetName() == "test1_Int");
        W_TEST_BOOL(pInt->GetDescription() == "Desc: test1_Int");

        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 12;
        CHECK_CVAR(pInt, 12, 11, 11, 12);
        W_TEST_INT(iChangedValue, 1);
        W_TEST_INT(iChangedRestart, 0);

        // no change
        *pInt = 12;
        W_TEST_INT(iChangedValue, 1);
        W_TEST_INT(iChangedRestart, 0);
      }

      WCVarFloat* pFloat = (WCVarFloat*)WCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.1f);

      if (pFloat)
      {
        W_TEST_BOOL(pFloat->GetType() == WCVarType::Float);
        W_TEST_BOOL(pFloat->GetName() == "test1_Float");
        W_TEST_BOOL(pFloat->GetDescription() == "Desc: test1_Float");

        pFloat->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pFloat = 1.2f;
        CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.2f);

        W_TEST_INT(iChangedValue, 1);
        W_TEST_INT(iChangedRestart, 1);

        // no change
        *pFloat = 1.2f;
        W_TEST_INT(iChangedValue, 1);
        W_TEST_INT(iChangedRestart, 1);

        pFloat->SetToDelayedSyncValue();
        CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.1f, 1.2f);

        W_TEST_INT(iChangedValue, 2);
        W_TEST_INT(iChangedRestart, 1);
      }

      WCVarBool* pBool = (WCVarBool*)WCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      if (pBool)
      {
        W_TEST_BOOL(pBool->GetType() == WCVarType::Bool);
        W_TEST_BOOL(pBool->GetName() == "test1_Bool");
        W_TEST_BOOL(pBool->GetDescription() == "Desc: test1_Bool");

        *pBool = true;
        CHECK_CVAR(pBool, true, false, false, true);
      }

      WCVarString* pString = (WCVarString*)WCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");

      if (pString)
      {
        W_TEST_BOOL(pString->GetType() == WCVarType::String);
        W_TEST_BOOL(pString->GetName() == "test1_String");
        W_TEST_BOOL(pString->GetDescription() == "Desc: test1_String");

        *pString = "test1_value2";
        CHECK_CVAR(pString, "test1_value2", "test1", "test1", "test1_value2");
      }
    }

    // CVars from Plugin 2
    {
      WCVarInt* pInt = (WCVarInt*)WCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      if (pInt)
      {
        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 23;
        CHECK_CVAR(pInt, 23, 22, 22, 23);
        W_TEST_INT(iChangedValue, 3);
        W_TEST_INT(iChangedRestart, 1);
      }

      WCVarFloat* pFloat = (WCVarFloat*)WCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      if (pFloat)
      {
        *pFloat = 2.3f;
        CHECK_CVAR(pFloat, 2.3f, 2.2f, 2.2f, 2.3f);
      }

      WCVarBool* pBool = (WCVarBool*)WCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, true, true, true, true);

      if (pBool)
      {
        *pBool = false;
        CHECK_CVAR(pBool, false, true, true, false);
      }

      WCVarString* pString = (WCVarString*)WCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2", "test2", "test2", "test2");

      if (pString)
      {
        *pString = "test2_value2";
        CHECK_CVAR(pString, "test2", "test2", "test2", "test2_value2");

        pString->SetToDelayedSyncValue();
        CHECK_CVAR(pString, "test2_value2", "test2", "test2", "test2_value2");
      }
    }

    WPlugin::UnloadAllPlugins();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Loaded Value Test")
  {
    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin2) == W_SUCCESS);

    // CVars from Plugin 1
    {
      WCVarInt* pInt = (WCVarInt*)WCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 12, 11, 12, 12);

      WCVarFloat* pFloat = (WCVarFloat*)WCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.2f, 1.2f);

      WCVarBool* pBool = (WCVarBool*)WCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      WCVarString* pString = (WCVarString*)WCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");
    }

    // CVars from Plugin 1, overridden by command line
    {
      WCVarInt* pInt = (WCVarInt*)WCVar::FindCVarByName("test1_Int2");
      CHECK_CVAR(pInt, 102, 21, 102, 102);

      WCVarFloat* pFloat = (WCVarFloat*)WCVar::FindCVarByName("test1_Float2");
      CHECK_CVAR(pFloat, 102.2f, 2.1f, 102.2f, 102.2f);

      WCVarBool* pBool = (WCVarBool*)WCVar::FindCVarByName("test1_Bool2");
      CHECK_CVAR(pBool, false, true, false, false);

      WCVarString* pString = (WCVarString*)WCVar::FindCVarByName("test1_String2");
      CHECK_CVAR(pString, "test1c", "test1b", "test1c", "test1c");
    }

    // CVars from Plugin 2
    {
      WCVarInt* pInt = (WCVarInt*)WCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      WCVarFloat* pFloat = (WCVarFloat*)WCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      WCVarBool* pBool = (WCVarBool*)WCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, false, true, false, false);

      WCVarString* pString = (WCVarString*)WCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2_value2", "test2", "test2_value2", "test2_value2");
    }

    WPlugin::UnloadAllPlugins();
  }

#endif

  WFileSystem::ClearAllDataDirectories();
}
