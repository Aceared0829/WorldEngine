#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

///////////////////////////////////// WProcessMessages /////////////////////////////////////



///////////////////////////////////// WEditorEngineMsg /////////////////////////////////////

/// Base class for all messages between editor and engine that are not bound to any document
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineMsg, WProcessMessage);

public:
  WEditorEngineMsg() = default;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WUpdateReflectionTypeMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WUpdateReflectionTypeMsgToEditor, WEditorEngineMsg);

public:
  // Mutable because it is eaten up by WPhantomRttiManager.
  mutable WReflectedTypeDescriptor m_desc;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSetupProjectMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSetupProjectMsgToEngine, WEditorEngineMsg);

public:
  WString m_sProjectDir;
  WApplicationFileSystemConfig m_FileSystemConfig;
  WApplicationPluginConfig m_PluginConfig;
  WString m_sFileserveAddress; ///< Optionally used for remote processes to tell them with which IP address to connect to the host
  WString m_sAssetProfile;
  float m_fDevicePixelRatio = 1.0f;
};

/// Sent to remote processes to shut them down.
/// Local processes are simply killed through QProcess::close, but remote processes have to close themselves.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WShutdownProcessMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WShutdownProcessMsgToEngine, WEditorEngineMsg);

public:
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WProjectReadyMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WProjectReadyMsgToEditor, WEditorEngineMsg);

public:
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSimpleConfigMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSimpleConfigMsgToEngine, WEditorEngineMsg);

public:
  WString m_sWhatToDo;
  WString m_sPayload;
  double m_fPayload;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSaveProfilingResponseToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSaveProfilingResponseToEditor, WEditorEngineMsg);

public:
  WString m_sProfilingFile;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WReloadResourceMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WReloadResourceMsgToEngine, WEditorEngineMsg);

public:
  WString m_sResourceType;
  WString m_sResourceID;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WResourceUpdateMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WResourceUpdateMsgToEngine, WEditorEngineMsg);

public:
  WString m_sResourceType;
  WString m_sResourceID;
  WDataBuffer m_Data;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WRestoreResourceMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WRestoreResourceMsgToEngine, WEditorEngineMsg);

public:
  WString m_sResourceType;
  WString m_sResourceID;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WChangeCVarMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WChangeCVarMsgToEngine, WEditorEngineMsg);

public:
  WString m_sCVarName;
  WVariant m_NewValue;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WConsoleCmdMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WConsoleCmdMsgToEngine, WEditorEngineMsg);

public:
  WInt8 m_iType; // 0 = execute, 1 = auto complete
  WString m_sCommand;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WConsoleCmdResultMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WConsoleCmdResultMsgToEditor, WEditorEngineMsg);

public:
  WString m_sResult;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WDynamicStringEnumMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicStringEnumMsgToEditor, WEditorEngineMsg);

public:
  WString m_sEnumName;
  WHybridArray<WString, 8> m_EnumValues;
};

///////////////////////////////////// WEditorEngineDocumentMsg /////////////////////////////////////

/// Base class for all messages that are tied to some document.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineDocumentMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineDocumentMsg, WProcessMessage);

public:
  WUuid m_DocumentGuid;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSimpleDocumentConfigMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSimpleDocumentConfigMsgToEngine, WEditorEngineDocumentMsg);

public:
  WString m_sWhatToDo;
  WString m_sPayload;
  WVariant m_PayloadValue;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSimpleDocumentConfigMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSimpleDocumentConfigMsgToEditor, WEditorEngineDocumentMsg);

public:
  WString m_sWhatToDo;
  WString m_sPayload;
  WVariant m_PayloadValue;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSyncWithProcessMsgToEngine : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WSyncWithProcessMsgToEngine, WProcessMessage);

public:
  WUInt32 m_uiRedrawCount;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSyncWithProcessMsgToEditor : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WSyncWithProcessMsgToEditor, WProcessMessage);

public:
  WUInt32 m_uiRedrawCount;
};


class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineViewMsg : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineViewMsg, WEditorEngineDocumentMsg);

public:
  WEditorEngineViewMsg() { m_uiViewID = 0xFFFFFFFF; }

  WUInt32 m_uiViewID;
};

/// For very simple uses cases where a custom message would be too much
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WDocumentConfigMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentConfigMsgToEngine, WEditorEngineDocumentMsg);

public:
  WString m_sWhatToDo;
  int m_iValue;
  float m_fValue;
  WString m_sValue;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WDocumentOpenMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentOpenMsgToEngine, WEditorEngineDocumentMsg);

public:
  WDocumentOpenMsgToEngine() { m_bDocumentOpen = false; }

  bool m_bDocumentOpen;
  WString m_sDocumentType;
  WVariant m_DocumentMetaData;
};

/// Used to reset the engine side to an empty document before sending the full document state over
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WDocumentClearMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentClearMsgToEngine, WEditorEngineDocumentMsg);

public:
  WDocumentClearMsgToEngine() = default;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WDocumentOpenResponseMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentOpenResponseMsgToEditor, WEditorEngineDocumentMsg);

public:
  WDocumentOpenResponseMsgToEditor() = default;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewDestroyedMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewDestroyedMsgToEngine, WEditorEngineViewMsg);
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewDestroyedResponseMsgToEditor : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewDestroyedResponseMsgToEditor, WEditorEngineViewMsg);
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewRedrawMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewRedrawMsgToEngine, WEditorEngineViewMsg);

public:
  WUInt64 m_uiHWND;
  WUInt16 m_uiWindowWidth;
  WUInt16 m_uiWindowHeight;
  bool m_bUpdatePickingData;
  bool m_bEnablePickingSelected;
  bool m_bEnablePickTransparent;
  bool m_bUseCameraTransformOnDevice = true;

  WInt8 m_iCameraMode; ///< WCameraMode::Enum
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fFovOrDim;
  WUInt8 m_uiRenderMode; ///< WViewRenderMode::Enum

  WVec3 m_vPosition;
  WVec3 m_vDirForwards;
  WVec3 m_vDirUp;
  WVec3 m_vDirRight;
  WMat4 m_ViewMatrix;
  WMat4 m_ProjMatrix;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewScreenshotMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewScreenshotMsgToEngine, WEditorEngineViewMsg);

public:
  WString m_sOutputFile;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WActivateRemoteViewMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WActivateRemoteViewMsgToEngine, WEditorEngineViewMsg);

public:
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEntityMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEntityMsgToEngine, WEditorEngineDocumentMsg);

public:
  WObjectChange m_change;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WExportDocumentMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WExportDocumentMsgToEngine, WEditorEngineDocumentMsg);

public:
  WExportDocumentMsgToEngine() = default;

  WString m_sOutputFile;
  WUInt64 m_uiAssetHash = 0;
  WUInt16 m_uiVersion = 0;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WExportDocumentMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WExportDocumentMsgToEditor, WEditorEngineDocumentMsg);

public:
  bool m_bOutputSuccess = false;
  WString m_sFailureMsg;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WCreateThumbnailMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WCreateThumbnailMsgToEngine, WEditorEngineDocumentMsg);

public:
  WUInt16 m_uiWidth = 0;
  WUInt16 m_uiHeight = 0;
  WHybridArray<WString, 1> m_ViewExcludeTags;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WCreateThumbnailMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WCreateThumbnailMsgToEditor, WEditorEngineDocumentMsg);

public:
  WCreateThumbnailMsgToEditor() = default;
  WDataBuffer m_ThumbnailData; ///< Raw 8-bit RGBA data (256x256x4 bytes)
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewPickingMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewPickingMsgToEngine, WEditorEngineViewMsg);

public:
  WUInt16 m_uiPickPosX;
  WUInt16 m_uiPickPosY;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewPickingResultMsgToEditor : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewPickingResultMsgToEditor, WEditorEngineViewMsg);

public:
  WUuid m_ObjectGuid;
  WUuid m_ComponentGuid;
  WUuid m_OtherGuid;
  WUInt32 m_uiPartIndex;

  WVec3 m_vPickedPosition;
  WVec3 m_vPickedNormal;
  WVec3 m_vPickingRayStartPosition;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewMarqueePickingMsgToEngine : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewMarqueePickingMsgToEngine, WEditorEngineViewMsg);

public:
  WUInt16 m_uiPickPosX0;
  WUInt16 m_uiPickPosY0;

  WUInt16 m_uiPickPosX1;
  WUInt16 m_uiPickPosY1;

  WUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  WUInt32 m_uiActionIdentifier;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewMarqueePickingResultMsgToEditor : public WEditorEngineViewMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewMarqueePickingResultMsgToEditor, WEditorEngineViewMsg);

public:
  WDynamicArray<WUuid> m_ObjectGuids;
  WUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  WUInt32 m_uiActionIdentifier;
};


class WEditorEngineConnection;

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WViewHighlightMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WViewHighlightMsgToEngine, WEditorEngineDocumentMsg);

public:
  WUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLogMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WLogMsgToEditor, WEditorEngineMsg);

public:
  WLogEntry m_Entry;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WCVarMsgToEditor : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WCVarMsgToEditor, WEditorEngineMsg);

public:
  WString m_sName;
  WString m_sPlugin;
  WString m_sDescription;
  WVariant m_Value;
};


class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpReplicationMsg : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpReplicationMsg, WEditorEngineMsg);

public:
  WUuid m_OperationGuid;
  WUuid m_DocumentGuid;
  WString m_sReplicationType;
  WDataBuffer m_ReplicationData;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpProgressMsg : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpProgressMsg, WEditorEngineMsg);

public:
  WUuid m_OperationGuid;
  float m_fCompletion;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WLongOpResultMsg : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WLongOpResultMsg, WEditorEngineMsg);

public:
  WUuid m_OperationGuid;
  bool m_bSuccess;
  WDataBuffer m_ResultData;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEditorEngineSyncObjectMsg : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineSyncObjectMsg, WEditorEngineDocumentMsg);

public:
  WUuid m_ObjectGuid;
  WString m_sObjectType;
  WDataBuffer m_ObjectData;

  const WDataBuffer& GetObjectData() const { return m_ObjectData; }
  void SetObjectData(const WDataBuffer& s) { m_ObjectData = s; }
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WObjectTagMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WObjectTagMsgToEngine, WEditorEngineDocumentMsg);

public:
  WObjectTagMsgToEngine()
  {
    m_bSetTag = false;
    m_bApplyOnAllChildren = false;
  }

  WUuid m_ObjectGuid;
  WString m_sTag;
  bool m_bSetTag;
  bool m_bApplyOnAllChildren;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WObjectSelectionMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WObjectSelectionMsgToEngine, WEditorEngineDocumentMsg);

public:
  WString m_sSelection;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSimulationSettingsMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSimulationSettingsMsgToEngine, WEditorEngineDocumentMsg);

public:
  bool m_bSimulateWorld = false;
  float m_fSimulationSpeed = 1.0f;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGridSettingsMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WGridSettingsMsgToEngine, WEditorEngineDocumentMsg);

public:
  float m_fGridDensity = 0.0f;
  WVec3 m_vGridCenter;
  WVec3 m_vGridTangent1;
  WVec3 m_vGridTangent2;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGlobalSettingsMsgToEngine : public WEditorEngineMsg
{
  W_ADD_DYNAMIC_REFLECTION(WGlobalSettingsMsgToEngine, WEditorEngineMsg);

public:
  float m_fGizmoScale = 0.0f;
  float m_fShapeIconScale = 0.0f;
  float m_fShapeIconFadeDistance = 0.0f;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WWorldSettingsMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WWorldSettingsMsgToEngine, WEditorEngineDocumentMsg);

public:
  bool m_bRenderOverlay = false;
  bool m_bRenderShapeIcons = false;
  bool m_bRenderSelectionBoxes = false;
  bool m_bAddAmbientLight = false;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGameModeMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WGameModeMsgToEngine, WEditorEngineDocumentMsg);

public:
  bool m_bEnablePTG = false;
  bool m_bUseStartPosition = false;
  WVec3 m_vStartPosition;
  WVec3 m_vStartDirection;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGameModeMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WGameModeMsgToEditor, WEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WQuerySelectionBBoxMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WQuerySelectionBBoxMsgToEngine, WEditorEngineDocumentMsg);

public:
  WUInt32 m_uiViewID; /// passed through to WQuerySelectionBBoxResultMsgToEditor
  WInt32 m_iPurpose;  /// passed through to WQuerySelectionBBoxResultMsgToEditor
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WQuerySelectionBBoxResultMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WQuerySelectionBBoxResultMsgToEditor, WEditorEngineDocumentMsg);

public:
  WVec3 m_vCenter;
  WVec3 m_vHalfExtents;

  WUInt32 m_uiViewID; /// passed through from WQuerySelectionBBoxMsgToEngine
  WInt32 m_iPurpose;  /// passed through from WQuerySelectionBBoxMsgToEngine
};

/// Send between editor documents, such that one document can know about objects in another document.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGatherObjectsOfTypeMsgInterDoc : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WGatherObjectsOfTypeMsgInterDoc, WReflectedClass);

public:
  const WRTTI* m_pType;

  struct Result
  {
    const WDocument* m_pDocument;
    WUuid m_ObjectGuid;
    WString m_sDisplayName;
  };

  WDynamicArray<Result> m_Results;
};

/// Send by the editor scene document to all other editor documents, to gather on which objects debug visualization should be enabled during
/// play-the-game.
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGatherObjectsForDebugVisMsgInterDoc : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WGatherObjectsForDebugVisMsgInterDoc, WReflectedClass);

public:
  WDynamicArray<WUuid> m_Objects;
};

/// Send by the editor scene document to the runtime scene document, to tell it about the poll results (see WGatherObjectsForDebugVisMsgInterDoc).
class W_EDITORENGINEPROCESSFRAMEWORK_DLL WObjectsForDebugVisMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WObjectsForDebugVisMsgToEngine, WEditorEngineDocumentMsg);

public:
  WDataBuffer m_Objects;
};
