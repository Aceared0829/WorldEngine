#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static WInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

W_PLUGIN_DEPENDENCY(WFoundationTest_Plugin1);

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

WCVarInt CVar_TestInt("test2_Int", 22, WCVarFlags::None, "Desc: test2_Int");
WCVarFloat CVar_TestFloat("test2_Float", 2.2f, WCVarFlags::Default, "Desc: test2_Float");
WCVarBool CVar_TestBool("test2_Bool", true, WCVarFlags::Save, "Desc: test2_Bool");
WCVarString CVar_TestString("test2_String", "test2", WCVarFlags::RequiresRestart, "Desc: test2_String");

WCVarBool CVar_TestInited("test2_Inited", false, WCVarFlags::None, "Desc: test2_Inited");

void OnLoadPlugin()
{
  W_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  WCVarInt* pCVar = (WCVarInt*)WCVar::FindCVarByName("TestPlugin2InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  WCVarBool* pCVarDep = (WCVarBool*)WCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are available (ie. plugin1 is already loaded)
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = true;
}

void OnUnloadPlugin()
{
  W_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  WCVarInt* pCVar = (WCVarInt*)WCVar::FindCVarByName("TestPlugin2UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  WCVarBool* pCVarDep = (WCVarBool*)WCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are STILL available (ie. plugin1 is not yet unloaded)
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (WCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = false;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin2, TestSubSystem2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginGroup_Plugin1"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on
