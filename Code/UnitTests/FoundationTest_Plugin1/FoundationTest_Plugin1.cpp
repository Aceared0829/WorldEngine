#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Reflection.h>

static WInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

WCVarInt CVar_TestInt("test1_Int", 11, WCVarFlags::Save, "Desc: test1_Int");
WCVarFloat CVar_TestFloat("test1_Float", 1.1f, WCVarFlags::RequiresRestart, "Desc: test1_Float");
WCVarBool CVar_TestBool("test1_Bool", false, WCVarFlags::None, "Desc: test1_Bool");
WCVarString CVar_TestString("test1_String", "test1", WCVarFlags::Default, "Desc: test1_String");

WCVarInt CVar_TestInt2("test1_Int2", 21, WCVarFlags::Default, "Desc: test1_Int2");
WCVarFloat CVar_TestFloat2("test1_Float2", 2.1f, WCVarFlags::Default, "Desc: test1_Float2");
WCVarBool CVar_TestBool2("test1_Bool2", true, WCVarFlags::Default, "Desc: test1_Bool2");
WCVarString CVar_TestString2("test1_String2", "test1b", WCVarFlags::Default, "Desc: test1_String2");

W_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

W_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void OnLoadPlugin()
{
  W_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  WCVarInt* pCVar = (WCVarInt*)WCVar::FindCVarByName("TestPlugin1InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  WCVarBool* pCVarPlugin2Inited = (WCVarBool*)WCVar::FindCVarByName("test2_Inited");
  if (pCVarPlugin2Inited)
  {
    W_TEST_BOOL(*pCVarPlugin2Inited == false); // Although Plugin2 is present, it should not yet have been initialized
  }
}

void OnUnloadPlugin()
{
  W_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  WCVarInt* pCVar = (WCVarInt*)WCVar::FindCVarByName("TestPlugin1UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin1, TestSubSystem1)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "PluginGroup_Plugin1"
  //END_SUBSYSTEM_DEPENDENCIES

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

struct WTestStruct2
{
  float m_fFloat2;

  WTestStruct2() { m_fFloat2 = 42.0f; }
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTestStruct2);

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WTestStruct2, WNoBase, 1, WRTTIDefaultAllocator<WTestStruct2>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Float2", m_fFloat2),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
