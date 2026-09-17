#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

WGizmoComponentManager::WGizmoComponentManager(WWorld* pWorld)
  : WComponentManager(pWorld)
{
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGizmoRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_COMPONENT_TYPE(WGizmoComponent, 1, WComponentMode::Static)
{
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgExtractRenderData, OnMsgExtractRenderData),
  }
  W_END_MESSAGEHANDLERS;
  W_BEGIN_ATTRIBUTES
  {
    new WHiddenAttribute(),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE;
// clang-format on

WGizmoComponent::WGizmoComponent() = default;
WGizmoComponent::~WGizmoComponent() = default;

WMeshRenderData* WGizmoComponent::CreateRenderData(const WRenderDataManager* pRenderDataManager) const
{
  WColor color = m_GizmoColor;

  auto pManager = static_cast<const WGizmoComponentManager*>(GetOwningManager());
  if (GetUniqueID() == pManager->m_uiHighlightID)
  {
    color = WColor(0.9f, 0.9f, 0.1f, color.a);
  }

  WGizmoRenderData* pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<WGizmoRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GizmoColor = color;
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_bIsPickable = m_bIsPickable;

  return pRenderData;
}

void WGizmoComponent::OnMsgExtractRenderData(WMsgExtractRenderData& msg) const
{
  if (!m_Lines.IsEmpty())
  {
    WTempHybridArray<WDebugRendererLine, 64> lines;
    lines.Reserve(m_Lines.GetCount() / 2);
    for (WUInt32 v = 0; v < m_Lines.GetCount(); v += 2)
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = m_Lines[v];
      l.m_end = m_Lines[v + 1];
    }

    WDebugRenderer::DrawLinesOccluded(WDebugRendererContext(msg.m_pView->GetHandle()), lines, m_GizmoColor.GetDarker(), GetOwner()->GetGlobalTransform());
    WDebugRenderer::DrawLines(WDebugRendererContext(msg.m_pView->GetHandle()), lines, m_GizmoColor, GetOwner()->GetGlobalTransform());

    if (!m_hMesh.IsValid())
      return;
  }

  SUPER::OnMsgExtractRenderData(msg);
}

WResult WGizmoComponent::GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg)
{
  WResult r = W_SUCCESS;

  if (!m_Lines.IsEmpty())
  {
    // The actual line data lives in the component and can be updated without touching bounds.
    // A unit-sized placeholder is enough because the gizmo is flagged as always visible.
    bounds = WBoundingBoxSphere::MakeFromCenterExtents(WVec3(0), WVec3(1), 1);
  }
  else
  {
    r = SUPER::GetLocalBounds(bounds, bAlwaysVisible, msg);
  }

  // Since there is always only a single gizmo on screen, there's no harm in making it always visible.
  // Must be set after SUPER, because the base implementation may reset it.
  bAlwaysVisible = true;
  return r;
}
