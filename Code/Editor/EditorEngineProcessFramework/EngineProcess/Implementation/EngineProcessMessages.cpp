#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

// clang-format off

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSyncWithProcessMsgToEngine, 1, WRTTIDefaultAllocator<WSyncWithProcessMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSyncWithProcessMsgToEditor, 1, WRTTIDefaultAllocator<WSyncWithProcessMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// WEditorEngineMsg /////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineMsg, 1, WRTTINoAllocator )
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WUpdateReflectionTypeMsgToEditor, 1, WRTTIDefaultAllocator<WUpdateReflectionTypeMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Descriptor", m_desc),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSetupProjectMsgToEngine, 1, WRTTIDefaultAllocator<WSetupProjectMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    W_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig),
    W_MEMBER_PROPERTY("PluginConfig", m_PluginConfig),
    W_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress),
    W_MEMBER_PROPERTY("Platform", m_sAssetProfile),
    W_MEMBER_PROPERTY("DevicePixelRatio", m_fDevicePixelRatio),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WShutdownProcessMsgToEngine, 1, WRTTIDefaultAllocator<WShutdownProcessMsgToEngine>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProjectReadyMsgToEditor, 1, WRTTIDefaultAllocator<WProjectReadyMsgToEditor> )
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimpleConfigMsgToEngine, 1, WRTTIDefaultAllocator<WSimpleConfigMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    W_MEMBER_PROPERTY("Payload", m_sPayload),
    W_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSaveProfilingResponseToEditor, 1, WRTTIDefaultAllocator<WSaveProfilingResponseToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WReloadResourceMsgToEngine, 1, WRTTIDefaultAllocator<WReloadResourceMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sResourceType),
    W_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WResourceUpdateMsgToEngine, 1, WRTTIDefaultAllocator<WResourceUpdateMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sResourceType),
    W_MEMBER_PROPERTY("ID", m_sResourceID),
    W_MEMBER_PROPERTY("Data", m_Data),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRestoreResourceMsgToEngine, 1, WRTTIDefaultAllocator<WRestoreResourceMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sResourceType),
    W_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WChangeCVarMsgToEngine, 1, WRTTIDefaultAllocator<WChangeCVarMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sCVarName),
    W_MEMBER_PROPERTY("Value", m_NewValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConsoleCmdMsgToEngine, 1, WRTTIDefaultAllocator<WConsoleCmdMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_iType),
    W_MEMBER_PROPERTY("Cmd", m_sCommand),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WConsoleCmdResultMsgToEditor, 1, WRTTIDefaultAllocator<WConsoleCmdResultMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Result", m_sResult),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicStringEnumMsgToEditor, 1, WRTTIDefaultAllocator<WDynamicStringEnumMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EnumName", m_sEnumName),
    W_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpReplicationMsg, 1, WRTTIDefaultAllocator<WLongOpReplicationMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    W_MEMBER_PROPERTY("DocGuid", m_DocumentGuid),
    W_MEMBER_PROPERTY("Type", m_sReplicationType),
    W_MEMBER_PROPERTY("Data", m_ReplicationData),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpProgressMsg, 1, WRTTIDefaultAllocator<WLongOpProgressMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    W_MEMBER_PROPERTY("Completion", m_fCompletion),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLongOpResultMsg, 1, WRTTIDefaultAllocator<WLongOpResultMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    W_MEMBER_PROPERTY("Success", m_bSuccess),
    W_MEMBER_PROPERTY("Data", m_ResultData),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// WEditorEngineDocumentMsg /////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineDocumentMsg, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentConfigMsgToEngine, 1, WRTTIDefaultAllocator<WDocumentConfigMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    W_MEMBER_PROPERTY("Int", m_iValue),
    W_MEMBER_PROPERTY("Float", m_fValue),
    W_MEMBER_PROPERTY("String", m_sValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineViewMsg, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ViewID", m_uiViewID),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentOpenMsgToEngine, 1, WRTTIDefaultAllocator<WDocumentOpenMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
    W_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    W_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentClearMsgToEngine, 1, WRTTIDefaultAllocator<WDocumentClearMsgToEngine>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDocumentOpenResponseMsgToEditor, 1, WRTTIDefaultAllocator<WDocumentOpenResponseMsgToEditor> )
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewDestroyedMsgToEngine, 1, WRTTIDefaultAllocator<WViewDestroyedMsgToEngine>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewDestroyedResponseMsgToEditor, 1, WRTTIDefaultAllocator<WViewDestroyedResponseMsgToEditor>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewRedrawMsgToEngine, 1, WRTTIDefaultAllocator<WViewRedrawMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("HWND", m_uiHWND),
    W_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    W_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
    W_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData),
    W_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected),
    W_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent),
    W_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice),
    W_MEMBER_PROPERTY("CameraMode", m_iCameraMode),
    W_MEMBER_PROPERTY("NearPlane", m_fNearPlane),
    W_MEMBER_PROPERTY("FarPlane", m_fFarPlane),
    W_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim),
    W_MEMBER_PROPERTY("Position", m_vPosition),
    W_MEMBER_PROPERTY("Forwards", m_vDirForwards),
    W_MEMBER_PROPERTY("Up", m_vDirUp),
    W_MEMBER_PROPERTY("Right", m_vDirRight),
    W_MEMBER_PROPERTY("ViewMat", m_ViewMatrix),
    W_MEMBER_PROPERTY("ProjMat", m_ProjMatrix),
    W_MEMBER_PROPERTY("RenderMode", m_uiRenderMode),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewScreenshotMsgToEngine, 1, WRTTIDefaultAllocator<WViewScreenshotMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("File", m_sOutputFile)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WActivateRemoteViewMsgToEngine, 1, WRTTIDefaultAllocator<WActivateRemoteViewMsgToEngine>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEntityMsgToEngine, 1, WRTTIDefaultAllocator<WEntityMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Change", m_change),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimpleDocumentConfigMsgToEngine, 1, WRTTIDefaultAllocator<WSimpleDocumentConfigMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    W_MEMBER_PROPERTY("Payload1", m_sPayload),
    W_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimpleDocumentConfigMsgToEditor, 1, WRTTIDefaultAllocator<WSimpleDocumentConfigMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    W_MEMBER_PROPERTY("Payload1", m_sPayload),
    W_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExportDocumentMsgToEngine, 1, WRTTIDefaultAllocator<WExportDocumentMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OutputFile", m_sOutputFile),
    W_MEMBER_PROPERTY("AssetHash", m_uiAssetHash),
    W_MEMBER_PROPERTY("AssetVersion", m_uiVersion),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExportDocumentMsgToEditor, 1, WRTTIDefaultAllocator<WExportDocumentMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess),
    W_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCreateThumbnailMsgToEngine, 1, WRTTIDefaultAllocator<WCreateThumbnailMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Width", m_uiWidth),
    W_MEMBER_PROPERTY("Height", m_uiHeight),
    W_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCreateThumbnailMsgToEditor, 1, WRTTIDefaultAllocator<WCreateThumbnailMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewPickingMsgToEngine, 1, WRTTIDefaultAllocator<WViewPickingMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    W_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewPickingResultMsgToEditor, 1, WRTTIDefaultAllocator<WViewPickingResultMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    W_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    W_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    W_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    W_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    W_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    W_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewMarqueePickingMsgToEngine, 1, WRTTIDefaultAllocator<WViewMarqueePickingMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0),
    W_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0),
    W_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1),
    W_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1),
    W_MEMBER_PROPERTY("what", m_uiWhatToDo),
    W_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewMarqueePickingResultMsgToEditor, 1, WRTTIDefaultAllocator<WViewMarqueePickingResultMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids),
    W_MEMBER_PROPERTY("what", m_uiWhatToDo),
    W_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WViewHighlightMsgToEngine, 1, WRTTIDefaultAllocator<WViewHighlightMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogMsgToEditor, 1, WRTTIDefaultAllocator<WLogMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Entry", m_Entry),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCVarMsgToEditor, 1, WRTTIDefaultAllocator<WCVarMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Plugin", m_sPlugin),
    W_MEMBER_PROPERTY("Desc", m_sDescription),
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineSyncObjectMsg, 1, WRTTIDefaultAllocator<WEditorEngineSyncObjectMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    W_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    W_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectTagMsgToEngine, 1, WRTTIDefaultAllocator<WObjectTagMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    W_MEMBER_PROPERTY("Tag", m_sTag),
    W_MEMBER_PROPERTY("Set", m_bSetTag),
    W_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectSelectionMsgToEngine, 1, WRTTIDefaultAllocator<WObjectSelectionMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Selection", m_sSelection),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSimulationSettingsMsgToEngine, 1, WRTTIDefaultAllocator<WSimulationSettingsMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld),
    W_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGridSettingsMsgToEngine, 1, WRTTIDefaultAllocator<WGridSettingsMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GridDensity", m_fGridDensity),
    W_MEMBER_PROPERTY("GridCenter", m_vGridCenter),
    W_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1),
    W_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGlobalSettingsMsgToEngine, 1, WRTTIDefaultAllocator<WGlobalSettingsMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale),
    W_MEMBER_PROPERTY("ShapeIconScale", m_fShapeIconScale),
    W_MEMBER_PROPERTY("ShapeIconFadeDistance", m_fShapeIconFadeDistance),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WWorldSettingsMsgToEngine, 1, WRTTIDefaultAllocator<WWorldSettingsMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay),
    W_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons),
    W_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes),
    W_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameModeMsgToEngine, 1, WRTTIDefaultAllocator<WGameModeMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Run", m_bEnablePTG),
    W_MEMBER_PROPERTY("UsePos", m_bUseStartPosition),
    W_MEMBER_PROPERTY("Pos", m_vStartPosition),
    W_MEMBER_PROPERTY("Dir", m_vStartDirection),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameModeMsgToEditor, 1, WRTTIDefaultAllocator<WGameModeMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Run", m_bRunningPTG),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WQuerySelectionBBoxMsgToEngine, 1, WRTTIDefaultAllocator<WQuerySelectionBBoxMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ViewID", m_uiViewID),
    W_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WQuerySelectionBBoxResultMsgToEditor, 1, WRTTIDefaultAllocator<WQuerySelectionBBoxResultMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Center", m_vCenter),
    W_MEMBER_PROPERTY("Extents", m_vHalfExtents),
    W_MEMBER_PROPERTY("ViewID", m_uiViewID),
    W_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGatherObjectsOfTypeMsgInterDoc, 1, WRTTIDefaultAllocator<WGatherObjectsOfTypeMsgInterDoc>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGatherObjectsForDebugVisMsgInterDoc, 1, WRTTIDefaultAllocator<WGatherObjectsForDebugVisMsgInterDoc>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WObjectsForDebugVisMsgToEngine, 1, WRTTIDefaultAllocator<WObjectsForDebugVisMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Objects", m_Objects),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

