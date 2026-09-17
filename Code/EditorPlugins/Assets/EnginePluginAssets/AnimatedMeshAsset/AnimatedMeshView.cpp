#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

WAnimatedMeshViewContext::WAnimatedMeshViewContext(WAnimatedMeshContext* pAnimatedMeshContext)
  : WEngineProcessViewContext(pAnimatedMeshContext)
{
  m_pContext = pAnimatedMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WAnimatedMeshViewContext::~WAnimatedMeshViewContext() = default;

bool WAnimatedMeshViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}


WViewHandle WAnimatedMeshViewContext::CreateView()
{
  WView* pView = CreateDefaultView("AnimatedMesh Editor - View");
  return pView->GetHandle();
}

void WAnimatedMeshViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    WEngineProcessViewContext::DrawSimpleGrid();
  }

  WEngineProcessViewContext::SetCamera(pMsg);

  auto hAnimatedMesh = m_pContext->GetAnimatedMesh();
  if (hAnimatedMesh.IsValid())
  {
    WResourceLock<WMeshResource> pAnimatedMesh(hAnimatedMesh, WResourceAcquireMode::AllowLoadingFallback);
    WResourceLock<WMeshBufferResource> pAnimatedMeshBuffer(pAnimatedMesh->GetMeshBuffer(), WResourceAcquireMode::AllowLoadingFallback);

    WUInt32 uiNumVertices = 0;
    WUInt32 uiVertexByteSize = 0;
    for (auto hBuffer : pAnimatedMeshBuffer->GetVertexBuffers())
    {
      if (auto pBuffer = WGALDevice::GetDefaultDevice()->GetBuffer(hBuffer))
      {
        auto& bufferDesc = pBuffer->GetDescription();
        uiNumVertices = WMath::Max(uiNumVertices, bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize);
        uiVertexByteSize += bufferDesc.m_uiStructSize;
      }
    }

    WUInt32 uiNumTriangles = pAnimatedMeshBuffer->GetPrimitiveCount();
    WVec3 bboxExtents = WVec3(2);

    if (pAnimatedMeshBuffer->GetBounds().IsValid())
    {
      bboxExtents = pAnimatedMeshBuffer->GetBounds().m_vBoxHalfExtents * 2.0f;
    }

    auto& streamConfig = pAnimatedMeshBuffer->GetVertexStreamConfig();
    const WUInt32 uiNumUVs = streamConfig.HasTexCoord0() + streamConfig.HasTexCoord1();
    const WUInt32 uiNumColors = streamConfig.HasColor0() + streamConfig.HasColor1();

    WStringBuilder sText;
    sText.AppendFormat("Bones: \t{}\n", pAnimatedMesh->m_Bones.GetCount());
    sText.AppendFormat("Triangles: \t{}\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\n", uiVertexByteSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", WArgF(bboxExtents.x, 2), WArgF(bboxExtents.y, 2), WArgF(bboxExtents.z, 2));

    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
