#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Grid/GridRenderer.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Shader/ShaderResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGridRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorGridExtractor, 1, WRTTIDefaultAllocator<WEditorGridExtractor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGridRenderer, 1, WRTTIDefaultAllocator<WGridRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEditorGridExtractor::WEditorGridExtractor(const char* szName)
  : WExtractor(szName)
{
  m_pSceneContext = nullptr;
}

WGridRenderer::WGridRenderer()
{
  CreateVertexBuffer();
}

void WGridRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WGridRenderData>());
}

void WGridRenderer::CreateVertexBuffer()
{
  if (m_VertexBuffer.IsInitialized())
    return;

  // load the shader
  {
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Debug/DebugPrimitive.WShader");
  }

  // Create the vertex buffer
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(GridVertex);
    desc.m_uiTotalSize = s_uiBufferSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer | WGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_VertexBuffer.Initialize(desc, "GridRenderer - VertexBuffer");
  }

  // Setup the vertex declaration
  {
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Position;
      va.m_eFormat = WGALResourceFormat::XYZFloat;
      va.m_uiOffset = offsetof(GridVertex, m_position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Color0;
      va.m_eFormat = WGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(GridVertex, m_color);
    }
  }
}

void WGridRenderer::CreateGrid(const WGridRenderData& rd) const
{
  m_Vertices.Clear();
  m_Vertices.Reserve(100);

  const WVec3 vCenter = rd.m_vGlobalPosition;
  const WVec3 vTangent1 = rd.m_qGlobalRotation * WVec3(1, 0, 0);
  const WVec3 vTangent2 = rd.m_qGlobalRotation * WVec3(0, 1, 0);
  const WInt32 iNumLines1 = rd.m_iLastLine1 - rd.m_iFirstLine1;
  const WInt32 iNumLines2 = rd.m_iLastLine2 - rd.m_iFirstLine2;
  const float maxExtent1 = iNumLines1 * rd.m_fDensity;
  const float maxExtent2 = iNumLines2 * rd.m_fDensity;

  WColor cCenter = WColorScheme::GetColor(WColorScheme::Blue, 6, 0.5f);
  WColor cTen = WColorScheme::LightUI(WColorScheme::Gray) * 0.7f;
  WColor cOther = WColorScheme::LightUI(WColorScheme::Gray) * 0.5f;

  if (rd.m_bOrthoMode)
  {
    // dimmer colors in ortho mode, to be more in the background
    cCenter *= 0.5f;
    cTen *= 0.4f;
    cOther *= 0.4f;

    // in ortho mode, the origin lines are highlighted when global space is enabled
    if (!rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }
  else
  {
    // in perspective mode, the lines through the object are highlighted when local space is enabled
    if (rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }

  const WVec3 vCorner = vCenter + rd.m_iFirstLine1 * rd.m_fDensity * vTangent1 + rd.m_iFirstLine2 * rd.m_fDensity * vTangent2;

  for (WInt32 i = 0; i <= iNumLines1; ++i)
  {
    const WInt32 iLineIdx = rd.m_iFirstLine1 + i;

    WColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent2;
  }

  for (WInt32 i = 0; i <= iNumLines2; ++i)
  {
    const WInt32 iLineIdx = rd.m_iFirstLine2 + i;

    WColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i + vTangent1 * maxExtent1;
  }
}

void WGridRenderer::RenderBatch(const WRenderViewContext& renderViewContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  for (auto it = batch.GetIterator<WGridRenderData>(); it.IsValid(); ++it)
  {
    CreateGrid(*it);

    if (m_Vertices.IsEmpty())
      return;

    WRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

    pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
    pRenderContext->BindShader(m_hShader);

    WUInt32 uiNumLineVertices = m_Vertices.GetCount();
    const GridVertex* pLineData = m_Vertices.GetData();

    while (uiNumLineVertices > 0)
    {
      WGALBufferHandle hBuffer = m_VertexBuffer.GetNewBuffer();
      const WUInt32 uiNumLineVerticesInBatch = WMath::Min<WUInt32>(uiNumLineVertices, s_uiLineVerticesPerBatch);
      W_ASSERT_DEBUG(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

      pRenderContext->GetCommandEncoder()->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

      pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), m_VertexAttributes, WGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);
      pRenderContext->DrawMeshBuffer().IgnoreResult();

      uiNumLineVertices -= uiNumLineVerticesInBatch;
      pLineData += s_uiLineVerticesPerBatch;
    }
  }
}

float AdjustGridDensity(float fDensity, WUInt32 uiWindowWidth, float fOrthoDimX, WUInt32 uiMinPixelsDist)
{
  WInt32 iFactor = 1;
  float fNewDensity = fDensity;

  while (true)
  {
    const float stepsAtDensity = fOrthoDimX / fNewDensity;
    const float minPixelsAtDensity = stepsAtDensity * uiMinPixelsDist;

    if (minPixelsAtDensity < uiWindowWidth)
      break;

    iFactor *= 10;
    fNewDensity = fDensity * iFactor;
  }

  return fNewDensity;
}

void WEditorGridExtractor::Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  if (m_pSceneContext == nullptr || m_pSceneContext->GetGridDensity() == 0.0f)
    return;

  const WCamera* cam = view.GetCamera();
  float fDensity = m_pSceneContext->GetGridDensity();

  W_LOCK(view.GetWorld()->GetReadMarker());
  auto pRenderDataManager = view.GetWorld()->GetModuleReadOnly<WRenderDataManager>();

  WGridRenderData* pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<WGridRenderData>(nullptr);
  pRenderData->m_bOrthoMode = cam->IsOrthographic();
  pRenderData->m_bGlobal = m_pSceneContext->IsGridInGlobalSpace();

  if (cam->IsOrthographic())
  {
    const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    const float fDimX = cam->GetDimensionX(fAspectRatio) * 0.5f;
    const float fDimY = cam->GetDimensionY(fAspectRatio) * 0.5f;

    fDensity = AdjustGridDensity(fDensity, (WUInt32)view.GetViewport().width, fDimX, 10);
    pRenderData->m_fDensity = fDensity;

    pRenderData->m_vGlobalPosition = cam->GetCenterDirForwards() * cam->GetFarPlane() * 0.9f;

    WMat3 mRot;
    mRot.SetColumn(0, cam->GetCenterDirRight());
    mRot.SetColumn(1, cam->GetCenterDirUp());
    mRot.SetColumn(2, cam->GetCenterDirForwards());
    pRenderData->m_qGlobalRotation = WQuat::MakeFromMat3(mRot);

    const WVec3 vBottomLeft = cam->GetCenterPosition() - cam->GetCenterDirRight() * fDimX - cam->GetCenterDirUp() * fDimY;
    const WVec3 vTopRight = cam->GetCenterPosition() + cam->GetCenterDirRight() * fDimX + cam->GetCenterDirUp() * fDimY;

    WPlane plane1, plane2;
    plane1 = WPlane::MakeFromNormalAndPoint(cam->GetCenterDirRight(), WVec3(0));
    plane2 = WPlane::MakeFromNormalAndPoint(cam->GetCenterDirUp(), WVec3(0));

    const float fFirstDist1 = plane1.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist1 = plane1.GetDistanceTo(vTopRight) + fDensity;

    const float fFirstDist2 = plane2.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist2 = plane2.GetDistanceTo(vTopRight) + fDensity;


    WVec3& val = pRenderData->m_vGlobalPosition;
    val.x = WMath::RoundToMultiple(val.x, pRenderData->m_fDensity);
    val.y = WMath::RoundToMultiple(val.y, pRenderData->m_fDensity);
    val.z = WMath::RoundToMultiple(val.z, pRenderData->m_fDensity);

    pRenderData->m_iFirstLine1 = (WInt32)WMath::Trunc(fFirstDist1 / fDensity);
    pRenderData->m_iLastLine1 = (WInt32)WMath::Trunc(fLastDist1 / fDensity);
    pRenderData->m_iFirstLine2 = (WInt32)WMath::Trunc(fFirstDist2 / fDensity);
    pRenderData->m_iLastLine2 = (WInt32)WMath::Trunc(fLastDist2 / fDensity);
  }
  else
  {
    auto& globalTransform = m_pSceneContext->GetGridTransform();

    // grid is disabled
    if (globalTransform.m_vScale.IsZero(0.001f))
      return;

    pRenderData->m_vGlobalPosition = globalTransform.m_vPosition;
    pRenderData->m_qGlobalRotation = globalTransform.m_qRotation;

    pRenderData->m_fDensity = fDensity;

    const WInt32 iNumLines = 50;
    pRenderData->m_iFirstLine1 = -iNumLines;
    pRenderData->m_iLastLine1 = iNumLines;
    pRenderData->m_iFirstLine2 = -iNumLines;
    pRenderData->m_iLastLine2 = iNumLines;
  }

  ref_extractedRenderData.AddRenderData(pRenderData, WDefaultRenderDataCategories::SimpleTransparent);
}

WResult WEditorGridExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}


WResult WEditorGridExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}
