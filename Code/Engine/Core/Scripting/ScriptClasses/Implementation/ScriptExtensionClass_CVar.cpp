#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_CVar.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_CVar, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetBoolValue, In, "Name")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetIntValue, In, "Name")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(GetStringValue, In, "Name")->AddFlags(WPropertyFlags::PureFunction),
    W_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(SetBoolValue, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(SetIntValue, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(SetFloatValue, In, "Name", In, "Value"),
    W_SCRIPT_FUNCTION_PROPERTY(SetStringValue, In, "Name", In, "Value"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("CVar"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

static WHashTable<WTempHashedString, WCVar*> s_CachedCVars;

static WCVar* FindCVarByNameCached(WStringView sName)
{
  WTempHashedString sNameHashed(sName);

  WCVar* pCVar = nullptr;
  if (!s_CachedCVars.TryGetValue(sNameHashed, pCVar))
  {
    pCVar = WCVar::FindCVarByName(sName);

    s_CachedCVars.Insert(sNameHashed, pCVar);
  }

  WCVar::s_AllCVarEvents.AddEventHandler(
    [&](const WCVarEvent& e)
    {
      if (e.m_EventType == WCVarEvent::Type::ListOfVarsChanged)
      {
        s_CachedCVars.Clear();
      }
    });

  return pCVar;
}

// static
WVariant WScriptExtensionClass_CVar::GetValue(WStringView sName)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    return {};
  }

  switch (pCVar->GetType())
  {
    case WCVarType::Bool:
      return static_cast<WCVarBool*>(pCVar)->GetValue();
    case WCVarType::Int:
      return static_cast<WCVarInt*>(pCVar)->GetValue();
    case WCVarType::Float:
      return static_cast<WCVarFloat*>(pCVar)->GetValue();
    case WCVarType::String:
      return static_cast<WCVarString*>(pCVar)->GetValue();

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return {};
}

// static
bool WScriptExtensionClass_CVar::GetBoolValue(WStringView sName)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Bool)
  {
    WLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return false;
  }

  return static_cast<WCVarBool*>(pCVar)->GetValue();
}

// static
int WScriptExtensionClass_CVar::GetIntValue(WStringView sName)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Int)
  {
    WLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return 0;
  }

  return static_cast<WCVarInt*>(pCVar)->GetValue();
}

// static
float WScriptExtensionClass_CVar::GetFloatValue(WStringView sName)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Float)
  {
    WLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return 0;
  }

  return static_cast<WCVarFloat*>(pCVar)->GetValue();
}

// static
WString WScriptExtensionClass_CVar::GetStringValue(WStringView sName)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::String)
  {
    WLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return "";
  }

  return static_cast<WCVarString*>(pCVar)->GetValue();
}

// static
void WScriptExtensionClass_CVar::SetValue(WStringView sName, const WVariant& value)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    WLog::Error("CVar '{}' does not exist.", sName);
    return;
  }

  switch (pCVar->GetType())
  {
    case WCVarType::Bool:
    {
      WCVarBool* pVar = static_cast<WCVarBool*>(pCVar);
      *pVar = value.ConvertTo<bool>();
      break;
    }

    case WCVarType::Int:
    {
      WCVarInt* pVar = static_cast<WCVarInt*>(pCVar);
      *pVar = value.ConvertTo<int>();
      break;
    }

    case WCVarType::Float:
    {
      WCVarFloat* pVar = static_cast<WCVarFloat*>(pCVar);
      *pVar = value.ConvertTo<float>();
      break;
    }

    case WCVarType::String:
    {
      WCVarString* pVar = static_cast<WCVarString*>(pCVar);
      *pVar = value.ConvertTo<WString>();
      break;
    }

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }
}

// static
void WScriptExtensionClass_CVar::SetBoolValue(WStringView sName, bool bValue)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Bool)
  {
    WLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return;
  }

  WCVarBool* pVar = static_cast<WCVarBool*>(pCVar);
  *pVar = bValue;
}

// static
void WScriptExtensionClass_CVar::SetIntValue(WStringView sName, int iValue)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Int)
  {
    WLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return;
  }

  WCVarInt* pVar = static_cast<WCVarInt*>(pCVar);
  *pVar = iValue;
}

// static
void WScriptExtensionClass_CVar::SetFloatValue(WStringView sName, float fValue)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::Float)
  {
    WLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return;
  }

  WCVarFloat* pVar = static_cast<WCVarFloat*>(pCVar);
  *pVar = fValue;
}

// static
void WScriptExtensionClass_CVar::SetStringValue(WStringView sName, const WString& sValue)
{
  WCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != WCVarType::String)
  {
    WLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return;
  }

  WCVarString* pVar = static_cast<WCVarString*>(pCVar);
  *pVar = sValue;
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_CVar);
