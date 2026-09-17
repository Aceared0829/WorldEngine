#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineRestartSimulationMsg, 1, WRTTIDefaultAllocator<WEditorEngineRestartSimulationMsg>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineLoopAnimationMsg, 1, WRTTIDefaultAllocator<WEditorEngineLoopAnimationMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorEngineSetMaterialsMsg, 1, WRTTIDefaultAllocator<WEditorEngineSetMaterialsMsg>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
    W_ARRAY_MEMBER_PROPERTY("SlotNames", m_SlotNames),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
