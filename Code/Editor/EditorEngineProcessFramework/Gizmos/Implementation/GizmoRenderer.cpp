#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGizmoRenderer, 1, WRTTIDefaultAllocator<WGizmoRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float WGizmoRenderer::s_fGizmoScale = 1.0f;

WGizmoRenderer::WGizmoRenderer() = default;
WGizmoRenderer::~WGizmoRenderer() = default;

void WGizmoRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WGizmoRenderData>());
}

void WGizmoRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
#if W_ENABLED(W_PLATFORM_WINDOWS)
  // special Windows specific hack:
  // When ALT is down, the editor shouldn't show any gizmos, because this is used for the orbit camera mode
  // in general the ALT key is problematic and shouldn't be used as a modifier in gizmos
  // so to indicate to users that ALT has a very different effect, and to discourage programmers from using ALT as a modifier,
  // we just hide all gizmos when ALT is down
  // however, detecting the ALT key is only possible with a direct OS check, since Qt doesn't report this as an individual key press
  // and the W input system is not active at all times
  if (GetKeyState(VK_MENU) & 0x8000)
    return;
#endif

  bool bOnlyPickable = false;

  if (auto pPickingRenderPass = WDynamicCast<const WPickingRenderPass*>(pPass))
  {
    // gizmos only exist for 'selected' objects, so ignore all gizmo rendering, if we don't want to pick selected objects
    if (!pPickingRenderPass->m_bPickSelected)
      return;

    bOnlyPickable = true;
  }

  const WGizmoRenderData* pRenderData = batch.GetFirstData<WGizmoRenderData>();

  // Mesh-less gizmos (e.g. CustomLines) render their geometry via WDebugRenderer inside the
  // component's extraction callback, so there is nothing to do here.
  if (!pRenderData->m_hMesh.IsValid())
    return;

  const WMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const WMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  WUInt32 uiSubMeshIndex = pRenderData->m_uiSubMeshIndex;

  WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiSubMeshIndex)
  {
    return;
  }

  const WMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiSubMeshIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);

  WConstantBufferStorage<WGizmoConstants>* pGizmoConstantBuffer;
  WConstantBufferStorageHandle hGizmoConstantBuffer = WRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);
  W_SCOPE_EXIT(WRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer));

  WBindGroupBuilder& bindGroupRenderPass = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
  bindGroupRenderPass.BindBuffer("WGizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<WGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    if (bOnlyPickable && !pRenderData->m_bIsPickable)
      continue;

    W_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    W_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    W_ASSERT_DEV(pRenderData->m_uiSubMeshIndex == uiSubMeshIndex, "Invalid batching (part)");

    WGizmoConstants& cb = pGizmoConstantBuffer->GetDataForWriting();
    WMat4 m = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.ObjectToWorldMatrix = m;
    m.Invert(0.001f).IgnoreResult(); // this can fail, if scale is 0 (which happens), doesn't matter in those cases
    cb.WorldToObjectMatrix = m;
    cb.GizmoColor = pRenderData->m_GizmoColor;
    cb.GizmoScale = fGizmoScale;
    cb.GameObjectID = pRenderData->m_uiUniqueID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
      // draw bounding box instead
      if (pRenderData->m_FallbackGlobalBBox.IsValid())
      {
        WDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_FallbackGlobalBBox, WColor::Magenta);
      }
#endif
    }
  }
}
