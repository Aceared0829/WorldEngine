#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDragDropInfo, 1, WRTTIDefaultAllocator<WDragDropInfo>)
W_END_DYNAMIC_REFLECTED_TYPE;

WDragDropInfo::WDragDropInfo()
{
  m_vDropPosition.Set(WMath::NaN<float>());
  m_vDropNormal.Set(WMath::NaN<float>());
  m_iTargetObjectSubID = -1;
  m_iTargetObjectInsertChildIndex = -1;
  m_bShiftKeyDown = false;
  m_bCtrlKeyDown = false;
}


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDragDropConfig, 1, WRTTIDefaultAllocator<WDragDropConfig>)
W_END_DYNAMIC_REFLECTED_TYPE;

WDragDropConfig::WDragDropConfig()
{
  m_bPickSelectedObjects = false;
}
