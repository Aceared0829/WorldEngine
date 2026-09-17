#pragma once

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <atomic>

struct WAssetCuratorEvent;
class WTask;
struct WAssetInfo;

/// Log for all background processing results
class WAssetProcessorLog : public WLogInterface
{
public:
  virtual void HandleLogMessage(const WLoggingEventData& le) override;
  void AddLogWriter(WLoggingEvent::Handler handler);
  void RemoveLogWriter(WLoggingEvent::Handler handler);

  WLoggingEvent m_LoggingEvent;
};

/// Event type used by WAssetProcessor::m_Events
struct WAssetProcessorEvent
{
  enum class Type
  {
    AssetProcessorStateChanged, ///< WAssetProcessor::GetProcessorState changed
    ProcessStateChanged,        ///< WAssetProcessor::GetProcessState changed
  };

  Type m_Type;
  WUInt8 m_uiProcessCount = 0; ///< Total number of processes. Only valid if ProcessorStateChanged.
  WUInt8 m_uiProcessorID = 0;  ///< The changed process index. Only valid if ProcessStateChanged.
};

/// Event type used by WAssetProcessor::m_ProgressEvents
struct WAssetProcessorProgressEvent
{
  enum class Type : WUInt8
  {
    ProcessingStarted, ///< A process started working on an asset
    ProcessingFinished ///< A process finished working on an asset
  };

  Type m_Type;
  WAssetInfo::TransformState m_TransformState = WAssetInfo::Unknown;
  WUInt8 m_uiProcessorID;
  WUuid m_AssetGuid;
  WString m_sAssetPath;
  WTime m_StartTime;
  WTime m_TransformStartTime;
  WTime m_EndTime;
  WTransformStatus m_Result; ///< Only valid when m_Type == ProcessingFinished
};

/// Thread used by WAssetProcessor to schedule work items on the WEditorProcessorProcess instances.
class WAssetProcessorThread : public WThread
{
public:
  WAssetProcessorThread()
    : WThread("WProcessThread")
  {
  }


  virtual WUInt32 Run() override;
};

/// Encapsulates one WEditorProcessor process managed by WAssetProcessor.
class WEditorProcessorProcess
{
public:
  enum class State
  {
    StartClient,          ///< Starting client process.
    WaitingForConnection, /// < Waiting for IPC connection to client process.
    LookingForWork,       ///< Connected. Waiting for work to become available.
    ReadyForProcessing,   ///< A work item has been found. Ready to start processing.
    Processing,           ///< A work item is being processed.
    ReportResult,         ///< Report result of the precessing phase.
    Crashed               ///< Process has crashed. Dead end until `RequestRestart` is called.
  };

public:
  WEditorProcessorProcess();
  ~WEditorProcessorProcess();

  WUInt32 m_uiProcessorID;
  WTime m_ProcessingStartTime;  // When a work item was started.
  WThreadSignal* m_pNewWorkSignal = nullptr;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  /// Called by the worker thread to restart a crashed process.
  void RequestRestart();

  bool IsConnected() const;
  bool IsRunning() const;
  bool IsCrashed() const;
  WOsProcessID GetProcessId() const;
  bool HasProcessCrashed();
  void HandleHashMissmatch();

  WResult StartProcess();
  void ShutdownProcess();

private:
  void EventHandlerIPC(const WProcessCommunicationChannel::Event& e);
  void ChannelEventHandler(const WIpcChannelEvent& e);

  bool GetNextAssetToProcess(WAssetInfo* pInfo, WUuid& out_guid, WDataDirPath& out_path, WAssetInfo::TransformState& out_transformState);
  bool GetNextAssetToProcess(WUuid& out_guid, WDataDirPath& out_path, WAssetInfo::TransformState& out_transformState);
  void OnProcessCrashed(WStringView message);

private:
  State m_State = State::StartClient;
  WEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning = false;
  bool m_bIsIdle = false;
  WOsProcessID m_CurrentProcessID = {};

  // New asset to process
  WUuid m_AssetGuid;
  WDataDirPath m_AssetPath;
  WAssetInfo::TransformState m_TransformState = WAssetInfo::TransformState::Unknown;
  WString m_sPlatform;
  WUInt64 m_uiAssetHash = 0;
  WUInt64 m_uiThumbHash = 0;
  WUInt64 m_uiPackageHash = 0;
  WDynamicArray<WString> m_TransitiveHull;

  // Transform result
  WTransformStatus m_Status;
  WDynamicArray<WLogEntry> m_LogEntries;
  WMap<WString, WUInt64> m_MissmatchTransformDependencies;
  WMap<WString, WUInt64> m_MissmatchThumbnailDependencies;
  WUInt64 m_uiMissmatchAssetHash = 0;
  WUInt64 m_uiMissmatchThumbHash = 0;
  WTime m_StartedProcessing;
  WTime m_StartedTransform;
  WTime m_FinishedProcessing;
};

/// Background asset processing is handled by this class.
/// Creates WEditorProcessor processes which are managed by the WEditorProcessorProcess class.
class W_EDITORFRAMEWORK_DLL WAssetProcessor
{
  W_DECLARE_SINGLETON(WAssetProcessor);

public:
  enum class ProcessorState : WUInt8
  {
    Stopped,  ///< No EditorProcessor or the process thread is running.
    Running,  ///< Everything is active.
    Stopping, ///< Everything is still running but no new tasks are put into the EditorProcessors.
  };

  WAssetProcessor();
  ~WAssetProcessor();

  // used to temporarily not process assets, usually because currently assets get imported
  WAtomicInteger32 m_iPauseProcessing;

  void StartProcessor();
  void StopProcessor(bool bForce);

  /// Returns whether the asset processor is running, stopped or stopping.
  ProcessorState GetProcessorState() const
  {
    return m_ProcessorState;
  }

  /// Returns how many WEditorProcessor processes are managed by the WAssetProcessor.
  WUInt32 GetProcessCount() const;
  /// Returns the state of one of the WEditorProcessor processes.
  /// \param uiProcessIndex The index of the process. Must be smaller than GetProcessCount.
  WEditorProcessorState GetProcessState(WUInt32 uiProcessIndex) const;

  /// Requests a restart of a crashed processor.
  /// This is safe to call from any thread. The restart will be handled by the worker thread.
  /// \param uiProcessIndex The index of the crashed process to restart. Must be smaller than GetProcessCount.
  void RequestRestartProcess(WUInt32 uiProcessIndex);

  void AddLogWriter(WLoggingEvent::Handler handler);
  void RemoveLogWriter(WLoggingEvent::Handler handler);
  void UpdateProcessStates();

public:
  // Can be called from worker threads!
  WCopyOnBroadcastEvent<const WAssetProcessorEvent&, WMutex> m_Events;
  WCopyOnBroadcastEvent<const WAssetProcessorProgressEvent&, WMutex> m_ProgressEvents;

private:
  friend class WEditorProcessorProcess;
  friend class WAssetProcessorThread;
  friend class WAssetCurator;

  void Run();

private:
  WAssetProcessorLog m_CuratorLog;

  // Process thread and its state
  WThreadSignal m_NewWorkSignal;
  WUniquePtr<WAssetProcessorThread> m_pThread;
  std::atomic<bool> m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable WMutex m_ProcessorMutex;
  std::atomic<ProcessorState> m_ProcessorState = ProcessorState::Stopped;
  WDynamicArray<WEditorProcessorState> m_EditorProcessorStates;
  WDynamicArray<WAtomicBool> m_RestartRequests; ///< Set by main thread, read by worker thread

  // Data owned by the process thread.
  WDynamicArray<WEditorProcessorProcess> m_Processes;
};
