#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>

WCVarInt CVar_TestPlugin1InitializedCount("TestPlugin1InitCount", 0, WCVarFlags::None, "How often Plugin1 has been initialized.");
WCVarInt CVar_TestPlugin1UninitializedCount("TestPlugin1UninitCount", 0, WCVarFlags::None, "How often Plugin1 has been uninitialized.");
WCVarInt CVar_TestPlugin1Reloaded("TestPlugin1Reloaded", 0, WCVarFlags::None, "How often Plugin1 has been reloaded (counts init AND de-init).");

WCVarInt CVar_TestPlugin2InitializedCount("TestPlugin2InitCount", 0, WCVarFlags::None, "How often Plugin2 has been initialized.");
WCVarInt CVar_TestPlugin2UninitializedCount("TestPlugin2UninitCount", 0, WCVarFlags::None, "How often Plugin2 has been uninitialized.");
WCVarInt CVar_TestPlugin2Reloaded("TestPlugin2Reloaded", 0, WCVarFlags::None, "How often Plugin2 has been reloaded (counts init AND de-init).");
WCVarBool CVar_TestPlugin2FoundDependencies("TestPlugin2FoundDependencies", false, WCVarFlags::None, "Whether Plugin2 found all its dependencies (other plugins).");

W_CREATE_SIMPLE_TEST(Configuration, Plugin)
{
  CVar_TestPlugin1InitializedCount = 0;
  CVar_TestPlugin1UninitializedCount = 0;
  CVar_TestPlugin1Reloaded = 0;
  CVar_TestPlugin2InitializedCount = 0;
  CVar_TestPlugin2UninitializedCount = 0;
  CVar_TestPlugin2Reloaded = 0;
  CVar_TestPlugin2FoundDependencies = false;

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

  W_TEST_BLOCK(WTestBlock::Enabled, "LoadPlugin")
  {
    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin2) == W_SUCCESS);
    W_TEST_BOOL(WPlugin::LoadPlugin(WFoundationTest_Plugin2, WPluginLoadFlags::PluginIsOptional) == W_SUCCESS); // loading already loaded plugin is always a success

    W_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    W_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    W_TEST_INT(CVar_TestPlugin1UninitializedCount, 0);
    W_TEST_INT(CVar_TestPlugin2UninitializedCount, 0);

    W_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    W_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    W_TEST_BOOL(CVar_TestPlugin2FoundDependencies);

    // this will fail the FoundationTests, as it logs an error
    // W_TEST_BOOL(WPlugin::LoadPlugin("Test") == W_FAILURE); // plugin does not exist
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UnloadPlugin")
  {
    CVar_TestPlugin2FoundDependencies = false;
    WPlugin::UnloadAllPlugins();

    W_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    W_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    W_TEST_INT(CVar_TestPlugin1UninitializedCount, 1);
    W_TEST_INT(CVar_TestPlugin2UninitializedCount, 1);

    W_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    W_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    W_TEST_BOOL(CVar_TestPlugin2FoundDependencies);
    W_TEST_BOOL(WPlugin::LoadPlugin("Test", WPluginLoadFlags::PluginIsOptional) == W_FAILURE); // plugin does not exist
  }

#endif
}
