#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WKrautTreeViewContext::WKrautTreeViewContext(WKrautTreeContext* pKrautTreeContext)
  : WEngineProcessViewContext(pKrautTreeContext)
{
  m_pKrautTreeContext = pKrautTreeContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WKrautTreeViewContext::~WKrautTreeViewContext() = default;

bool WKrautTreeViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}

WViewHandle WKrautTreeViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Kraut Tree Editor - View");
  return pView->GetHandle();
}

void WKrautTreeViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  WEngineProcessViewContext::SetCamera(pMsg);

  WStringBuilder sText;

  // Distance from camera to tree (tree preview is always at world origin)
  const float fDistance = m_Camera.GetPosition().GetLength();
  sText.AppendFormat("Distance: \t{0}m\t\n", WArgF(fDistance, 1));

  // Determine which regular LOD would be auto-selected at this distance
  WInt32 iAutoLod = -1; // -1 = beyond all LOD distances (would not render in auto mode)
  WInt32 iLodOverride = -1;

  auto hGenRes = m_pKrautTreeContext->GetResource();
  if (hGenRes.IsValid())
  {
    WResourceLock<WKrautGeneratorResource> pGenRes(hGenRes, WResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (pGenRes.GetAcquireResult() != WResourceAcquireResult::None)
    {
      const auto& pDesc = pGenRes->GetDescriptor();
      if (pDesc != nullptr)
      {
        const float fDistSqr = fDistance * fDistance;
        float fPrevMaxDist = 0.0f;

        for (WUInt32 n = 0; n < 5; ++n)
        {
          const Kraut::LodDesc& lodDesc = pDesc->m_LodDesc[n];
          if (lodDesc.m_Mode != Kraut::LodMode::Full)
            break;

          const float fMaxDist = lodDesc.m_uiLodDistance * pDesc->m_fLodDistanceScale * pDesc->m_fUniformScaling;
          if (fDistSqr >= (fPrevMaxDist * fPrevMaxDist) && fDistSqr < (fMaxDist * fMaxDist))
          {
            iAutoLod = (WInt32)n;
            break;
          }
          fPrevMaxDist = fMaxDist;
        }
      }
    }
  }

  // Get the active LOD override from the Kraut tree component
  WWorld* pWorld = m_pKrautTreeContext->GetWorld();
  if (pWorld != nullptr)
  {
    W_LOCK(pWorld->GetReadMarker());
    WKrautTreeComponent* pTree = nullptr;
    if (pWorld->TryGetComponent(m_pKrautTreeContext->GetKrautComponentHandle(), pTree))
      iLodOverride = pTree->m_iLodOverride;
  }

  if (iLodOverride == -1)
  {
    // Automatic LOD selection: the auto-selected LOD is also the active one
    if (iAutoLod < 0)
      sText.AppendFormat("Active LOD: \tNone (hidden)");
    else
      sText.AppendFormat("Active LOD: \tLOD {}", iAutoLod);
  }
  else
  {
    if (iLodOverride == 0)
      sText.AppendFormat("Active LOD: \tFull Detail (fixed)\n");
    else
      sText.AppendFormat("Active LOD: \tLOD {} (fixed)\n", iLodOverride - 1);

    // Fixed LOD: show both the auto-selected and the actually forced LOD
    if (iAutoLod < 0)
      sText.AppendFormat("Auto LOD: \tNone (hidden)");
    else
      sText.AppendFormat("Auto LOD: \tLOD {}", iAutoLod);
  }

  WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "KrautStats", sText);
}
