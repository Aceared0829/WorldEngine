#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/World/World.h>
#  include <Foundation/IO/TypeVersionContext.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/DearImgui/DearImguiRenderer.h>
#  include <RendererCore/Pipeline/ExtractedRenderData.h>
#  include <RendererCore/Pipeline/RenderDataManager.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderContext/RenderContext.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Shader/ShaderResource.h>
#  include <RendererFoundation/Device/Device.h>

#  include <Imgui/imgui_internal.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImguiRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImguiExtractor, 1, WRTTIDefaultAllocator<WImguiExtractor>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImguiRenderer, 1, WRTTIDefaultAllocator<WImguiRenderer>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WImguiExtractor::WImguiExtractor(const char* szName)
  : WExtractor(szName)
{
  m_DependsOn.PushBack(WMakeHashedString("WVisibleObjectsExtractor"));
}

void WImguiExtractor::Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  WImgui* pImGui = WImgui::GetSingleton();
  if (pImGui == nullptr)
  {
    return;
  }

  {
    W_LOCK(pImGui->m_ViewToContextTableMutex);
    WImgui::Context context;
    if (!pImGui->m_ViewToContextTable.TryGetValue(view.GetHandle(), context))
    {
      // No context for this view
      return;
    }

    WUInt64 uiCurrentFrameCounter = WRenderWorld::GetFrameCounter();
    if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
    {
      // Nothing was rendered with ImGui this frame
      return;
    }

    context.m_uiFrameRenderCounter = uiCurrentFrameCounter;

    ImGui::SetCurrentContext(context.m_pImGuiContext);
  }

  ImGui::Render();

  ImDrawData* pDrawData = ImGui::GetDrawData();

  if (pDrawData && pDrawData->Valid)
  {
    W_LOCK(view.GetWorld()->GetReadMarker());
    auto pRenderDataManager = view.GetWorld()->GetModuleReadOnly<WRenderDataManager>();

    for (int draw = 0; draw < pDrawData->CmdListsCount; ++draw)
    {
      WImguiRenderData* pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<WImguiRenderData>(nullptr);
      pRenderData->m_uiSortingKey = draw;

      // copy the vertex data
      // uses the frame allocator to prevent unnecessary deallocations
      {
        const ImDrawList* pCmdList = pDrawData->CmdLists[draw];

        pRenderData->m_Vertices = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WImguiVertex, pCmdList->VtxBuffer.size());
        for (WUInt32 vtx = 0; vtx < pRenderData->m_Vertices.GetCount(); ++vtx)
        {
          const auto& vert = pCmdList->VtxBuffer[vtx];

          pRenderData->m_Vertices[vtx].m_Position.Set(vert.pos.x, vert.pos.y, 0);
          pRenderData->m_Vertices[vtx].m_TexCoord.Set(vert.uv.x, vert.uv.y);
          pRenderData->m_Vertices[vtx].m_Color = *reinterpret_cast<const WColorGammaUB*>(&vert.col);
        }

        pRenderData->m_Indices = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), ImDrawIdx, pCmdList->IdxBuffer.size());
        for (WUInt32 i = 0; i < pRenderData->m_Indices.GetCount(); ++i)
        {
          pRenderData->m_Indices[i] = pCmdList->IdxBuffer[i];
        }
      }

      // pass along an WImguiBatch for every necessary drawcall
      {
        const ImDrawList* pCommands = pDrawData->CmdLists[draw];

        pRenderData->m_Batches = W_NEW_ARRAY(WFrameAllocator::GetCurrentAllocator(), WImguiBatch, pCommands->CmdBuffer.Size);

        for (int cmdIdx = 0; cmdIdx < pCommands->CmdBuffer.Size; cmdIdx++)
        {
          const ImDrawCmd* pCmd = &pCommands->CmdBuffer[cmdIdx];

          WImguiBatch& batch = pRenderData->m_Batches[cmdIdx];
          batch.m_uiVertexCount = static_cast<WUInt16>(pCmd->ElemCount);
          batch.m_TextureId = pCmd->TextureId;
          batch.m_ScissorRect = WRectU32((WUInt32)pCmd->ClipRect.x, (WUInt32)pCmd->ClipRect.y, (WUInt32)(pCmd->ClipRect.z - pCmd->ClipRect.x), (WUInt32)(pCmd->ClipRect.w - pCmd->ClipRect.y));
        }
      }

      ref_extractedRenderData.AddRenderData(pRenderData, WDefaultRenderDataCategories::GUI);
    }
  }
}

WResult WImguiExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WImguiExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

WImguiRenderer::WImguiRenderer()
{
  SetupRenderer();
}

WImguiRenderer::~WImguiRenderer()
{
  m_hShader.Invalidate();
}

void WImguiRenderer::GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WImguiRenderData>());
}

void WImguiRenderer::RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const
{
  if (WImgui::GetSingleton() == nullptr)
    return;

  WRenderContext* pRenderContext = renderContext.m_pRenderContext;
  WGALCommandEncoder* pCommandEncoder = pRenderContext->GetCommandEncoder();

  pRenderContext->BindShader(m_hShader);
  const auto& registeredTextures = WImgui::GetSingleton()->m_RegisteredTextures;
  WBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();

  for (auto it = batch.GetIterator<WImguiRenderData>(); it.IsValid(); ++it)
  {
    const WImguiRenderData* pRenderData = it;

    W_ASSERT_DEV(pRenderData->m_Vertices.GetCount() < s_uiVertexBufferSize, "GUI has too many elements to render in one drawcall");
    W_ASSERT_DEV(pRenderData->m_Indices.GetCount() < s_uiIndexBufferSize, "GUI has too many elements to render in one drawcall");

    WGALBufferHandle hVertexBuffer = m_VertexBuffer.GetNewBuffer();
    WGALBufferHandle hIndexBuffer = m_IndexBuffer.GetNewBuffer();

    pCommandEncoder->UpdateBuffer(hVertexBuffer, 0, WMakeArrayPtr(pRenderData->m_Vertices.GetPtr(), pRenderData->m_Vertices.GetCount()).ToByteArray(), WGALUpdateMode::AheadOfTime);
    pCommandEncoder->UpdateBuffer(hIndexBuffer, 0, WMakeArrayPtr(pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount()).ToByteArray(), WGALUpdateMode::AheadOfTime);

    pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hVertexBuffer, 1), hIndexBuffer, m_VertexAttributes, WGALPrimitiveTopology::Triangles, pRenderData->m_Indices.GetCount() / 3);

    WUInt32 uiFirstIndex = 0;
    const WUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (WUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const WImguiBatch& imGuiBatch = pRenderData->m_Batches[batchIdx];

      WImgui::WImGuiTextureRegistration reg;
      WImgui::WImGuiTextureIdData handle = *reinterpret_cast<const WImgui::WImGuiTextureIdData*>(&imGuiBatch.m_TextureId);
      if (imGuiBatch.m_uiVertexCount > 0 && registeredTextures.TryGetValue(handle, reg))
      {
        pCommandEncoder->SetScissorRect(imGuiBatch.m_ScissorRect);

        switch (reg.m_Type)
        {
          case WImgui::WImGuiTextureRegistration::Type::Texture2D:
          {
            pRenderContext->BindShader(m_hShader);
            bindGroup.BindTexture("BaseTexture", reg.m_hTexture2D);
            break;
          }
          case WImgui::WImGuiTextureRegistration::Type::GALTexture:
          {
            pRenderContext->BindShader(m_hShader);
            bindGroup.BindTexture("BaseTexture", reg.m_hGALTexture);
            break;
          }
          case WImgui::WImGuiTextureRegistration::Type::Material:
          {
            pRenderContext->BindMaterial(reg.m_hMaterial);
            break;
          }
        }

        pRenderContext->DrawMeshBuffer(imGuiBatch.m_uiVertexCount / 3, uiFirstIndex / 3).IgnoreResult();
      }

      uiFirstIndex += imGuiBatch.m_uiVertexCount;
    }
  }

  // Reset scissor to default.
  WRectFloat rect = renderContext.m_pViewData->m_ViewPortRect;
  pCommandEncoder->SetScissorRect(WRectU32((WUInt32)rect.x, (WUInt32)rect.y, (WUInt32)rect.width, (WUInt32)rect.height));
}

void WImguiRenderer::SetupRenderer()
{
  if (m_hShader.IsValid())
    return;

  // load the shader
  {
    m_hShader = WResourceManager::LoadResource<WShaderResource>("Shaders/GUI/DearImguiPrimitives.WShader");
  }

  // Create the vertex buffer
  {
    WGALBufferCreationDescription desc;
    desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer | WGALBufferUsageFlags::Transient;

    desc.m_uiStructSize = sizeof(WImguiVertex);
    desc.m_uiTotalSize = s_uiVertexBufferSize * desc.m_uiStructSize;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_VertexBuffer.Initialize(desc, "DearImguiRenderer - VertexBuffer");
  }

  // Create the index buffer
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ImDrawIdx);
    desc.m_uiTotalSize = s_uiIndexBufferSize * desc.m_uiStructSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::IndexBuffer | WGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_IndexBuffer.Initialize(desc, "DearImguiRenderer - IndexBuffer");
  }

  // Setup the vertex declaration
  {
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Position;
      va.m_eFormat = WGALResourceFormat::XYZFloat;
      va.m_uiOffset = offsetof(WImguiVertex, m_Position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = WGALResourceFormat::UVFloat;
      va.m_uiOffset = offsetof(WImguiVertex, m_TexCoord);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Color0;
      va.m_eFormat = WGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(WImguiVertex, m_Color);
    }
  }
}

#endif

W_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImguiRenderer);
