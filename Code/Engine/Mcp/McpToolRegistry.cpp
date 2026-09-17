#include <Mcp/McpPCH.h>

#include <Mcp/McpToolRegistry.h>

WSet<const WRTTI*> WMcpToolRegistry::s_KnownTypes;
WDynamicArray<WMcpToolProvider*> WMcpToolRegistry::s_Providers;
WDynamicArray<WMcpToolDesc> WMcpToolRegistry::s_Tools;
WMap<WString, WMcpToolProvider*> WMcpToolRegistry::s_ToolLookup;
WMcpExecuteWrapper WMcpToolRegistry::s_ExecuteWrapper;

void WMcpToolRegistry::UpdateProviders()
{
  WRTTI::ForEachDerivedType<WMcpToolProvider>(
    [](const WRTTI* pRtti)
    {
      // remember the type even if it can't be allocated, so we don't look at it again
      if (s_KnownTypes.Contains(pRtti))
        return;

      s_KnownTypes.Insert(pRtti);

      // an abstract provider base declares the tools that several hosts share; only the host's concrete
      // subclass is instantiated, so the tool names cannot collide with themselves
      if (!pRtti->GetAllocator()->CanAllocate())
        return;

      WMcpToolProvider* pProvider = pRtti->GetAllocator()->Allocate<WMcpToolProvider>();
      s_Providers.PushBack(pProvider);

      pProvider->OnActivate();

      WDynamicArray<WMcpToolDesc> tools;
      pProvider->GetSupportedTools(tools);

      for (const WMcpToolDesc& tool : tools)
      {
        if (s_ToolLookup.Contains(tool.m_sName))
        {
          // two providers claiming the same name would make dispatch ambiguous, and the client would
          // see a duplicate entry in its tool list
          WLog::Error("MCP: Tool name '{}' is already in use, the one from '{}' is ignored.", tool.m_sName, pRtti->GetTypeName());
          continue;
        }

        s_ToolLookup[tool.m_sName] = pProvider;
        s_Tools.PushBack(tool);
      }
    },
    WRTTI::ForEachOptions::ExcludeNotConcrete);
}

void WMcpToolRegistry::RemoveProvider(const WRTTI* pProviderType)
{
  for (WUInt32 i = s_Providers.GetCount(); i > 0; --i)
  {
    WMcpToolProvider* pProvider = s_Providers[i - 1];

    // GetDynamicRTTI() reads the vtable, so this must run before the module is actually unmapped
    const WRTTI* pRtti = pProvider->GetDynamicRTTI();
    if (pRtti != pProviderType)
      continue;

    for (WUInt32 uiTool = s_Tools.GetCount(); uiTool > 0; --uiTool)
    {
      const WString& sToolName = s_Tools[uiTool - 1].m_sName;

      if (s_ToolLookup.GetValueOrDefault(sToolName, nullptr) == pProvider)
      {
        s_ToolLookup.Remove(sToolName);
        s_Tools.RemoveAtAndCopy(uiTool - 1);
      }
    }

    pProvider->OnDeactivate();
    pRtti->GetAllocator()->Deallocate(pProvider);

    s_Providers.RemoveAtAndCopy(i - 1);
    s_KnownTypes.Remove(pRtti);
  }
}

void WMcpToolRegistry::Clear()
{
  for (WMcpToolProvider* pProvider : s_Providers)
  {
    pProvider->OnDeactivate();
    pProvider->GetDynamicRTTI()->GetAllocator()->Deallocate(pProvider);
  }

  s_Providers.Clear();
  s_Tools.Clear();
  s_ToolLookup.Clear();
  s_KnownTypes.Clear();
}

WResult WMcpToolRegistry::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  auto it = s_ToolLookup.Find(sToolName);

  if (!it.IsValid())
    return W_FAILURE;

  WMcpToolProvider* pProvider = it.Value();

  WDelegate<void()> execute = [&]()
  {
    pProvider->Execute(sToolName, arguments, out_result);
  };

  if (s_ExecuteWrapper.IsValid())
  {
    s_ExecuteWrapper(sToolName, out_result, execute);
  }
  else
  {
    execute();
  }

  return W_SUCCESS;
}
