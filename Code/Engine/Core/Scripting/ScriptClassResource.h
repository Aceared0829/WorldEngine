#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptRTTI.h>

class WWorld;
using WScriptClassResourceHandle = WTypedResourceHandle<class WScriptClassResource>;

/// Resource representing a script class with its type information and instantiation capabilities.
///
/// Base class for script resources that define class types for scripting languages. Manages
/// script type creation, instantiation, and coroutine type handling. Derived classes implement
/// language-specific instantiation logic.
class W_CORE_DLL WScriptClassResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WScriptClassResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WScriptClassResource);

public:
  WScriptClassResource();
  ~WScriptClassResource();

  const WSharedPtr<WScriptRTTI>& GetType() const { return m_pType; }

  virtual WUniquePtr<WScriptInstance> Instantiate(WReflectedClass& inout_owner, WWorld* pWorld) const = 0;

protected:
  WSharedPtr<WScriptRTTI> CreateScriptType(WStringView sName, const WRTTI* pBaseType, WScriptRTTI::FunctionList&& functions, WScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  WSharedPtr<WScriptCoroutineRTTI> CreateScriptCoroutineType(WStringView sScriptClassName, WStringView sFunctionName, WUniquePtr<WRTTIAllocator>&& pAllocator);
  void DeleteAllScriptCoroutineTypes();

  WSharedPtr<WScriptRTTI> m_pType;
  WDynamicArray<WSharedPtr<WScriptCoroutineRTTI>> m_CoroutineTypes;
};
