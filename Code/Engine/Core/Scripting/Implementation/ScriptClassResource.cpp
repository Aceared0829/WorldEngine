#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScriptClassResource, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
W_RESOURCE_IMPLEMENT_COMMON_CODE(WScriptClassResource);
// clang-format on

WScriptClassResource::WScriptClassResource()
  : WResource(DoUpdate::OnAnyThread, 1)
{
}

WScriptClassResource::~WScriptClassResource() = default;

WSharedPtr<WScriptRTTI> WScriptClassResource::CreateScriptType(WStringView sName, const WRTTI* pBaseType, WScriptRTTI::FunctionList&& functions, WScriptRTTI::MessageHandlerList&& messageHandlers)
{
  WScriptRTTI::FunctionList sortedFunctions;
  for (auto pFuncProp : pBaseType->GetFunctions())
  {
    auto pBaseClassFuncAttr = pFuncProp->GetAttributeByType<WScriptBaseClassFunctionAttribute>();
    if (pBaseClassFuncAttr == nullptr)
      continue;

    WStringView sBaseClassFuncName = pFuncProp->GetPropertyName();
    sBaseClassFuncName.TrimWordStart("Reflection_");

    WUInt16 uiIndex = pBaseClassFuncAttr->GetIndex();
    sortedFunctions.EnsureCount(uiIndex + 1);

    for (WUInt32 i = 0; i < functions.GetCount(); ++i)
    {
      auto& pScriptFuncProp = functions[i];
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        functions.RemoveAtAndSwap(i);
        break;
      }
    }
  }

  m_pType = W_SCRIPT_NEW(WScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
  return m_pType;
}

void WScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}

WSharedPtr<WScriptCoroutineRTTI> WScriptClassResource::CreateScriptCoroutineType(WStringView sScriptClassName, WStringView sFunctionName, WUniquePtr<WRTTIAllocator>&& pAllocator)
{
  WStringBuilder sCoroutineTypeName;
  sCoroutineTypeName.Set(sScriptClassName, "::", sFunctionName, "<Coroutine>");

  WSharedPtr<WScriptCoroutineRTTI> pCoroutineType = W_SCRIPT_NEW(WScriptCoroutineRTTI, sCoroutineTypeName, std::move(pAllocator));
  m_CoroutineTypes.PushBack(pCoroutineType);

  return pCoroutineType;
}

void WScriptClassResource::DeleteAllScriptCoroutineTypes()
{
  m_CoroutineTypes.Clear();
}


W_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptClassResource);
