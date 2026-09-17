#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

class WLongOpProxy;

/// Events about all known long ops. Broadcast by WLongOpControllerManager.
struct WLongOpControllerEvent
{
  enum class Type
  {
    OpAdded,    ///< A new long op has been added / registered.
    OpRemoved,  ///< A long op has been deleted. The GUID is sent, but it cannot be resolved anymore.
    OpProgress, ///< The completion progress of a long op has changed.
  };

  Type m_Type;
  WUuid m_OperationGuid; ///< Use WLongOpControllerManager::GetOperation() to resolve the GUID to the actual long op.
};

/// The LongOp controller is active in the editor process and manages which long ops are available, running, etc.
///
/// All available long ops are registered with the controller, typically automatically by the WLongOpsAdapter,
/// although it is theoretically possible to register additional long ops.
///
/// Through the controller long ops can be started or canceled, which is exposed in the UI by the WQtLongOpsPanel.
///
/// Through the broadcast WLongOpControllerEvent, one can track the state of all long ops.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpControllerManager final : public WLongOpManager
{
  W_DECLARE_SINGLETON(WLongOpControllerManager);

public:
  WLongOpControllerManager();
  ~WLongOpControllerManager();

  /// Holds all information about the proxy long op on the editor side
  struct ProxyOpInfo
  {
    WUniquePtr<WLongOpProxy> m_pProxyOp;
    WUuid m_OperationGuid;     ///< Identifies the operation itself.
    WUuid m_DocumentGuid;      ///< To which document the long op belongs. When the document is closed, all running long ops belonging to it
                                ///< will be canceled.
    WUuid m_ComponentGuid;     ///< To which component in the scene document the long op is linked. If the component is deleted, the long op
                                ///< disappears as well.
    WTime m_StartOrDuration;   ///< While m_bIsRunning is true, this is the time the long op started, once m_bIsRunning it holds the last
                                ///< duration of the long op execution.
    float m_fCompletion = 0.0f; ///< [0; 1] range for the progress.
    bool m_bIsRunning = false;  ///< Whether the long op is currently being executed.
  };

  /// Events about the state of all available long ops.
  WEvent<const WLongOpControllerEvent&> m_Events;

  /// Typically called by WLongOpsAdapter when a component that has an WLongOpAttribute is added to a scene
  void RegisterLongOp(const WUuid& documentGuid, const WUuid& componentGuid, const char* szLongOpType);

  /// Typically called by WLongOpsAdapter when a component that has an WLongOpAttribute is removed from a scene
  void UnregisterLongOp(const WUuid& documentGuid, const WUuid& componentGuid, const char* szLongOpType);

  /// Starts executing the given long op. Typically called by the WQtLongOpsPanel.
  void StartOperation(WUuid opGuid);

  /// Cancels a given long op. Typically called by the WQtLongOpsPanel.
  void CancelOperation(WUuid opGuid);

  /// Cancels and deletes all operations linked to the given document. Makes sure to wait for all canceled ops.
  /// Typically called by the WLongOpsAdapter when a document is about to be closed.
  void CancelAndRemoveAllOpsForDocument(const WUuid& documentGuid);

  /// Returns a pointer to the given long op, or null if the GUID does not exist.
  ProxyOpInfo* GetOperation(const WUuid& opGuid);

  /// Gives access to all currently available long ops. Make sure the lock m_Mutex (of the WLongOpManager base class) while accessing this.
  const WDynamicArray<WUniquePtr<ProxyOpInfo>>& GetOperations() const { return m_ProxyOps; }

private:
  virtual void ProcessCommunicationChannelEventHandler(const WProcessCommunicationChannel::Event& e) override;

  void ReplicateToWorkerProcess(ProxyOpInfo& opInfo);
  void BroadcastProgress(ProxyOpInfo& opInfo);
  void RemoveOperation(WUuid opGuid);

  WDynamicArray<WUniquePtr<ProxyOpInfo>> m_ProxyOps;
};
