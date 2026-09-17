#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class W_SHAREDPLUGINSCENE_DLL WExposedSceneProperty : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WExposedSceneProperty, WReflectedClass);

public:
  WString m_sName;
  WUuid m_Object;
  WString m_sPropertyPath;
};

class W_SHAREDPLUGINSCENE_DLL WExposedDocumentObjectPropertiesMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WExposedDocumentObjectPropertiesMsgToEngine, WEditorEngineDocumentMsg);

public:
  WDynamicArray<WExposedSceneProperty> m_Properties;
};

class W_SHAREDPLUGINSCENE_DLL WExportSceneGeometryMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WExportSceneGeometryMsgToEngine, WEditorEngineDocumentMsg);

public:
  bool m_bSelectionOnly = false;
  WString m_sOutputFile;
  int m_iExtractionMode; // WWorldGeoExtractionUtil::ExtractionMode
  WMat3 m_Transform;
};

class W_SHAREDPLUGINSCENE_DLL WPullObjectStateMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WPullObjectStateMsgToEngine, WEditorEngineDocumentMsg);
};

struct WPushObjectStateData
{
  WUuid m_LayerGuid;
  WUuid m_ObjectGuid;
  WVec3 m_vPosition;
  WQuat m_qRotation;
  bool m_bAdjustFromPrefabRootChild = false; // only used internally, not synchronized
  WMap<WString, WTransform> m_BoneTransforms;
};

W_DECLARE_REFLECTABLE_TYPE(W_SHAREDPLUGINSCENE_DLL, WPushObjectStateData);

class W_SHAREDPLUGINSCENE_DLL WPushObjectStateMsgToEditor : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WPushObjectStateMsgToEditor, WEditorEngineDocumentMsg);

public:
  WDynamicArray<WPushObjectStateData> m_ObjectStates;
};

class W_SHAREDPLUGINSCENE_DLL WActiveLayerChangedMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WActiveLayerChangedMsgToEngine, WEditorEngineDocumentMsg);

public:
  WUuid m_ActiveLayer;
};

class W_SHAREDPLUGINSCENE_DLL WLayerVisibilityChangedMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WLayerVisibilityChangedMsgToEngine, WEditorEngineDocumentMsg);

public:
  WHybridArray<WUuid, 1> m_HiddenLayers;
};

/// Sent from the editor to the engine to communicate the desired child object order for a component.
///
/// Targets a component identified by its GUID. The component must handle WMsgSyncChildOrder.
/// Used in conjunction with WSyncChildOrderAttribute.
class W_SHAREDPLUGINSCENE_DLL WSyncChildOrderMsgToEngine : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WSyncChildOrderMsgToEngine, WEditorEngineDocumentMsg);

public:
  WUuid m_LayerGuid;
  WUuid m_ComponentGuid;
  WDynamicArray<WUuid> m_ChildOrder;
};
