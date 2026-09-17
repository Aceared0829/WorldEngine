#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WJoltCollisionMeshViewContext::WJoltCollisionMeshViewContext(WJoltCollisionMeshContext* pMeshContext)
  : WEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WJoltCollisionMeshViewContext::~WJoltCollisionMeshViewContext() = default;

bool WJoltCollisionMeshViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}


WViewHandle WJoltCollisionMeshViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Collision Mesh Editor - View");
  return pView->GetHandle();
}

void WJoltCollisionMeshViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    WEngineProcessViewContext::DrawSimpleGrid();
  }

  WEngineProcessViewContext::SetCamera(pMsg);

  auto hResource = m_pContext->GetMesh();
  if (hResource.IsValid())
  {
    WResourceLock<WJoltMeshResource> pResource(hResource, WResourceAcquireMode::AllowLoadingFallback);
    WBoundingBox bbox = pResource->GetBounds().GetBox();
    WUInt32 uiNumTris = pResource->GetNumTriangles();
    WUInt32 uiNumVertices = pResource->GetNumVertices();
    WUInt32 uiNumPieces = pResource->GetNumConvexParts() + (pResource->HasTriangleMesh() ? 1 : 0);

    WStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\n", uiNumTris);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("Pieces: \t{}\n", uiNumPieces);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", WArgF(bbox.GetHalfExtents().x * 2, 2),
      WArgF(bbox.GetHalfExtents().y * 2, 2), WArgF(bbox.GetHalfExtents().z * 2, 2));

    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
