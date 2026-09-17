#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

W_IMPLEMENT_SINGLETON(WLongOpWorkerManager);

WLongOpWorkerManager::WLongOpWorkerManager()
  : m_SingletonRegistrar(this)
{
}

WLongOpWorkerManager::~WLongOpWorkerManager() = default;

class WLongOpTask final : public WTask
{
public:
  WLongOpWorker* m_pWorkerOp = nullptr;
  WUuid m_OperationGuid;
  WProgress* m_pProgress = nullptr;

  WLongOpTask()
  {
    WStringBuilder name;
    name.SetFormat("Long Op: '{}'", "TODO: NAME"); // TODO
    ConfigureTask(name, WTaskNesting::Maybe);
  }

  ~WLongOpTask() = default;

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    WDataBuffer resultData;
    WMemoryStreamContainerWrapperStorage<WDataBuffer> storage(&resultData);
    WMemoryStreamWriter writer(&storage);

    const WResult res = m_pWorkerOp->Execute(*m_pProgress, writer);

    WLongOpWorkerManager::GetSingleton()->WorkerOperationFinished(m_OperationGuid, res, std::move(resultData));
  }
};

void WLongOpWorkerManager::ProcessCommunicationChannelEventHandler(const WProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = WDynamicCast<const WLongOpReplicationMsg*>(e.m_pMessage))
  {
    W_LOCK(m_Mutex);

    WRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const WRTTI* pRtti = WRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfoPtr = m_WorkerOps.ExpandAndGetRef();
    opInfoPtr = W_DEFAULT_NEW(WorkerOpInfo);

    auto& opInfo = *opInfoPtr;
    opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pWorkerOp = pRtti->GetAllocator()->Allocate<WLongOpWorker>();

    LaunchWorkerOperation(opInfo, reader);
    return;
  }

  if (auto pMsg = WDynamicCast<const WLongOpResultMsg*>(e.m_pMessage))
  {
    W_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      W_ASSERT_DEBUG(pMsg->m_bSuccess == false, "Only Cancel messages are allowed to send to the processor");

      pOpInfo->m_Progress.UserClickedCancel();
    }

    return;
  }
}

void WLongOpWorkerManager::LaunchWorkerOperation(WorkerOpInfo& opInfo, WStreamReader& config)
{
  opInfo.m_Progress.SetCompletion(0.0f);
  opInfo.m_Progress.m_pUserData = &opInfo;
  opInfo.m_Progress.m_Events.AddEventHandler(
    WMakeDelegate(&WLongOpWorkerManager::WorkerProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  SendProgress(opInfo);

  if (opInfo.m_pWorkerOp->InitializeExecution(config, opInfo.m_DocumentGuid).Failed())
  {
    WorkerOperationFinished(opInfo.m_OperationGuid, W_FAILURE, WDataBuffer());
  }
  else
  {
    WSharedPtr<WLongOpTask> pTask = W_DEFAULT_NEW(WLongOpTask);
    pTask->m_OperationGuid = opInfo.m_OperationGuid;
    pTask->m_pWorkerOp = opInfo.m_pWorkerOp.Borrow();
    pTask->m_pProgress = &opInfo.m_Progress;
    opInfo.m_TaskID = WTaskSystem::StartSingleTask(pTask, WTaskPriority::LongRunning);
  }
}

void WLongOpWorkerManager::WorkerOperationFinished(WUuid operationGuid, WResult result, WDataBuffer&& resultData)
{
  W_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(operationGuid);

  if (pOpInfo == nullptr)
    return;

  // tell the controller about the result
  {
    WLongOpResultMsg msg;
    msg.m_OperationGuid = operationGuid;
    msg.m_bSuccess = result.Succeeded();
    msg.m_ResultData = std::move(resultData);

    m_pCommunicationChannel->SendMessage(&msg);
  }

  RemoveOperation(operationGuid);
}

void WLongOpWorkerManager::WorkerProgressBarEventHandler(const WProgressEvent& e)
{
  if (e.m_Type == WProgressEvent::Type::ProgressChanged)
  {
    auto pOpInfo = static_cast<WorkerOpInfo*>(e.m_pProgressbar->m_pUserData);

    SendProgress(*pOpInfo);
  }
}

void WLongOpWorkerManager::RemoveOperation(WUuid opGuid)
{
  W_LOCK(m_Mutex);

  for (WUInt32 i = 0; i < m_WorkerOps.GetCount(); ++i)
  {
    if (m_WorkerOps[i]->m_OperationGuid == opGuid)
    {
      m_WorkerOps.RemoveAtAndSwap(i);
      return;
    }
  }
}

WLongOpWorkerManager::WorkerOpInfo* WLongOpWorkerManager::GetOperation(const WUuid& opGuid) const
{
  W_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_WorkerOps)
  {
    if (opInfoPtr->m_OperationGuid == opGuid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void WLongOpWorkerManager::SendProgress(WorkerOpInfo& opInfo)
{
  WLongOpProgressMsg msg;
  msg.m_OperationGuid = opInfo.m_OperationGuid;
  msg.m_fCompletion = opInfo.m_Progress.GetCompletion();

  m_pCommunicationChannel->SendMessage(&msg);
}
