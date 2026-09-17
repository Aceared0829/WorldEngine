#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

WMeshViewContext::WMeshViewContext(WMeshContext* pMeshContext)
  : WEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WMeshViewContext::~WMeshViewContext() = default;

bool WMeshViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}


WViewHandle WMeshViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Mesh Editor - View");
  return pView->GetHandle();
}

void WMeshViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    WEngineProcessViewContext::DrawSimpleGrid();
  }

  WEngineProcessViewContext::SetCamera(pMsg);

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    WResourceLock<WMeshResource> pMesh(hMesh, WResourceAcquireMode::AllowLoadingFallback);
    WResourceLock<WMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), WResourceAcquireMode::AllowLoadingFallback);

    WUInt32 uiNumVertices = 0;
    WUInt32 uiVertexByteSize = 0;
    for (auto hBuffer : pMeshBuffer->GetVertexBuffers())
    {
      if (auto pBuffer = WGALDevice::GetDefaultDevice()->GetBuffer(hBuffer))
      {
        auto& bufferDesc = pBuffer->GetDescription();
        uiNumVertices = WMath::Max(uiNumVertices, bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize);
        uiVertexByteSize += bufferDesc.m_uiStructSize;
      }
    }

    const WUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    WVec3 bboxExtents = WVec3(2);

    if (pMeshBuffer->GetBounds().IsValid())
    {
      bboxExtents = pMeshBuffer->GetBounds().m_vBoxHalfExtents * 2.0f;
    }

    auto& streamConfig = pMeshBuffer->GetVertexStreamConfig();
    const WUInt32 uiNumUVs = streamConfig.HasTexCoord0() + streamConfig.HasTexCoord1();
    const WUInt32 uiNumColors = streamConfig.HasColor0() + streamConfig.HasColor1();

    WStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", uiVertexByteSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", WArgF(bboxExtents.x, 2), WArgF(bboxExtents.y, 2), WArgF(bboxExtents.z, 2));

    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }

  if (m_uiLastHoveredPartIndex != WInvalidIndex && m_uiLastHoveredPartIndex < (WUInt32)m_pContext->m_SlotNames.GetCount())
  {
    WStringBuilder sHoverText;
    sHoverText.AppendFormat("Slot {}: {}\t\n(Ctrl+MMB to open)\t", m_uiLastHoveredPartIndex, m_pContext->m_SlotNames[m_uiLastHoveredPartIndex]);
    WDebugRenderer::DrawInfoText(m_hView, WDebugTextPlacement::TopLeft, "HoveredMaterial", sHoverText);
  }
}

void WMeshViewContext::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
  WEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewRedrawMsgToEngine>())
  {
    const WViewRedrawMsgToEngine* pMsg2 = static_cast<const WViewRedrawMsgToEngine*>(pMsg);

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.Active"), pMsg2->m_bUpdatePickingData);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickSelected"), pMsg2->m_bEnablePickingSelected);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickTransparent"), pMsg2->m_bEnablePickTransparent);
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewPickingMsgToEngine>())
  {
    const WViewPickingMsgToEngine* pMsg2 = static_cast<const WViewPickingMsgToEngine*>(pMsg);
    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
}

void WMeshViewContext::PickObjectAt(WUInt16 x, WUInt16 y)
{
  WViewPickingResultMsgToEditor res;
  W_SCOPE_EXIT(SendViewMessage(&res));

  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hView, pView))
    return;

  auto pBlackboard = pView->GetBlackboard();
  pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.PickingPosition"), WVec2(x, y));

  auto pPickedPositionEntry = pBlackboard->GetEntry("EditorPickingPass.PickedPosition");
  if (pPickedPositionEntry == nullptr || pPickedPositionEntry->m_Value.IsA<WVec3>() == false)
    return;

  const WUInt32 uiPickingID = pBlackboard->GetEntryValue("EditorPickingPass.PickedID").ConvertTo<WUInt32>();
  res.m_vPickedNormal = pBlackboard->GetEntryValue("EditorPickingPass.PickedNormal").ConvertTo<WVec3>();
  res.m_vPickingRayStartPosition = pBlackboard->GetEntryValue("EditorPickingPass.PickedRayStartPosition").ConvertTo<WVec3>();
  res.m_vPickedPosition = pPickedPositionEntry->m_Value.ConvertTo<WVec3>();

  // The component and object GUIDs are not available in the mesh preview since the objects
  // are not registered in the component picking map. Only the part index (material slot) is relevant here.
  res.m_uiPartIndex = (uiPickingID >> 24) & 0xFF;

  m_uiLastHoveredPartIndex = (uiPickingID != 0) ? res.m_uiPartIndex : WInvalidIndex;
}
