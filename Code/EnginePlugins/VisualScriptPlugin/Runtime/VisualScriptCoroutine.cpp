#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

WVisualScriptCoroutine::WVisualScriptCoroutine(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc)
  : m_Context(pDesc, WScriptAllocator::GetAllocator())
{
}

WVisualScriptCoroutine::~WVisualScriptCoroutine() = default;

void WVisualScriptCoroutine::StartWithVarargs(WArrayPtr<WVariant> arguments)
{
  auto pVisualScriptInstance = static_cast<WVisualScriptInstance*>(GetScriptInstance());
  m_Context.Initialize(*pVisualScriptInstance, arguments);
}

void WVisualScriptCoroutine::Stop()
{
  m_Context.Deinitialize();
}

WScriptCoroutine::Result WVisualScriptCoroutine::Update(WTime deltaTimeSinceLastUpdate)
{
  auto result = m_Context.Execute(deltaTimeSinceLastUpdate);
  if (result.m_NextExecAndState == WVisualScriptExecutionContext::ExecResult::State::ContinueLater)
  {
    return Result::Running(result.m_MaxDelay);
  }

  return Result::Completed();
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptCoroutineAllocator::WVisualScriptCoroutineAllocator(const WSharedPtr<const WVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

void WVisualScriptCoroutineAllocator::Deallocate(void* pObject, WAllocator* pAllocator /*= nullptr*/)
{
  W_REPORT_FAILURE("Deallocate is not supported");
}

WInternal::NewInstance<void> WVisualScriptCoroutineAllocator::AllocateInternal(WAllocator* pAllocator)
{
  return W_SCRIPT_NEW(WVisualScriptCoroutine, m_pDesc);
}
