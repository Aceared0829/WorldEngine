#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

WVisualScriptFunctionProperty::WVisualScriptFunctionProperty(WStringView sName, const WSharedPtr<const WVisualScriptGraphDescription>& pDesc)
  : WScriptFunctionProperty(sName)
  , m_pDesc(pDesc)
{
  W_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");
}

WVisualScriptFunctionProperty::~WVisualScriptFunctionProperty() = default;

void WVisualScriptFunctionProperty::Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const
{
  W_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<WVisualScriptInstance*>(pInstance);

  WVisualScriptExecutionContext context(m_pDesc, WTempAllocator::Get());
  context.Initialize(*pVisualScriptInstance, arguments);

  auto result = context.Execute(WTime::MakeZero());
  W_ASSERT_DEBUG(result.m_NextExecAndState != WVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");

  // TODO: return value
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptMessageHandler::WVisualScriptMessageHandler(const WScriptMessageDesc& desc, const WSharedPtr<const WVisualScriptGraphDescription>& pDesc)
  : WScriptMessageHandler(desc)
  , m_pDesc(pDesc)
{
  W_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_DispatchFunc = &Dispatch;
}

WVisualScriptMessageHandler::~WVisualScriptMessageHandler() = default;

// static
void WVisualScriptMessageHandler::Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg)
{
  auto pHandler = static_cast<WVisualScriptMessageHandler*>(pSelf);
  auto pComponent = static_cast<WScriptComponent*>(pInstance);
  auto pVisualScriptInstance = static_cast<WVisualScriptInstance*>(pComponent->GetScriptInstance());

  WTempHybridArray<WVariant, 8> arguments;
  pHandler->FillMessagePropertyValues(ref_msg, arguments);

  WVisualScriptExecutionContext context(pHandler->m_pDesc, WTempAllocator::Get());
  context.Initialize(*pVisualScriptInstance, arguments);

  auto result = context.Execute(WTime::MakeZero());
  W_ASSERT_DEBUG(result.m_NextExecAndState != WVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");
}
