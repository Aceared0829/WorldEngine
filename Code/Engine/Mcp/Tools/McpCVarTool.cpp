#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/Tools/McpCVarTool.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMcpCVarTool, 1, WRTTIDefaultAllocator<WMcpCVarTool>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// The names used in tool arguments and results, lower case because that is what the AI has to type.
  WStringView CVarTypeToString(WCVarType::Enum type)
  {
    switch (type)
    {
      case WCVarType::Int:
        return "int";
      case WCVarType::Float:
        return "float";
      case WCVarType::Bool:
        return "bool";
      case WCVarType::String:
        return "string";
      default:
        return "unknown";
    }
  }

  /// Whether the CVar still holds the value it was declared with.
  ///
  /// One place, because both the listing filter and the decision whether to report 'default' need it,
  /// and each type has to be compared through its own pointer cast.
  bool IsAtDefaultValue(const WCVar* pCVar)
  {
    switch (pCVar->GetType())
    {
      case WCVarType::Int:
      {
        const WCVarInt* p = static_cast<const WCVarInt*>(pCVar);
        return p->GetValue() == p->GetValue(WCVarValue::Default);
      }
      case WCVarType::Float:
      {
        const WCVarFloat* p = static_cast<const WCVarFloat*>(pCVar);
        return p->GetValue() == p->GetValue(WCVarValue::Default);
      }
      case WCVarType::Bool:
      {
        const WCVarBool* p = static_cast<const WCVarBool*>(pCVar);
        return p->GetValue() == p->GetValue(WCVarValue::Default);
      }
      case WCVarType::String:
      {
        const WCVarString* p = static_cast<const WCVarString*>(pCVar);
        return p->GetValue() == p->GetValue(WCVarValue::Default);
      }
      default:
        return true;
    }
  }

  /// Whether a value was written that the engine is not reading yet. Only ever true for CVars
  /// flagged RequiresDelayedSync.
  bool HasPendingValue(const WCVar* pCVar)
  {
    switch (pCVar->GetType())
    {
      case WCVarType::Int:
        return static_cast<const WCVarInt*>(pCVar)->HasDelayedSyncValueChanged();
      case WCVarType::Float:
        return static_cast<const WCVarFloat*>(pCVar)->HasDelayedSyncValueChanged();
      case WCVarType::Bool:
        return static_cast<const WCVarBool*>(pCVar)->HasDelayedSyncValueChanged();
      case WCVarType::String:
        return static_cast<const WCVarString*>(pCVar)->HasDelayedSyncValueChanged();
      default:
        return false;
    }
  }
} // namespace

void WMcpCVarTool::WriteValue(WMcpJsonWriter& ref_writer, WStringView sFieldName, const WCVar* pCVar, WUInt32 uiWhichValue)
{
  const WCVarValue::Enum which = static_cast<WCVarValue::Enum>(uiWhichValue);

  // Written as the JSON type that matches the CVar, not as a string: a client that reads a bool and
  // writes it straight back must not have to know it was quoted on the way out.
  switch (pCVar->GetType())
  {
    case WCVarType::Int:
      ref_writer.AddVariableInt32(sFieldName, static_cast<const WCVarInt*>(pCVar)->GetValue(which));
      break;
    case WCVarType::Float:
      ref_writer.AddVariableFloat(sFieldName, static_cast<const WCVarFloat*>(pCVar)->GetValue(which));
      break;
    case WCVarType::Bool:
      ref_writer.AddVariableBool(sFieldName, static_cast<const WCVarBool*>(pCVar)->GetValue(which));
      break;
    case WCVarType::String:
      ref_writer.AddVariableString(sFieldName, static_cast<const WCVarString*>(pCVar)->GetValue(which).GetView());
      break;
    default:
      break;
  }
}

void WMcpCVarTool::WriteCVar(WMcpJsonWriter& ref_writer, const WCVar* pCVar)
{
  ref_writer.BeginObject();

  ref_writer.AddVariableString("name", pCVar->GetName());
  ref_writer.AddVariableString("type", CVarTypeToString(pCVar->GetType()));
  WriteValue(ref_writer, "value", pCVar, WCVarValue::Current);

  // Only when it differs, per the 'omit empty fields' rule - a CVar sitting at its default is the
  // common case and repeating the same number twice costs tokens without saying anything.
  if (!IsAtDefaultValue(pCVar))
  {
    WriteValue(ref_writer, "default", pCVar, WCVarValue::Default);
  }

  const WBitflags<WCVarFlags> flags = pCVar->GetFlags();

  if (flags.IsSet(WCVarFlags::RequiresDelayedSync))
  {
    // The trap this field exists for: writing such a CVar does not change what the engine reads until
    // the owning subsystem syncs it, so a caller that only re-read 'value' would conclude its write was
    // ignored. Reported whenever the flag is set, not only when a write is outstanding, because that is
    // what tells a caller in advance that a write here will not take effect immediately.
    ref_writer.AddVariableBool("requiresRestart", true);

    if (HasPendingValue(pCVar))
    {
      WriteValue(ref_writer, "pendingValue", pCVar, WCVarValue::DelayedSync);
    }
  }

  if (flags.IsSet(WCVarFlags::Save))
  {
    ref_writer.AddVariableBool("saved", true);
  }

  if (!pCVar->GetPluginName().IsEmpty())
  {
    ref_writer.AddVariableString("plugin", pCVar->GetPluginName());
  }

  if (!pCVar->GetDescription().IsEmpty())
  {
    ref_writer.AddVariableString("description", pCVar->GetDescription());
  }

  ref_writer.EndObject();
}

void WMcpCVarTool::GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const
{
  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "cvar_list";
    desc.m_sDescription = "Lists the CVars registered in this process, with their current value, type, owning plugin and "
                          "description. CVars are the debug and configuration switches the engine and its plugins declare - "
                          "rendering, physics and AI visualisation, resource management - so this is how to find out what can "
                          "be toggled at runtime without knowing about each feature in advance. Filter, do not dump: a process "
                          "registers hundreds. 'default' is only reported when the CVar has been changed away from it, and "
                          "'requiresRestart' marks the ones whose new value the engine will not read until the owning "
                          "subsystem syncs it.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("contains":{"type":"string","description":"Only return CVars whose name or description contains this, case insensitive."},)"
                          R"("plugin":{"type":"string","description":"Only return CVars declared by this plugin, case insensitive. Use cvar_list without filters first to see which plugin names exist."},)"
                          R"("changedOnly":{"type":"boolean","description":"Only return CVars whose value differs from their default. Cheap way to see what this process was configured with. Default false."})"
                          R"(}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "cvar_set";
    desc.m_sDescription = "Changes the value of one CVar. The value is converted to the CVar's own type, so a number may be "
                          "sent as a JSON number or as a string. Returns what the value was and what it is now.\n"
                          "If the CVar is flagged 'requiresRestart', the write goes to a pending value and what the engine "
                          "reads does NOT change until the owning subsystem syncs it - the response says so explicitly, "
                          "because re-reading the CVar afterwards would otherwise look as though the write was ignored.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"The CVar to change. Case insensitive. Use cvar_list to find it."},)"
                          R"("value":{"description":"The new value. Converted to the CVar's type; booleans also accept 'true'/'false' and 0/1."})"
                          R"(},"required":["name","value"]})";
  }
}

void WMcpCVarTool::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (sToolName == "cvar_list")
  {
    ExecuteList(arguments, out_result);
  }
  else if (sToolName == "cvar_set")
  {
    ExecuteSet(arguments, out_result);
  }
}

void WMcpCVarTool::ExecuteList(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sContains = WMcpJson::GetString(arguments, "contains");
  const WStringView sPlugin = WMcpJson::GetString(arguments, "plugin");
  const bool bChangedOnly = WMcpJson::GetBool(arguments, "changedOnly", false);

  WMcpJsonWriter writer;
  writer.BeginObject();

  WUInt32 uiTotalMatches = 0;
  WUInt32 uiReturned = 0;

  writer.BeginArray("cvars");

  // WCVar is WEnumerable, so every CVar of every loaded plugin is on this list without registration
  for (const WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (!sPlugin.IsEmpty() && !pCVar->GetPluginName().IsEqual_NoCase(sPlugin))
      continue;

    if (!sContains.IsEmpty())
    {
      const bool bInName = pCVar->GetName().FindSubString_NoCase(sContains) != nullptr;
      const bool bInDesc = pCVar->GetDescription().FindSubString_NoCase(sContains) != nullptr;

      if (!bInName && !bInDesc)
        continue;
    }

    if (bChangedOnly && IsAtDefaultValue(pCVar))
      continue;

    ++uiTotalMatches;

    if (uiReturned >= s_uiMaxResults)
      continue; // keep counting, so that 'totalMatches' reports how much was left out

    WriteCVar(writer, pCVar);
    ++uiReturned;
  }

  writer.EndArray();

  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", uiReturned);

  if (uiTotalMatches > uiReturned)
  {
    writer.AddVariableBool("truncated", true);
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void WMcpCVarTool::ExecuteSet(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sName = WMcpJson::GetString(arguments, "name");

  if (sName.IsEmpty())
  {
    out_result.SetError("No 'name' argument given.");
    return;
  }

  WCVar* pCVar = WCVar::FindCVarByName(sName);

  if (pCVar == nullptr)
  {
    WStringBuilder sError;
    sError.SetFormat("No CVar named '{}' exists in this process. Use cvar_list to find the right name; a CVar only exists once "
                     "the plugin that declares it has been loaded.",
      sName);
    out_result.SetError(sError);
    return;
  }

  const WVariant* pValue = nullptr;

  if (!arguments.TryGetValue("value", pValue) || !pValue->IsValid())
  {
    out_result.SetError("No 'value' argument given.");
    return;
  }

  // The target's type decides the conversion, not the type the client happened to send: AI clients send
  // numbers as strings and booleans as 0/1, and refusing those would be a type check dressed up as
  // validation. Everything below fails only when the value genuinely cannot be read as that type.
  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("name", pCVar->GetName());
  writer.AddVariableString("type", CVarTypeToString(pCVar->GetType()));
  WriteValue(writer, "previousValue", pCVar, WCVarValue::Current);

  WStringBuilder sConversionError;

  switch (pCVar->GetType())
  {
    case WCVarType::Int:
    {
      WResult res = W_FAILURE;
      const WInt32 iValue = static_cast<WInt32>(pValue->ConvertTo<WInt64>(&res));
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as an integer.", pValue->ConvertTo<WString>());
      else
        *static_cast<WCVarInt*>(pCVar) = iValue;
      break;
    }
    case WCVarType::Float:
    {
      WResult res = W_FAILURE;
      const float fValue = pValue->ConvertTo<float>(&res);
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as a float.", pValue->ConvertTo<WString>());
      else
        *static_cast<WCVarFloat*>(pCVar) = fValue;
      break;
    }
    case WCVarType::Bool:
    {
      WResult res = W_FAILURE;
      const bool bValue = pValue->ConvertTo<bool>(&res);
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as a boolean. Use true/false, \"true\"/\"false\" or 1/0.", pValue->ConvertTo<WString>());
      else
        *static_cast<WCVarBool*>(pCVar) = bValue;
      break;
    }
    case WCVarType::String:
    {
      WResult res = W_FAILURE;
      const WString sValue = pValue->ConvertTo<WString>(&res);
      if (res.Failed())
        sConversionError = "Value cannot be read as a string.";
      else
        *static_cast<WCVarString*>(pCVar) = sValue.GetView();
      break;
    }
    default:
      sConversionError = "This CVar has a type that this tool does not know how to write.";
      break;
  }

  if (!sConversionError.IsEmpty())
  {
    // Abandoned rather than finished: the object was opened before the conversion was attempted, and
    // returning from inside it would trip the JSON writer's 'stream was not closed' assert.
    writer.EndAll();
    out_result.SetError(sConversionError);
    return;
  }

  WriteValue(writer, "value", pCVar, WCVarValue::Current);

  if (pCVar->GetFlags().IsSet(WCVarFlags::RequiresDelayedSync))
  {
    WriteValue(writer, "pendingValue", pCVar, WCVarValue::DelayedSync);
    writer.AddVariableBool("requiresRestart", true);
    writer.AddVariableString("note",
      "This CVar only takes effect after the owning subsystem syncs it, usually at startup. 'value' is what the engine still "
      "reads and 'pendingValue' is what was just written - so re-reading this CVar will keep reporting the old value, and that "
      "is not a failed write.");
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}
