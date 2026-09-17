#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
W_IMPLEMENT_WORLD_MODULE(WScriptWorldModule);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScriptWorldModule, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScriptWorldModule::WScriptWorldModule(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

WScriptWorldModule::~WScriptWorldModule() = default;

void WScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = W_CREATE_MODULE_UPDATE_FUNCTION_DESC(WScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = WWorldUpdatePhase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }
}

void WScriptWorldModule::WorldClear()
{
  m_Scheduler.Clear();
}

void WScriptWorldModule::AddUpdateFunctionToSchedule(const WAbstractFunctionProperty* pFunction, void* pInstance, WTime updateInterval, bool bOnlyWhenSimulating)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtrAndFlags(pFunction, bOnlyWhenSimulating ? FunctionContext::Flags::OnlyWhenSimulating : FunctionContext::Flags::None);
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void WScriptWorldModule::RemoveUpdateFunctionToSchedule(const WAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtr(pFunction);
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

WScriptCoroutineHandle WScriptWorldModule::CreateCoroutine(const WRTTI* pCoroutineType, WStringView sName, WScriptInstance& inout_instance, WScriptCoroutineCreationMode::Enum creationMode, WScriptCoroutine*& out_pCoroutine)
{
  if (creationMode != WScriptCoroutineCreationMode::AllowOverlap)
  {
    WScriptCoroutine* pOverlappingCoroutine = nullptr;

    auto& runningCoroutines = m_InstanceToScriptCoroutines[&inout_instance];
    for (auto& hCoroutine : runningCoroutines)
    {
      WUniquePtr<WScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        pOverlappingCoroutine = pCoroutine->Borrow();
        break;
      }
    }

    if (pOverlappingCoroutine != nullptr)
    {
      if (creationMode == WScriptCoroutineCreationMode::StopOther)
      {
        StopAndDeleteCoroutine(pOverlappingCoroutine->GetHandle());
      }
      else if (creationMode == WScriptCoroutineCreationMode::DontCreateNew)
      {
        out_pCoroutine = nullptr;
        return WScriptCoroutineHandle();
      }
      else
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<WScriptCoroutine>(WScriptAllocator::GetAllocator());

  WScriptCoroutineId id = m_RunningScriptCoroutines.Insert(pCoroutine);
  pCoroutine->Initialize(id, sName, inout_instance, *this);

  m_InstanceToScriptCoroutines[&inout_instance].PushBack(WScriptCoroutineHandle(id));

  out_pCoroutine = pCoroutine;
  return WScriptCoroutineHandle(id);
}

void WScriptWorldModule::StartCoroutine(WScriptCoroutineHandle hCoroutine, WArrayPtr<WVariant> arguments)
{
  WUniquePtr<WScriptCoroutine>* pCoroutine = nullptr;
  if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine))
  {
    (*pCoroutine)->StartWithVarargs(arguments);
    (*pCoroutine)->UpdateAndSchedule();
  }
}

void WScriptWorldModule::StopAndDeleteCoroutine(WScriptCoroutineHandle hCoroutine)
{
  WUniquePtr<WScriptCoroutine> pCoroutine;
  if (m_RunningScriptCoroutines.Remove(hCoroutine.GetInternalID(), &pCoroutine) == false)
    return;

  pCoroutine->Stop();
  pCoroutine->Deinitialize();
  m_DeadScriptCoroutines.PushBack(std::move(pCoroutine));
}

void WScriptWorldModule::StopAndDeleteCoroutine(WStringView sName, WScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (WUInt32 i = 0; i < pCoroutines->GetCount();)
    {
      auto hCoroutine = (*pCoroutines)[i];

      WUniquePtr<WScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        StopAndDeleteCoroutine(hCoroutine);
      }
      else
      {
        ++i;
      }
    }
  }
}

void WScriptWorldModule::StopAndDeleteAllCoroutines(WScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (auto hCoroutine : *pCoroutines)
    {
      StopAndDeleteCoroutine(hCoroutine);
    }
  }
}

bool WScriptWorldModule::IsCoroutineFinished(WScriptCoroutineHandle hCoroutine) const
{
  return m_RunningScriptCoroutines.Contains(hCoroutine.GetInternalID()) == false;
}

void WScriptWorldModule::CallUpdateFunctions(const WWorldModule::UpdateContext& context)
{
  W_IGNORE_UNUSED(context);

  WWorld* pWorld = GetWorld();

  WTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = WClock::GetGlobalClock()->GetTimeDiff();
  }

  m_Scheduler.Update(deltaTime,
    [this](const FunctionContext& context, WTime deltaTime)
    {
      if (GetWorld()->GetWorldSimulationEnabled() || context.m_pFunctionAndFlags.GetFlags() == FunctionContext::Flags::None)
      {
        WVariant args[] = {deltaTime};
        WVariant returnValue;
        context.m_pFunctionAndFlags->Execute(context.m_pInstance, WMakeArrayPtr(args), returnValue);
      }
    });

  // Delete dead coroutines
  for (WUInt32 i = 0; i < m_DeadScriptCoroutines.GetCount(); ++i)
  {
    auto& pCoroutine = m_DeadScriptCoroutines[i];
    WScriptInstance* pInstance = pCoroutine->GetScriptInstance();
    auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance);
    W_ASSERT_DEV(pCoroutines != nullptr, "Implementation error");

    pCoroutines->RemoveAndSwap(pCoroutine->GetHandle());
    if (pCoroutines->IsEmpty())
    {
      m_InstanceToScriptCoroutines.Remove(pInstance);
    }

    pCoroutine = nullptr;
  }
  m_DeadScriptCoroutines.Clear();
}


W_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptWorldModule);
