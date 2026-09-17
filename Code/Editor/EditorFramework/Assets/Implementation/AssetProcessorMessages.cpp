#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcessAssetMsg, 1, WRTTIDefaultAllocator<WProcessAssetMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
    W_MEMBER_PROPERTY("AssetHash", m_AssetHash),
    W_MEMBER_PROPERTY("ThumbHash", m_ThumbHash),
    W_MEMBER_PROPERTY("PackageHash", m_PackageHash),
    W_MEMBER_PROPERTY("AssetPath", m_sAssetPath),
    W_MEMBER_PROPERTY("Platform", m_sPlatform),
    W_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcessAssetResponseMsg, 1, WRTTIDefaultAllocator<WProcessAssetResponseMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Status", m_Status),
    W_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries),
    W_MAP_MEMBER_PROPERTY("MissmatchTransformDependencies", m_MissmatchTransformDependencies),
    W_MAP_MEMBER_PROPERTY("MissmatchThumbnailDependencies", m_MissmatchThumbnailDependencies),
    W_MEMBER_PROPERTY("MissmatchAssetHash", m_uiMissmatchAssetHash),
    W_MEMBER_PROPERTY("MissmatchThumbHash", m_uiMissmatchThumbHash),
    W_MEMBER_PROPERTY("StartedProcessing", m_StartedProcessing),
    W_MEMBER_PROPERTY("StartedTransform", m_StartedTransform),
    W_MEMBER_PROPERTY("FinishedProcessing", m_FinishedProcessing),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFreeAllResourcesMsg, 1, WRTTIDefaultAllocator<WFreeAllResourcesMsg>)
{
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
