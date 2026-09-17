#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Utilities/Progress.h>

class WLongOpWorker;
struct WProgressEvent;
using WDataBuffer = WDynamicArray<WUInt8>;

/// The LongOp worker manager is active in the engine process of the editor.
///
/// This class has no public functionality, it communicates with the WLongOpControllerManager
/// and executes the WLongOpWorker's that are named by the respective WLongOpProxy's.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpWorkerManager final : public WLongOpManager
{
  W_DECLARE_SINGLETON(WLongOpWorkerManager);

public:
  WLongOpWorkerManager();
  ~WLongOpWorkerManager();

private:
  friend class WLongOpTask;

  struct WorkerOpInfo
  {
    WUniquePtr<WLongOpWorker> m_pWorkerOp;
    WTaskGroupID m_TaskID;
    WUuid m_DocumentGuid;
    WUuid m_OperationGuid;
    WProgress m_Progress;
    WEvent<const WProgressEvent&>::Unsubscriber m_ProgressSubscription;
  };

  virtual void ProcessCommunicationChannelEventHandler(const WProcessCommunicationChannel::Event& e) override;
  WorkerOpInfo* GetOperation(const WUuid& opGuid) const;
  void LaunchWorkerOperation(WorkerOpInfo& opInfo, WStreamReader& config);
  void WorkerProgressBarEventHandler(const WProgressEvent& e);
  void RemoveOperation(WUuid opGuid);
  void SendProgress(WorkerOpInfo& opInfo);
  void WorkerOperationFinished(WUuid operationGuid, WResult result, WDataBuffer&& resultData);

  WDynamicArray<WUniquePtr<WorkerOpInfo>> m_WorkerOps;
};
