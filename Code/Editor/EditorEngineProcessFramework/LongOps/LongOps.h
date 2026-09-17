#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WStringBuilder;
class WStreamWriter;
class WProgress;

//////////////////////////////////////////////////////////////////////////

/// Proxy long ops represent a long operation on the editor side.
///
/// Proxy long ops have little functionality other than naming which WLongOpWorker to execute
/// in the engine process and to feed it with the necessary parameters.
/// Since the proxy long op runs in the editor process, it may access WDocumentObject's
/// and extract data from them.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpProxy : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpProxy, WReflectedClass);

public:
  /// Called once by WLongOpControllerManager::RegisterLongOp() to inform the proxy
  /// to which WDocument and component (WDocumentObject) it is linked.
  virtual void InitializeRegistered(const WUuid& documentGuid, const WUuid& componentGuid) {}

  /// Called by the WQtLongOpsPanel to determine the display string to be shown in the UI.
  virtual const char* GetDisplayName() const = 0;

  /// Called every time the long op shall be executed
  /// \param out_sReplicationOpType must name the WLongOpWorker that shall be executed in the engine process.
  /// \param config can be optionally written to. The data is transmitted to the WLongOpWorker on the other side
  /// and fed to it in WLongOpWorker::InitializeExecution().
  virtual void GetReplicationInfo(WStringBuilder& out_sReplicationOpType, WStreamWriter& inout_config) = 0;

  /// Called once the corresponding WLongOpWorker has finished.
  /// \param result Whether the operation succeeded or failed (e.g. via user cancellation).
  /// \param resultData Optional data written by WLongOpWorker::Execute().
  virtual void Finalize(WResult result, const WDataBuffer& resultData) {}
};

//////////////////////////////////////////////////////////////////////////

/// Worker long ops are executed by the editor engine process.
///
/// They typically do the actual long processing. Since they run in the engine process, they have access
/// to the runtime scene graph and resources but not the editor representation of the scene.
///
/// WLongOpWorker instances are automatically instantiated by WLongOpWorkerManager when they have
/// been named by a WLongOpProxy's GetReplicationInfo() function.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpWorker : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpWorker, WReflectedClass);

public:
  /// Called within the engine processes main thread.
  /// The function may lock the WWorld from the given scene document and extract vital information.
  /// It should try to be as quick as possible and leave the heavy lifting to Execute(), which will run on a background thread.
  /// If this function return failure, the long op is canceled right away.
  virtual WResult InitializeExecution(WStreamReader& ref_config, const WUuid& documentGuid) { return W_SUCCESS; }

  /// Executed in a separete thread after InitializeExecution(). This should do the work that takes a while.
  ///
  /// This function may write the result data directly to disk. Everything that is written to \a proxydata
  /// will be transmitted back to the proxy long op and given to WLongOpProxy::Finalize(). Since this requires IPC bandwidth
  /// the amount of data should be kept very small (a few KB at most).
  ///
  /// All updates to \a progress will be automatically synchronized back to the editor process and become visible through
  /// the WLongOpControllerManager via the WLongOpControllerEvent.
  /// Use WProgressRange for convenient progress updates.
  virtual WResult Execute(WProgress& ref_progress, WStreamWriter& ref_proxydata) = 0;
};
