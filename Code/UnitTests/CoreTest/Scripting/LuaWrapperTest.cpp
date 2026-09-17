#include <CoreTest/CoreTestPCH.h>

#include <Core/Scripting/LuaWrapper.h>

W_CREATE_SIMPLE_TEST_GROUP(Scripting);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static const char* g_Script = "\
globaltable = true;\n\
function f_globaltable()\n\
end\n\
function f_NotWorking()\n\
  DoNothing();\n\
end\n\
intvar1 = 4;\n\
intvar2 = 7;\n\
floatvar1 = 4.3;\n\
floatvar2 = 7.3;\n\
boolvar1 = true;\n\
boolvar2 = false;\n\
stringvar1 = \"zweiundvierzig\";\n\
stringvar2 = \"OhWhatsInHere\";\n\
\n\
\n\
function f1()\n\
end\n\
\n\
function f2()\n\
end\n\
\n\
MyTable =\n\
{\n\
  table1 = true;\n\
  \n\
  f_table1 = function()\n\
  end;\n\
  intvar1 = 14;\n\
  intvar2 = 17;\n\
  floatvar1 = 14.3;\n\
  floatvar2 = 17.3;\n\
  boolvar1 = false;\n\
  boolvar2 = true;\n\
  stringvar1 = \"+zweiundvierzig\";\n\
  stringvar2 = \"+OhWhatsInHere\";\n\
  \n\
  SubTable =\n\
  {\n\
    table2 = true;\n\
    f_table2 = function()\n\
    end;\n\
    intvar1 = 24;\n\
  };\n\
};\n\
\n\
";

class ScriptLog : public WLogInterface
{
public:
  virtual void HandleLogMessage(const WLoggingEventData& le) override
  {
    W_TEST_FAILURE("Script Error", le.m_sText);
    W_TEST_DEBUG_BREAK;
  }
};

class ScriptLogIgnore : public WLogInterface
{
public:
  static WInt32 g_iErrors;

  virtual void HandleLogMessage(const WLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case WLogMsgType::ErrorMsg:
      case WLogMsgType::SeriousWarningMsg:
      case WLogMsgType::WarningMsg:
        ++g_iErrors;
      default:
        break;
    }
  }
};

WInt32 ScriptLogIgnore::g_iErrors = 0;

int MyFunc1(lua_State* pState)
{
  WLuaWrapper s(pState);

  W_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  return s.ReturnToScript();
}

int MyFunc2(lua_State* pState)
{
  WLuaWrapper s(pState);

  W_TEST_INT(s.GetNumberOfFunctionParameters(), 6);
  W_TEST_BOOL(s.IsParameterBool(0));
  W_TEST_BOOL(s.IsParameterFloat(1));
  W_TEST_BOOL(s.IsParameterInt(2));
  W_TEST_BOOL(s.IsParameterNil(3));
  W_TEST_BOOL(s.IsParameterString(4));
  W_TEST_BOOL(s.IsParameterString(5));

  W_TEST_BOOL(s.GetBoolParameter(0) == true);
  W_TEST_FLOAT(s.GetFloatParameter(1), 2.3f, 0.0001f);
  W_TEST_INT(s.GetIntParameter(2), 42);
  W_TEST_STRING(s.GetStringParameter(4), "test");
  W_TEST_STRING(s.GetStringParameter(5), "tut");

  return s.ReturnToScript();
}

int MyFunc3(lua_State* pState)
{
  WLuaWrapper s(pState);

  W_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  s.PushReturnValue(false);
  s.PushReturnValue(2.3f);
  s.PushReturnValue(42);
  s.PushReturnValueNil();
  s.PushReturnValue("test");
  s.PushReturnValue("tuttut", 3);

  return s.ReturnToScript();
}

int MyFunc4(lua_State* pState)
{
  WLuaWrapper s(pState);

  W_TEST_INT(s.GetNumberOfFunctionParameters(), 1);

  W_TEST_BOOL(s.IsParameterTable(0));

  W_TEST_BOOL(s.OpenTableFromParameter(0) == W_SUCCESS);

  W_TEST_BOOL(s.IsVariableAvailable("table1") == true);

  s.CloseAllTables();

  return s.ReturnToScript();
}

W_CREATE_SIMPLE_TEST(Scripting, LuaWrapper)
{
  ScriptLog Log;
  ScriptLogIgnore LogIgnore;

  WLuaWrapper sMain;
  W_TEST_BOOL(sMain.ExecuteString(g_Script, "MainScript", &Log) == W_SUCCESS);

  W_TEST_BLOCK(WTestBlock::Enabled, "ExecuteString")
  {
    WLuaWrapper s;
    ScriptLogIgnore::g_iErrors = 0;

    W_TEST_BOOL(s.ExecuteString(" pups ", "FailToCompile", &LogIgnore) == W_FAILURE);
    W_TEST_INT(ScriptLogIgnore::g_iErrors, 1);

    W_TEST_BOOL(s.ExecuteString(" pups(); ", "FailToExecute", &LogIgnore) == W_FAILURE);
    W_TEST_INT(ScriptLogIgnore::g_iErrors, 2);

    W_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == W_SUCCESS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clear")
  {
    WLuaWrapper s;
    W_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == W_SUCCESS);

    W_TEST_BOOL(s.IsVariableAvailable("globaltable") == true);
    W_TEST_BOOL(s.IsVariableAvailable("boolvar1") == true);
    W_TEST_BOOL(s.IsVariableAvailable("boolvar2") == true);
    W_TEST_BOOL(s.IsVariableAvailable("intvar1") == true);
    W_TEST_BOOL(s.IsVariableAvailable("intvar2") == true);
    W_TEST_BOOL(s.IsVariableAvailable("floatvar1") == true);
    W_TEST_BOOL(s.IsVariableAvailable("floatvar2") == true);
    W_TEST_BOOL(s.IsVariableAvailable("stringvar1") == true);
    W_TEST_BOOL(s.IsVariableAvailable("stringvar2") == true);

    s.Clear();

    // after clearing the script, these variables should not be available anymore
    W_TEST_BOOL(s.IsVariableAvailable("globaltable") == false);
    W_TEST_BOOL(s.IsVariableAvailable("boolvar1") == false);
    W_TEST_BOOL(s.IsVariableAvailable("boolvar2") == false);
    W_TEST_BOOL(s.IsVariableAvailable("intvar1") == false);
    W_TEST_BOOL(s.IsVariableAvailable("intvar2") == false);
    W_TEST_BOOL(s.IsVariableAvailable("floatvar1") == false);
    W_TEST_BOOL(s.IsVariableAvailable("floatvar2") == false);
    W_TEST_BOOL(s.IsVariableAvailable("stringvar1") == false);
    W_TEST_BOOL(s.IsVariableAvailable("stringvar2") == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsVariableAvailable (Global)")
  {
    W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("nonexisting1") == false);
    W_TEST_BOOL(sMain.IsVariableAvailable("boolvar1") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("boolvar2") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("nonexisting2") == false);
    W_TEST_BOOL(sMain.IsVariableAvailable("intvar1") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("intvar2") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("nonexisting3") == false);
    W_TEST_BOOL(sMain.IsVariableAvailable("floatvar1") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("floatvar2") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("nonexisting4") == false);
    W_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("stringvar2") == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsFunctionAvailable (Global)")
  {
    W_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting1") == false);
    W_TEST_BOOL(sMain.IsFunctionAvailable("f1") == true);
    W_TEST_BOOL(sMain.IsFunctionAvailable("f2") == true);
    W_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting2") == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIntVariable (Global)")
  {
    W_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    W_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
    W_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    W_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIntVariable (Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    W_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);
    W_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    W_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFloatVariable (Global)")
  {
    W_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFloatVariable (Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, WMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBoolVariable (Global)")
  {
    W_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
    W_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBoolVariable (Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);
    W_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    W_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetStringVariable (Global)")
  {
    W_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
    W_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetStringVariable (Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");
    W_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (int, Global)")
  {
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    sMain.SetVariable("intvar1", 27);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 27);
    sMain.SetVariable("intvar1", 4);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (int, Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    sMain.SetVariable("intvar1", 127);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 127);
    sMain.SetVariable("intvar1", 14);
    W_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (float, Global)")
  {
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, WMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 27.3f);
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 27.3f, WMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 4.3f);
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (float, Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, WMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 127.3f);
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 127.3f, WMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 14.3f);
    W_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, WMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (bool, Global)")
  {
    W_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    W_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    W_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (bool, Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    W_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    W_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (string, Global)")
  {
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "test1");
    sMain.SetVariable("stringvar1", "zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1", 3);
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "tes");
    sMain.SetVariable("stringvar1", "zweiundvierzigabc", 14);
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (string, Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+test1");
    sMain.SetVariable("stringvar1", "+zweiundvierzig");
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1", 4);
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+tes");
    sMain.SetVariable("stringvar1", "+zweiundvierzigabc", 15);
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (nil, Global)")
  {
    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    W_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "zweiundvierzig");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetVariable (nil, Table)")
  {
    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);

    W_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    W_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "+zweiundvierzig");

    sMain.CloseTable();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "OpenTable")
  {
    W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    W_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
    W_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

    W_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == true);
    W_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
    W_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

    W_TEST_BOOL(sMain.OpenTable("NotMyTable") == W_FAILURE);

    W_TEST_BOOL(sMain.OpenTable("MyTable") == W_SUCCESS);
    {
      W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      W_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      W_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      W_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      W_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      W_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      W_TEST_BOOL(sMain.OpenTable("NotMyTable") == W_FAILURE);

      W_TEST_BOOL(sMain.OpenTable("SubTable") == W_SUCCESS);
      {
        W_TEST_BOOL(sMain.OpenTable("NotMyTable") == W_FAILURE);

        W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        W_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        W_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        W_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        W_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        W_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      W_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      W_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      W_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      W_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      W_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      W_TEST_BOOL(sMain.OpenTable("NotMyTable") == W_FAILURE);

      W_TEST_BOOL(sMain.OpenTable("SubTable") == W_SUCCESS);
      {
        W_TEST_BOOL(sMain.OpenTable("NotMyTable") == W_FAILURE);

        W_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        W_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        W_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        W_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        W_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        W_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      sMain.CloseTable();
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RegisterCFunction")
  {
    W_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == false);

    sMain.RegisterCFunction("Func1", MyFunc1);

    W_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Call Lua Function")
  {
    W_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    W_TEST_BOOL(sMain.PrepareFunctionCall("f_globaltable") == true);
    W_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == W_SUCCESS);

    ScriptLogIgnore::g_iErrors = 0;
    W_TEST_BOOL(sMain.PrepareFunctionCall("f_NotWorking") == true);
    W_TEST_BOOL(sMain.CallPreparedFunction(0, &LogIgnore) == W_FAILURE);
    W_TEST_INT(ScriptLogIgnore::g_iErrors, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Call C Function")
  {
    W_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    if (sMain.IsFunctionAvailable("Func1") == false)
      sMain.RegisterCFunction("Func1", MyFunc1);

    W_TEST_BOOL(sMain.PrepareFunctionCall("Func1") == true);

    W_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == W_SUCCESS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Call C Function with Parameters")
  {
    if (sMain.IsFunctionAvailable("Func2") == false)
      sMain.RegisterCFunction("Func2", MyFunc2);

    W_TEST_BOOL(sMain.PrepareFunctionCall("Func2") == true);

    sMain.PushParameter(true);
    sMain.PushParameter(2.3f);
    sMain.PushParameter(42);
    sMain.PushParameterNil();
    sMain.PushParameter("test");
    sMain.PushParameter("tuttut", 3);

    W_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == W_SUCCESS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Call C Function with Return Values")
  {
    if (sMain.IsFunctionAvailable("Func3") == false)
      sMain.RegisterCFunction("Func3", MyFunc3);

    W_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    W_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == W_SUCCESS);

    W_TEST_BOOL(sMain.IsReturnValueBool(0));
    W_TEST_BOOL(sMain.IsReturnValueFloat(1));
    W_TEST_BOOL(sMain.IsReturnValueInt(2));
    W_TEST_BOOL(sMain.IsReturnValueNil(3));
    W_TEST_BOOL(sMain.IsReturnValueString(4));
    W_TEST_BOOL(sMain.IsReturnValueString(5));

    W_TEST_BOOL(sMain.GetBoolReturnValue(0) == false);
    W_TEST_FLOAT(sMain.GetFloatReturnValue(1), 2.3f, 0.0001f);
    W_TEST_INT(sMain.GetIntReturnValue(2), 42);
    W_TEST_STRING(sMain.GetStringReturnValue(4), "test");
    W_TEST_STRING(sMain.GetStringReturnValue(5), "tut");

    sMain.DiscardReturnValues();

    W_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    W_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == W_SUCCESS);

    sMain.DiscardReturnValues();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Call C Function with Table Parameter")
  {
    if (sMain.IsFunctionAvailable("Func4") == false)
      sMain.RegisterCFunction("Func4", MyFunc4);

    W_TEST_BOOL(sMain.PrepareFunctionCall("Func4") == true);

    sMain.PushTable("MyTable", true);

    W_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == W_SUCCESS);
  }
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
