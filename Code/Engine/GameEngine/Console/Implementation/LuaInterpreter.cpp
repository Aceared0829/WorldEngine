#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <GameEngine/Console/LuaInterpreter.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static void AllowScriptCVarAccess(WLuaWrapper& ref_script);

static const WString GetNextWord(WStringView& ref_sString)
{
  const char* szStartWord = WStringUtils::SkipCharacters(ref_sString.GetStartPointer(), WStringUtils::IsWhiteSpace, false);
  const char* szEndWord = WStringUtils::FindWordEnd(szStartWord, WStringUtils::IsIdentifierDelimiter_C_Code, true);

  ref_sString = WStringView(szEndWord);

  return WStringView(szStartWord, szEndWord);
}

static WString GetRestWords(WStringView sString)
{
  return WStringUtils::SkipCharacters(sString.GetStartPointer(), WStringUtils::IsWhiteSpace, false);
}

static int LUAFUNC_ConsoleFunc(lua_State* pState)
{
  WLuaWrapper s(pState);

  WConsoleFunctionBase* pFunc = (WConsoleFunctionBase*)s.GetFunctionLightUserData();

  if (pFunc->GetNumParameters() != s.GetNumberOfFunctionParameters())
  {
    WLog::Error("Function '{0}' expects {1} parameters, {2} were provided.", pFunc->GetName(), pFunc->GetNumParameters(), s.GetNumberOfFunctionParameters());
    return s.ReturnToScript();
  }

  WTempHybridArray<WVariant, 8> m_Params;
  m_Params.SetCount(pFunc->GetNumParameters());

  for (WUInt32 p = 0; p < pFunc->GetNumParameters(); ++p)
  {
    switch (pFunc->GetParameterType(p))
    {
      case WVariant::Type::Bool:
        m_Params[p] = s.GetBoolParameter(p);
        break;
      case WVariant::Type::Int8:
      case WVariant::Type::Int16:
      case WVariant::Type::Int32:
      case WVariant::Type::Int64:
      case WVariant::Type::UInt8:
      case WVariant::Type::UInt16:
      case WVariant::Type::UInt32:
      case WVariant::Type::UInt64:
        m_Params[p] = s.GetIntParameter(p);
        break;
      case WVariant::Type::Float:
      case WVariant::Type::Double:
        m_Params[p] = s.GetFloatParameter(p);
        break;
      case WVariant::Type::String:
        m_Params[p] = s.GetStringParameter(p);
        break;
      default:
        WLog::Error("Function '{0}': Type of parameter {1} is not supported by the Lua interpreter.", pFunc->GetName(), p);
        return s.ReturnToScript();
    }
  }

  if (!m_Params.IsEmpty())
    pFunc->Call(WArrayPtr<WVariant>(&m_Params[0], m_Params.GetCount())).IgnoreResult();
  else
    pFunc->Call(WArrayPtr<WVariant>()).IgnoreResult();

  return s.ReturnToScript();
}

static void SanitizeCVarNames(WStringBuilder& ref_sCommand)
{
  WStringBuilder sanitizedCVarName;

  for (const WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    ref_sCommand.ReplaceAll(pCVar->GetName(), sanitizedCVarName);
  }
}

static void UnSanitizeCVarName(WStringBuilder& ref_sCvarName)
{
  WStringBuilder sanitizedCVarName;

  for (const WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    if (ref_sCvarName == sanitizedCVarName)
    {
      ref_sCvarName = pCVar->GetName();
      return;
    }
  }
}

void WCommandInterpreterLua::Interpret(WCommandInterpreterState& inout_state)
{
  inout_state.m_sOutput.Clear();

  WStringBuilder sRealCommand = inout_state.m_sInput;

  if (sRealCommand.IsEmpty())
  {
    inout_state.AddOutputLine("");
    return;
  }

  sRealCommand.Trim(" \t\n\r");
  WStringBuilder sSanitizedCommand = sRealCommand;
  SanitizeCVarNames(sSanitizedCommand);

  WStringView sCommandIt = sSanitizedCommand;

  const WString sSanitizedVarName = GetNextWord(sCommandIt);
  WStringBuilder sRealVarName = sSanitizedVarName;
  UnSanitizeCVarName(sRealVarName);

  while (WStringUtils::IsWhiteSpace(sCommandIt.GetCharacter()))
  {
    sCommandIt.Shrink(1, 0);
  }

  const bool bSetValue = sCommandIt.StartsWith("=");

  if (bSetValue)
  {
    sCommandIt.Shrink(1, 0);
  }

  WStringBuilder sValue = GetRestWords(sCommandIt);
  bool bValueEmpty = sValue.IsEmpty();

  WStringBuilder sTemp;

  WLuaWrapper Script;
  AllowScriptCVarAccess(Script);

  // Register all ConsoleFunctions
  {
    WConsoleFunctionBase* pFunc = WConsoleFunctionBase::GetFirstInstance();
    while (pFunc)
    {
      Script.RegisterCFunction(pFunc->GetName().GetData(sTemp), LUAFUNC_ConsoleFunc, pFunc);

      pFunc = pFunc->GetNextInstance();
    }
  }

  sTemp = "> ";
  sTemp.Append(sRealCommand);
  inout_state.AddOutputLine(sTemp, WConsoleString::Type::Executed);

  WCVar* pCVAR = WCVar::FindCVarByName(sRealVarName.GetData());
  if (pCVAR != nullptr)
  {
    if ((bSetValue) && (sValue == "") && (pCVAR->GetType() == WCVarType::Bool))
    {
      // someone typed "myvar =" -> on bools this is the short form for "myvar = not myvar" (toggle), so insert the rest here

      bValueEmpty = false;

      sSanitizedCommand.AppendFormat(" not {0}", sSanitizedVarName);
    }

    if (bSetValue && !bValueEmpty)
    {
      WMuteLog muteLog;

      if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
      {
        inout_state.AddOutputLine("  Error Executing Command.", WConsoleString::Type::Error);
        return;
      }
      else
      {
        if (pCVAR->GetFlags().IsAnySet(WCVarFlags::ShowRequiresRestartMsg))
        {
          inout_state.AddOutputLine("  This change takes only effect after a restart.", WConsoleString::Type::Note);
        }

        sTemp.SetFormat("  {0} = {1}", sRealVarName, GetFullInfoAsString(pCVAR));
        inout_state.AddOutputLine(sTemp, WConsoleString::Type::Success);
      }
    }
    else
    {
      sTemp.SetFormat("{0} = {1}", sRealVarName, GetFullInfoAsString(pCVAR));
      inout_state.AddOutputLine(sTemp);

      if (!pCVAR->GetDescription().IsEmpty())
      {
        sTemp.SetFormat("  Description: {0}", pCVAR->GetDescription());
        inout_state.AddOutputLine(sTemp, WConsoleString::Type::Success);
      }
      else
        inout_state.AddOutputLine("  No Description available.", WConsoleString::Type::Success);
    }

    return;
  }
  else
  {
    WMuteLog muteLog;

    if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
    {
      inout_state.AddOutputLine("  Error Executing Command.", WConsoleString::Type::Error);
      return;
    }
  }
}

static int LUAFUNC_ReadCVAR(lua_State* pState)
{
  WLuaWrapper s(pState);

  WStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  WCVar* pCVar = WCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValueNil();
    return s.ReturnToScript();
  }

  switch (pCVar->GetType())
  {
    case WCVarType::Int:
    {
      WCVarInt* pVar = (WCVarInt*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case WCVarType::Bool:
    {
      WCVarBool* pVar = (WCVarBool*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case WCVarType::Float:
    {
      WCVarFloat* pVar = (WCVarFloat*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case WCVarType::String:
    {
      WCVarString* pVar = (WCVarString*)pCVar;
      s.PushReturnValue(pVar->GetValue().GetData());
    }
    break;
    case WCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}


static int LUAFUNC_WriteCVAR(lua_State* pState)
{
  WLuaWrapper s(pState);

  WStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  WCVar* pCVar = WCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValue(false);
    return s.ReturnToScript();
  }

  s.PushReturnValue(true);

  switch (pCVar->GetType())
  {
    case WCVarType::Int:
    {
      WCVarInt* pVar = (WCVarInt*)pCVar;
      *pVar = s.GetIntParameter(1);
    }
    break;
    case WCVarType::Bool:
    {
      WCVarBool* pVar = (WCVarBool*)pCVar;
      *pVar = s.GetBoolParameter(1);
    }
    break;
    case WCVarType::Float:
    {
      WCVarFloat* pVar = (WCVarFloat*)pCVar;
      *pVar = s.GetFloatParameter(1);
    }
    break;
    case WCVarType::String:
    {
      WCVarString* pVar = (WCVarString*)pCVar;
      *pVar = s.GetStringParameter(1);
    }
    break;
    case WCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}

static void AllowScriptCVarAccess(WLuaWrapper& ref_script)
{
  ref_script.RegisterCFunction("ReadCVar", LUAFUNC_ReadCVAR);
  ref_script.RegisterCFunction("WriteCVar", LUAFUNC_WriteCVAR);

  WStringBuilder sInit = "\
function readcvar (t, key)\n\
return (ReadCVar (key))\n\
end\n\
\n\
function writecvar (t, key, value)\n\
if not WriteCVar (key, value) then\n\
rawset (t, key, value or false)\n\
end\n\
end\n\
\n\
setmetatable (_G, {\n\
__newindex = writecvar,\n\
__index = readcvar,\n\
__metatable = \"Access Denied\",\n\
})";

  ref_script.ExecuteString(sInit.GetData()).IgnoreResult();
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
