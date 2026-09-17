#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptCoroutine : public WScriptCoroutine
{
public:
  WVisualScriptCoroutine(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc);
  ~WVisualScriptCoroutine();

  virtual void StartWithVarargs(WArrayPtr<WVariant> arguments) override;
  virtual void Stop() override;
  virtual Result Update(WTime deltaTimeSinceLastUpdate) override;

private:
  WVisualScriptExecutionContext m_Context;
};

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptCoroutineAllocator : public WRTTIAllocator
{
public:
  WVisualScriptCoroutineAllocator(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc);

  void Deallocate(void* pObject, WAllocator* pAllocator = nullptr) override;
  WInternal::NewInstance<void> AllocateInternal(WAllocator* pAllocator) override;

private:
  WSharedPtr<const WVisualScriptGraphDescription> m_pDesc;
};
