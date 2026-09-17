#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <SharedPluginScene/Common/Messages.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedSceneProperty, 1, WRTTIDefaultAllocator<WExposedSceneProperty>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Object", m_Object)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedDocumentObjectPropertiesMsgToEngine, 1, WRTTIDefaultAllocator<WExposedDocumentObjectPropertiesMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExportSceneGeometryMsgToEngine, 1, WRTTIDefaultAllocator<WExportSceneGeometryMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Selection", m_bSelectionOnly),
    W_MEMBER_PROPERTY("File", m_sOutputFile),
    W_MEMBER_PROPERTY("Mode", m_iExtractionMode),
    W_MEMBER_PROPERTY("Transform", m_Transform),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPullObjectStateMsgToEngine, 1, WRTTIDefaultAllocator<WPullObjectStateMsgToEngine>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WPushObjectStateData, WNoBase, 1, WRTTIDefaultAllocator<WPushObjectStateData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LayerGuid", m_LayerGuid),
    W_MEMBER_PROPERTY("Guid", m_ObjectGuid),
    W_MEMBER_PROPERTY("Pos", m_vPosition),
    W_MEMBER_PROPERTY("Rot", m_qRotation),
    W_MAP_MEMBER_PROPERTY("Bones", m_BoneTransforms),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPushObjectStateMsgToEditor, 1, WRTTIDefaultAllocator<WPushObjectStateMsgToEditor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("States", m_ObjectStates)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WActiveLayerChangedMsgToEngine, 1, WRTTIDefaultAllocator<WActiveLayerChangedMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ActiveLayer", m_ActiveLayer),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerVisibilityChangedMsgToEngine, 1, WRTTIDefaultAllocator<WLayerVisibilityChangedMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("HiddenLayers", m_HiddenLayers),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSyncChildOrderMsgToEngine, 1, WRTTIDefaultAllocator<WSyncChildOrderMsgToEngine>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LayerGuid", m_LayerGuid),
    W_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    W_ARRAY_MEMBER_PROPERTY("ChildOrder", m_ChildOrder),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
