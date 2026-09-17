#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

W_IMPLEMENT_SINGLETON(WLongOpControllerManager);

WLongOpControllerManager::WLongOpControllerManager()
  : m_SingletonRegistrar(this)
{
}

WLongOpControllerManager::~WLongOpControllerManager() = default;

void WLongOpControllerManager::ProcessCommunicationChannelEventHandler(const WProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = WDynamicCast<const WLongOpProgressMsg*>(e.m_pMessage))
  {
    W_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_fCompletion = pMsg->m_fCompletion;

      BroadcastProgress(*pOpInfo);
    }

    return;
  }

  if (auto pMsg = WDynamicCast<const WLongOpResultMsg*>(e.m_pMessage))
  {
    W_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_bIsRunning = false;
      pOpInfo->m_StartOrDuration = WTime::Now() - pOpInfo->m_StartOrDuration;
      pOpInfo->m_fCompletion = 0.0f;

      pOpInfo->m_pProxyOp->Finalize(pMsg->m_bSuccess ? W_SUCCESS : W_FAILURE, pMsg->m_ResultData);

      // TODO: show success/failure in UI
      BroadcastProgress(*pOpInfo);
    }
  }
}

void WLongOpControllerManager::StartOperation(WUuid opGuid)
{
  W_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || pOpInfo->m_bIsRunning)
    return;

  pOpInfo->m_StartOrDuration = WTime::Now();
  pOpInfo->m_bIsRunning = true;

  ReplicateToWorkerProcess(*pOpInfo);

  BroadcastProgress(*pOpInfo);
}

void WLongOpControllerManager::CancelOperation(WUuid opGuid)
{
  W_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || !pOpInfo->m_bIsRunning)
    return;

  // send a cancel message to the processor
  WLongOpResultMsg msg;
  msg.m_OperationGuid = opGuid;
  msg.m_bSuccess = false;
  m_pCommunicationChannel->SendMessage(&msg);
}

void WLongOpControllerManager::RemoveOperation(WUuid opGuid)
{
  W_LOCK(m_Mutex);

  for (WUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    if (m_ProxyOps[i]->m_OperationGuid == opGuid)
    {
      m_ProxyOps.RemoveAtAndCopy(i);

      // broadcast the removal to the UI
      {
        WLongOpControllerEvent e;
        e.m_Type = WLongOpControllerEvent::Type::OpRemoved;
        e.m_OperationGuid = opGuid;

        m_Events.Broadcast(e);
      }

      return;
    }
  }
}

void WLongOpControllerManager::RegisterLongOp(const WUuid& documentGuid, const WUuid& componentGuid, const char* szLongOpType)
{
  const WRTTI* pRtti = WRTTI::FindTypeByName(szLongOpType);
  if (pRtti == nullptr)
  {
    WLog::Error("Can't register long op of unknown type '{}'", szLongOpType);
    return;
  }

  auto& opInfoPtr = m_ProxyOps.ExpandAndGetRef();
  opInfoPtr = W_DEFAULT_NEW(ProxyOpInfo);

  auto& opInfo = *opInfoPtr;
  opInfo.m_DocumentGuid = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_OperationGuid = WUuid::MakeUuid();

  opInfo.m_pProxyOp = pRtti->GetAllocator()->Allocate<WLongOpProxy>();
  opInfo.m_pProxyOp->InitializeRegistered(documentGuid, componentGuid);

  WLongOpControllerEvent e;
  e.m_Type = WLongOpControllerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void WLongOpControllerManager::UnregisterLongOp(const WUuid& documentGuid, const WUuid& componentGuid, const char* szLongOpType)
{
  for (WUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    auto& opInfoPtr = m_ProxyOps[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_DocumentGuid == documentGuid &&
        opInfoPtr->m_pProxyOp->GetDynamicRTTI()->GetTypeName() == szLongOpType)
    {
      RemoveOperation(opInfoPtr->m_OperationGuid);
      return;
    }
  }
}

WLongOpControllerManager::ProxyOpInfo* WLongOpControllerManager::GetOperation(const WUuid& opGuid)
{
  W_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_ProxyOps)
  {
    if (opInfoPtr->m_OperationGuid == opGuid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void WLongOpControllerManager::CancelAndRemoveAllOpsForDocument(const WUuid& documentGuid)
{
  {
    W_LOCK(m_Mutex);

    for (auto& opInfoPtr : m_ProxyOps)
    {
      CancelOperation(opInfoPtr->m_OperationGuid);
    }
  }

  bool bOperationsStillActive = true;

  while (bOperationsStillActive)
  {
    bOperationsStillActive = false;
    m_pCommunicationChannel->ProcessMessages();

    {
      W_LOCK(m_Mutex);

      for (WUInt32 i0 = m_ProxyOps.GetCount(); i0 > 0; --i0)
      {
        const WUInt32 i = i0 - 1;

        auto& op = m_ProxyOps[i];
        if (op->m_DocumentGuid == documentGuid)
        {
          if (op->m_bIsRunning)
          {
            bOperationsStillActive = true;
            break;
          }

          WLongOpControllerEvent e;
          e.m_Type = WLongOpControllerEvent::Type::OpRemoved;
          e.m_OperationGuid = m_ProxyOps[i]->m_OperationGuid;

          m_ProxyOps.RemoveAtAndCopy(i);

          m_Events.Broadcast(e);
        }
      }
    }

    if (bOperationsStillActive)
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    }
  }
}

void WLongOpControllerManager::ReplicateToWorkerProcess(ProxyOpInfo& opInfo)
{
  W_LOCK(m_Mutex);

  // send the replication message
  {
    WLongOpReplicationMsg msg;

    WMemoryStreamContainerWrapperStorage<WDataBuffer> storage(&msg.m_ReplicationData);
    WMemoryStreamWriter writer(&storage);

    WStringBuilder replType;
    opInfo.m_pProxyOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
    msg.m_OperationGuid = opInfo.m_OperationGuid;

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

void WLongOpControllerManager::BroadcastProgress(ProxyOpInfo& opInfo)
{
  // as controller, broadcast progress to the UI
  WLongOpControllerEvent e;
  e.m_Type = WLongOpControllerEvent::Type::OpProgress;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}


// void WLongOpManager::AddLongOperation(WUniquePtr<WLongOp>&& pOperation, const WUuid& documentGuid)
//{
//  W_LOCK(m_Mutex);
//
//  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
//  opInfoPtr = W_DEFAULT_NEW(LongOpInfo);
//
//  auto& opInfo = *opInfoPtr;
//  opInfo.m_pOperation = std::move(pOperation);
//  opInfo.m_OperationGuid.CreateNewUuid();
//  opInfo.m_DocumentGuid = documentGuid;
//  opInfo.m_StartOrDuration = WTime::Now();
//  opInfo.m_Progress.m_pUserData = opInfo.m_pOperation.Borrow();
//  opInfo.m_Progress.m_Events.AddEventHandler(
//    WMakeDelegate(&WLongOpManager::ProgressBarEventHandler, this), opInfo.m_ProgressSubscription);
//
//  WLongOp* pNewOp = opInfo.m_pOperation.Borrow();
//
//  if (m_Mode == Mode::Processor || WDynamicCast<WLongOpProxy*>(pNewOp) != nullptr)
//  {
//    WStringBuilder replType;
//
//    WLongOpReplicationMsg msg;
//
//    WMemoryStreamContainerWrapperStorage<WDataBuffer> storage(&msg.m_ReplicationData);
//    WMemoryStreamWriter writer(&storage);
//
//    pNewOp->GetReplicationInfo(replType, writer);
//
//    msg.m_sReplicationType = replType;
//    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
//    msg.m_OperationGuid = opInfo.m_OperationGuid;
//    msg.m_sDisplayName = pNewOp->GetDisplayName();
//
//    m_pCommunicationChannel->SendMessage(&msg);
//  }
//
//  LaunchWorkerOperation(opInfo);
//
//  {
//    WLongOpManagerEvent e;
//    e.m_Type = WLongOpManagerEvent::Type::OpAdded;
//    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
//    m_Events.Broadcast(e);
//  }
//}
