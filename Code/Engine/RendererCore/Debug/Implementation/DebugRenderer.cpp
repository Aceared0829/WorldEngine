#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/SimpleASCIIFont.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/BufferPool.h>
#include <RendererFoundation/Shader/Types.h>

WCVarFloat cvar_AppTextScale("App.TextScale", 1.0f, WCVarFlags::Save, "Global scale for debug text");
WCVarBool cvar_DebugRenderOccluded("Debug.RenderOccluded", true, WCVarFlags::Save, "Also render debug geometry behind opaque surfaces, dimmed.");

//////////////////////////////////////////////////////////////////////////

WDebugRendererContext::WDebugRendererContext(const WWorld* pWorld)
  : m_uiId(pWorld != nullptr ? pWorld->GetIndex() : 0)
{
}

WDebugRendererContext::WDebugRendererContext(const WViewHandle& hView)
  : m_uiId(hView.GetInternalID().m_Data)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WDebugTextHAlign, 1)
  W_ENUM_CONSTANTS(WDebugTextHAlign::Left, WDebugTextHAlign::Center, WDebugTextHAlign::Right)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WDebugTextVAlign, 1)
  W_ENUM_CONSTANTS(WDebugTextVAlign::Top, WDebugTextVAlign::Center, WDebugTextVAlign::Bottom)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WDebugTextPlacement, 1)
  W_ENUM_CONSTANTS(WDebugTextPlacement::TopLeft, WDebugTextPlacement::TopCenter, WDebugTextPlacement::TopRight)
  W_ENUM_CONSTANTS(WDebugTextPlacement::BottomLeft, WDebugTextPlacement::BottomCenter, WDebugTextPlacement::BottomRight)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct Vertex
  {
    WVec3 m_position;
    WColorLinearUB m_color;
  };

  static_assert(sizeof(Vertex) == 16);

  struct TexVertex
  {
    WVec3 m_position;
    WColorLinearUB m_color;
    WVec2 m_texCoord;
  };

  static_assert(sizeof(TexVertex) == 24);

  struct alignas(16) BoxData
  {
    WShaderTransform m_transform;
    WColor m_color;
  };

  static_assert(sizeof(BoxData) == 64);

  struct alignas(16) GlyphData
  {
    WVec2 m_topLeftCorner;
    WColorLinearUB m_color;
    WUInt16 m_glyphIndex;
    WUInt16 m_sizeInPixel;
  };

  static_assert(sizeof(GlyphData) == 16);

  struct TextLineData2D
  {
    WString m_text;
    WVec2 m_topLeftCorner;
    WColorLinearUB m_color;
    WUInt32 m_uiSizeInPixel;
  };

  struct TextLineData3D : public TextLineData2D
  {
    WVec3 m_position;
  };

  struct InfoTextData
  {
    WString m_group;
    WString m_text;
    WColor m_color;
  };

  struct PerContextData
  {
    WDynamicArray<Vertex, WAlignedAllocatorWrapper> m_lineVertices;
    WDynamicArray<Vertex, WAlignedAllocatorWrapper> m_lineOccludedVertices;
    WDynamicArray<Vertex, WAlignedAllocatorWrapper> m_triangleVertices;
    WDynamicArray<Vertex, WAlignedAllocatorWrapper> m_triangle2DVertices;
    WDynamicArray<Vertex, WAlignedAllocatorWrapper> m_line2DVertices;
    WDynamicArray<BoxData, WAlignedAllocatorWrapper> m_lineBoxes;
    WDynamicArray<BoxData, WAlignedAllocatorWrapper> m_solidBoxes;
    WMap<WGALTextureHandle, WDynamicArray<TexVertex, WAlignedAllocatorWrapper>> m_texTriangle2DVertices;
    WMap<WGALTextureHandle, WDynamicArray<TexVertex, WAlignedAllocatorWrapper>> m_texTriangle3DVertices;

    WDynamicArray<InfoTextData> m_infoTextData[(int)WDebugTextPlacement::ENUM_COUNT];
    WDynamicArray<TextLineData2D> m_textLines2D;
    WDynamicArray<TextLineData3D> m_textLines3D;
    WDynamicArray<GlyphData, WAlignedAllocatorWrapper> m_glyphs;
  };

  struct DoubleBufferedPerContextData
  {
    DoubleBufferedPerContextData()
    {
      m_uiLastRenderedFrame = 0;
      m_pData[0] = nullptr;
      m_pData[1] = nullptr;
    }

    WUInt64 m_uiLastRenderedFrame;
    WUniquePtr<PerContextData> m_pData[2];
  };

  static WHashTable<WDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static WMutex s_Mutex;

  static PerContextData& GetDataForExtraction(const WDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    const WUInt32 uiDataIndex = WRenderWorld::IsRenderingThread() && (doubleBufferedData.m_uiLastRenderedFrame != WRenderWorld::GetFrameCounter()) ? WRenderWorld::GetDataIndexForRendering() : WRenderWorld::GetDataIndexForExtraction();

    WUniquePtr<PerContextData>& pData = doubleBufferedData.m_pData[uiDataIndex];
    if (pData == nullptr)
    {
      doubleBufferedData.m_pData[uiDataIndex] = W_DEFAULT_NEW(PerContextData);
    }

    return *pData;
  }

  static void ClearRenderData()
  {
    W_LOCK(s_Mutex);

    for (auto it = s_PerContextData.GetIterator(); it.IsValid(); ++it)
    {
      PerContextData* pData = it.Value().m_pData[WRenderWorld::GetDataIndexForRendering()].Borrow();
      if (pData)
      {
        pData->m_lineVertices.Clear();
        pData->m_lineOccludedVertices.Clear();
        pData->m_line2DVertices.Clear();
        pData->m_lineBoxes.Clear();
        pData->m_solidBoxes.Clear();
        pData->m_triangleVertices.Clear();
        pData->m_triangle2DVertices.Clear();
        pData->m_texTriangle2DVertices.Clear();
        pData->m_texTriangle3DVertices.Clear();
        pData->m_textLines2D.Clear();
        pData->m_textLines3D.Clear();

        for (WUInt32 i = 0; i < (WUInt32)WDebugTextPlacement::ENUM_COUNT; ++i)
        {
          pData->m_infoTextData[i].Clear();
        }
      }
    }
  }

  static void OnRenderEvent(const WRenderWorldRenderEvent& e)
  {
    if (e.m_Type == WRenderWorldRenderEvent::Type::EndRender)
    {
      ClearRenderData();
    }
  }

  struct BufferType
  {
    enum Enum
    {
      Lines,
      LineBoxes,
      SolidBoxes,
      Triangles3D,
      Triangles2D,
      TexTriangles2D,
      TexTriangles3D,
      Glyphs,
      Lines2D,

      Count
    };
  };

  static WGALBufferPool s_DataBuffer[BufferType::Count];

  static WMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  static WMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  static WGALVertexAttribute s_VertexAttributes[2];
  static WGALVertexAttribute s_TexVertexAttributes[3];
  static WTexture2DResourceHandle s_hDebugFontTexture;

  static WShaderResourceHandle s_hDebugGeometryShader;
  static WShaderResourceHandle s_hDebugPrimitiveShader;
  static WShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static WShaderResourceHandle s_hDebugTextShader;

  enum
  {
    DEBUG_BUFFER_SIZE = 1024 * 256,
    BOXES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
  };

  static void CreateDataBuffer(BufferType::Enum bufferType, WUInt32 uiStructSize)
  {
    if (!s_DataBuffer[bufferType].IsInitialized())
    {
      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiStructSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource | WGALBufferUsageFlags::Transient;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_DataBuffer[bufferType].Initialize(desc, "DebugRenderer - StructuredBuffer");
    }
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, WUInt32 uiVertexSize)
  {
    if (!s_DataBuffer[bufferType].IsInitialized())
    {
      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiVertexSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer | WGALBufferUsageFlags::Transient;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_DataBuffer[bufferType].Initialize(desc, "DebugRenderer - VertexBuffer");
    }
  }

  static void DestroyBuffer(BufferType::Enum bufferType)
  {
    s_DataBuffer[bufferType].Deinitialize();
  }

  template <typename AddFunc>
  static WUInt32 AddTextLines(const WDebugRendererContext& context, const WFormatString& text0, const WVec2I32& vPositionInPixel, WUInt32 uiSizeInPixel, WDebugTextHAlign::Enum horizontalAlignment, WDebugTextVAlign::Enum verticalAlignment, AddFunc func)
  {
    if (text0.IsEmpty())
      return 0;

    WStringBuilder tmp;
    WStringView text = text0.GetText(tmp);

    WTempHybridArray<WStringView, 8> lines;
    WUInt32 maxLineLength = 0;

    WTempHybridArray<WUInt32, 8> maxColumWidth;
    bool isTabular = false;

    WStringBuilder sb;
    if (text.FindSubString("\n"))
    {
      sb = text;
      sb.Split(true, lines, "\n");

      for (auto& line : lines)
      {
        WUInt32 uiColIdx = 0;

        const char* colPtrCur = line.GetStartPointer();

        while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
        {
          isTabular = true;

          const WUInt32 colLen = WMath::RoundUp(1 + static_cast<WUInt32>(colPtrNext - colPtrCur), 4);

          maxColumWidth.EnsureCount(uiColIdx + 1);
          maxColumWidth[uiColIdx] = WMath::Max(maxColumWidth[uiColIdx], colLen);

          colPtrCur = colPtrNext + 1;
          ++uiColIdx;
        }

        // length of the last column (that wasn't counted)
        maxLineLength = WMath::Max(maxLineLength, WStringUtils::GetStringElementCount(colPtrCur, line.GetEndPointer()));
      }

      for (WUInt32 columnWidth : maxColumWidth)
      {
        maxLineLength += columnWidth;
      }
    }
    else
    {
      lines.PushBack(text);
      maxLineLength = text.GetElementCount();
      maxColumWidth.PushBack(maxLineLength);
    }


    const float fGlyphWidth = WDebugRenderer::GetTextGlyphWidth(uiSizeInPixel);
    const float fGlyphHeight = WMath::Ceil(uiSizeInPixel * cvar_AppTextScale);
    const float fLineHeight = WDebugRenderer::GetTextLineHeight(uiSizeInPixel);
    const float fLineSpacing = fLineHeight - fGlyphHeight;

    float screenPosX = (float)vPositionInPixel.x;
    if (horizontalAlignment == WDebugTextHAlign::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)vPositionInPixel.y;
    if (verticalAlignment == WDebugTextVAlign::Center)
      screenPosY -= WMath::Ceil(lines.GetCount() * fLineHeight * 0.5f) - fLineSpacing * 0.5f;
    else if (verticalAlignment == WDebugTextVAlign::Bottom)
      screenPosY -= lines.GetCount() * fLineHeight - fLineSpacing;

    {
      W_LOCK(s_Mutex);

      auto& data = GetDataForExtraction(context);

      WVec2 currentPos(screenPosX, screenPosY);

      for (WStringView line : lines)
      {
        currentPos.x = screenPosX;
        if (horizontalAlignment == WDebugTextHAlign::Center)
          currentPos.x -= WMath::Ceil(line.GetElementCount() * fGlyphWidth * 0.5f);

        if (isTabular)
        {
          WUInt32 uiColIdx = 0;

          const char* colPtrCur = line.GetStartPointer();

          WUInt32 addWidth = 0;

          while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
          {
            const WVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, WStringView(colPtrCur, colPtrNext), currentPos + tabOff);

            addWidth += maxColumWidth[uiColIdx];

            colPtrCur = colPtrNext + 1;
            ++uiColIdx;
          }

          // last column
          {
            const WVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, WStringView(colPtrCur, line.GetEndPointer()), currentPos + tabOff);
          }
        }
        else
        {
          func(data, line, currentPos);
        }

        currentPos.y += fLineHeight;
      }
    }

    return lines.GetCount();
  }

  static void AppendGlyphs(WDynamicArray<GlyphData, WAlignedAllocatorWrapper>& ref_glyphs, const TextLineData2D& textLine)
  {
    WVec2 currentPos = textLine.m_topLeftCorner;
    const float fGlyphWidth = WDebugRenderer::GetTextGlyphWidth(textLine.m_uiSizeInPixel);

    for (WUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = ref_glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? static_cast<WUInt16>(uiCharacter) : 0;
      glyphData.m_sizeInPixel = (WUInt16)WMath::Ceil(textLine.m_uiSizeInPixel * cvar_AppTextScale);

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  struct PersistentCrossData
  {
    float m_fSize;
    WColor m_Color;
    WMat4 m_Transform;
    WTime m_Timeout;
  };

  struct PersistentSphereData
  {
    float m_fRadius;
    WColor m_Color;
    WMat4 m_Transform;
    WTime m_Timeout;
  };

  struct PersistentBoxData
  {
    WVec3 m_vHalfSize;
    WColor m_Color;
    WMat4 m_Transform;
    WTime m_Timeout;
  };

  struct PersistentLineData
  {
    WHybridArray<WDebugRendererLine, 32> m_Lines;
    WColor m_Color;
    WMat4 m_Transform;
    WTime m_Timeout;
  };

  struct PersistentInfoTextData
  {
    WString m_sText;
    WDebugTextPlacement::Enum m_Placement;
    WColor m_Color;
    WTime m_Timeout;
  };

  struct PersistentPerContextData
  {
    WTime m_Now;
    WDeque<PersistentCrossData> m_Crosses;
    WDeque<PersistentSphereData> m_Spheres;
    WDeque<PersistentBoxData> m_Boxes;
    WDeque<PersistentLineData> m_Lines;
    WDeque<PersistentInfoTextData> m_InfoText;
  };

  static WHashTable<WDebugRendererContext, PersistentPerContextData> s_PersistentPerContextData;

} // namespace

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    WDebugRenderer::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    WDebugRenderer::OnEngineShutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
void WDebugRenderer::DrawLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform /*= WMat4::MakeIdentity()*/)
{
  if (lines.IsEmpty())
    return;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const WVec3* pPositions = &line.m_start;
    const WColor* pColors = &line.m_startColor;

    for (WUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_lineVertices.ExpandAndGetRef();
      vertex.m_position = mTransform.m_Mat4.TransformPosition(pPositions[i]);
      vertex.m_color = pColors[i] * color;
    }
  }
}

void WDebugRenderer::DrawLinesOccluded(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform /*= WMat4::MakeIdentity()*/)
{
  if (lines.IsEmpty())
    return;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const WVec3* pPositions = &line.m_start;
    const WColor* pColors = &line.m_startColor;

    for (WUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_lineOccludedVertices.ExpandAndGetRef();
      vertex.m_position = mTransform.m_Mat4.TransformPosition(pPositions[i]);
      vertex.m_color = pColors[i] * color;
    }
  }
}

void WDebugRenderer::Draw2DLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color)
{
  if (lines.IsEmpty())
    return;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const WVec3* pPositions = &line.m_start;
    const WColor* pColors = &line.m_startColor;

    for (WUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_line2DVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = pColors[i] * color;
    }
  }
}

// static
void WDebugRenderer::DrawCross(const WDebugRendererContext& context, const WVec3& vGlobalPosition, float fLineLength, const WColor& color, WMatOrTransform mTransform0 /*= WMat4::MakeIdentity()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const WMat4& transform = mTransform0.m_Mat4;

  const float fHalfLineLength = fLineLength * 0.5f;
  const WVec3 xAxis = WVec3::MakeAxisX() * fHalfLineLength;
  const WVec3 yAxis = WVec3::MakeAxisY() * fHalfLineLength;
  const WVec3 zAxis = WVec3::MakeAxisZ() * fHalfLineLength;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - xAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + xAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - yAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + yAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - zAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + zAxis), color});
}

// static
void WDebugRenderer::DrawLineBox(const WDebugRendererContext& context, const WBoundingBox& box, const WColor& color, WMatOrTransform mTransform0)
{
  W_LOCK(s_Mutex);

  const WMat4& transform = mTransform0.m_Mat4;

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  WTransform boxTransform(box.GetCenter(), WQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform.GetAsMat4();
  boxData.m_color = color;
}

// static
void WDebugRenderer::DrawLineBoxCorners(const WDebugRendererContext& context, const WBoundingBox& box, float fCornerFraction, const WColor& color, WMatOrTransform mTransform0)
{
  const WMat4& transform = mTransform0.m_Mat4;

  fCornerFraction = WMath::Clamp(fCornerFraction, 0.0f, 1.0f) * 0.5f;

  WVec3 corners[8];
  box.GetCorners(corners);

  for (WUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = transform * corners[i];
  }

  WVec3 edgeEnds[12];
  edgeEnds[0] = corners[1];  // 0 -> 1
  edgeEnds[1] = corners[3];  // 1 -> 3
  edgeEnds[2] = corners[0];  // 2 -> 0
  edgeEnds[3] = corners[2];  // 3 -> 2
  edgeEnds[4] = corners[5];  // 4 -> 5
  edgeEnds[5] = corners[7];  // 5 -> 7
  edgeEnds[6] = corners[4];  // 6 -> 4
  edgeEnds[7] = corners[6];  // 7 -> 6
  edgeEnds[8] = corners[4];  // 0 -> 4
  edgeEnds[9] = corners[5];  // 1 -> 5
  edgeEnds[10] = corners[6]; // 2 -> 6
  edgeEnds[11] = corners[7]; // 3 -> 7

  WDebugRendererLine lines[24];
  for (WUInt32 i = 0; i < 12; ++i)
  {
    WVec3 edgeStart = corners[i % 8];
    WVec3 edgeEnd = edgeEnds[i];
    WVec3 edgeDir = edgeEnd - edgeStart;

    lines[i * 2 + 0].m_start = edgeStart;
    lines[i * 2 + 0].m_end = edgeStart + edgeDir * fCornerFraction;

    lines[i * 2 + 1].m_start = edgeEnd;
    lines[i * 2 + 1].m_end = edgeEnd - edgeDir * fCornerFraction;
  }

  DrawLines(context, lines, color);
}

// static
void WDebugRenderer::DrawLineSphere(const WDebugRendererContext& context, const WBoundingSphere& sphere, const WColor& color, WMatOrTransform mTransform0 /*= WMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const WVec3 vCenter = sphere.m_vCenter;
  const float fRadius = sphere.m_fRadius;
  const WAngle stepAngle = WAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  const WMat4& transform = mTransform0.m_Mat4;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (WUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = WMath::Cos(fS1 * stepAngle);
    const float fCos2 = WMath::Cos(fS2 * stepAngle);

    const float fSin1 = WMath::Sin(fS1 * stepAngle);
    const float fSin2 = WMath::Sin(fS2 * stepAngle);

    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(0.0f, fCos1, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(0.0f, fCos2, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(fCos1, 0.0f, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(fCos2, 0.0f, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(fCos1, fSin1, 0.0f) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + WVec3(fCos2, fSin2, 0.0f) * fRadius), color});
  }
}


void WDebugRenderer::DrawLineCapsuleZ(const WDebugRendererContext& context, float fLength, float fRadius, const WColor& color, WMatOrTransform mTransform0 /*= WMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const WMat4& transform = mTransform0.m_Mat4;

  const WAngle stepAngle = WAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  WDebugRendererLine lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  WUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_start = transform * WVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (WUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = WMath::Cos(fS1 * stepAngle);
    const float fCos2 = WMath::Cos(fS2 * stepAngle);

    const float fSin1 = WMath::Sin(fS1 * stepAngle);
    const float fSin2 = WMath::Sin(fS2 * stepAngle);

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  // render top and bottom half circles
  for (WUInt32 s = 0; s < NUM_HALF_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = WMath::Cos(fS1 * stepAngle);
    const float fCos2 = WMath::Cos(fS2 * stepAngle);

    const float fSin1 = WMath::Sin(fS1 * stepAngle);
    const float fSin2 = WMath::Sin(fS2 * stepAngle);

    // top two bows
    lines[curLine].m_start = transform * WVec3(0.0f, fCos1 * fRadius, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * WVec3(0.0f, fCos2 * fRadius, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, 0.0f, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, 0.0f, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    // bottom two bows
    lines[curLine].m_start = transform * WVec3(0.0f, fCos1 * fRadius, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * WVec3(0.0f, fCos2 * fRadius, -fSin2 * fRadius - fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, 0.0f, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, 0.0f, -fSin2 * fRadius - fOffsetZ);
    ++curLine;
  }

  W_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

void WDebugRenderer::DrawLineCylinderZ(const WDebugRendererContext& context, float fLength, float fRadius, const WColor& color, WMatOrTransform mTransform0 /*= WMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const WMat4& transform = mTransform0.m_Mat4;

  const WAngle stepAngle = WAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  WDebugRendererLine lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  WUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_start = transform * WVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * WVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_end = transform * WVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (WUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = WMath::Cos(fS1 * stepAngle);
    const float fCos2 = WMath::Cos(fS2 * stepAngle);

    const float fSin1 = WMath::Sin(fS1 * stepAngle);
    const float fSin2 = WMath::Sin(fS2 * stepAngle);

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * WVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_end = transform * WVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  W_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

// static
void WDebugRenderer::DrawLineFrustum(const WDebugRendererContext& context, const WFrustum& frustum, const WColor& color, bool bDrawPlaneNormals /*= false*/)
{
  WVec3 cornerPoints[8];
  if (frustum.ComputeCornerPoints(cornerPoints).Failed())
    return;

  WDebugRendererLine lines[12] = {
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomLeft]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomRight], cornerPoints[WFrustum::FrustumCorner::FarBottomRight]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopLeft], cornerPoints[WFrustum::FrustumCorner::FarTopLeft]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopRight], cornerPoints[WFrustum::FrustumCorner::FarTopRight]),

    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomLeft], cornerPoints[WFrustum::FrustumCorner::NearBottomRight]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomRight], cornerPoints[WFrustum::FrustumCorner::NearTopRight]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopRight], cornerPoints[WFrustum::FrustumCorner::NearTopLeft]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopLeft], cornerPoints[WFrustum::FrustumCorner::NearBottomLeft]),

    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomRight]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomRight], cornerPoints[WFrustum::FrustumCorner::FarTopRight]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopRight], cornerPoints[WFrustum::FrustumCorner::FarTopLeft]),
    WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomLeft]),
  };

  DrawLines(context, lines, color);

  if (bDrawPlaneNormals)
  {
    WColor normalColor = color + WColor(0.4f, 0.4f, 0.4f);
    float fDrawLength = 0.5f;

    const WVec3 nearPlaneNormal = frustum.GetPlane(0).m_vNormal * fDrawLength;
    const WVec3 farPlaneNormal = frustum.GetPlane(1).m_vNormal * fDrawLength;
    const WVec3 leftPlaneNormal = frustum.GetPlane(2).m_vNormal * fDrawLength;
    const WVec3 rightPlaneNormal = frustum.GetPlane(3).m_vNormal * fDrawLength;
    const WVec3 bottomPlaneNormal = frustum.GetPlane(4).m_vNormal * fDrawLength;
    const WVec3 topPlaneNormal = frustum.GetPlane(5).m_vNormal * fDrawLength;

    WDebugRendererLine normalLines[24] = {
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomLeft], cornerPoints[WFrustum::FrustumCorner::NearBottomLeft] + nearPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomRight], cornerPoints[WFrustum::FrustumCorner::NearBottomRight] + nearPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopLeft], cornerPoints[WFrustum::FrustumCorner::NearTopLeft] + nearPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopRight], cornerPoints[WFrustum::FrustumCorner::NearTopRight] + nearPlaneNormal),

      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomLeft] + farPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomRight], cornerPoints[WFrustum::FrustumCorner::FarBottomRight] + farPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopLeft], cornerPoints[WFrustum::FrustumCorner::FarTopLeft] + farPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopRight], cornerPoints[WFrustum::FrustumCorner::FarTopRight] + farPlaneNormal),

      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomLeft], cornerPoints[WFrustum::FrustumCorner::NearBottomLeft] + leftPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopLeft], cornerPoints[WFrustum::FrustumCorner::NearTopLeft] + leftPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomLeft] + leftPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopLeft], cornerPoints[WFrustum::FrustumCorner::FarTopLeft] + leftPlaneNormal),

      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomRight], cornerPoints[WFrustum::FrustumCorner::NearBottomRight] + rightPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopRight], cornerPoints[WFrustum::FrustumCorner::NearTopRight] + rightPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomRight], cornerPoints[WFrustum::FrustumCorner::FarBottomRight] + rightPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopRight], cornerPoints[WFrustum::FrustumCorner::FarTopRight] + rightPlaneNormal),

      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomLeft], cornerPoints[WFrustum::FrustumCorner::NearBottomLeft] + bottomPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearBottomRight], cornerPoints[WFrustum::FrustumCorner::NearBottomRight] + bottomPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomLeft], cornerPoints[WFrustum::FrustumCorner::FarBottomLeft] + bottomPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarBottomRight], cornerPoints[WFrustum::FrustumCorner::FarBottomRight] + bottomPlaneNormal),

      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopLeft], cornerPoints[WFrustum::FrustumCorner::NearTopLeft] + topPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::NearTopRight], cornerPoints[WFrustum::FrustumCorner::NearTopRight] + topPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopLeft], cornerPoints[WFrustum::FrustumCorner::FarTopLeft] + topPlaneNormal),
      WDebugRendererLine(cornerPoints[WFrustum::FrustumCorner::FarTopRight], cornerPoints[WFrustum::FrustumCorner::FarTopRight] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void WDebugRenderer::DrawSolidBox(const WDebugRendererContext& context, const WBoundingBox& box, const WColor& color, WMatOrTransform mTransform0)
{
  const WMat4& transform = mTransform0.m_Mat4;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_solidBoxes.ExpandAndGetRef();

  WTransform boxTransform(box.GetCenter(), WQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform.GetAsMat4();
  boxData.m_color = color;
}

// static
void WDebugRenderer::DrawSolidTriangles(const WDebugRendererContext& context, WArrayPtr<WDebugRendererTriangle> triangles, const WColor& color, bool bTwoSided)
{
  if (triangles.IsEmpty())
    return;

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& triangle : triangles)
  {
    const WColorLinearUB col = triangle.m_color * color;

    for (WUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.m_triangleVertices.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_color = col;
    }
  }

  if (bTwoSided)
  {
    for (auto& triangle : triangles)
    {
      const WColorLinearUB col = triangle.m_color * color;

      auto& v1 = data.m_triangleVertices.ExpandAndGetRef();
      auto& v2 = data.m_triangleVertices.ExpandAndGetRef();
      auto& v3 = data.m_triangleVertices.ExpandAndGetRef();

      v1.m_position = triangle.m_position[0];
      v1.m_color = col;
      v2.m_position = triangle.m_position[2];
      v2.m_color = col;
      v3.m_position = triangle.m_position[1];
      v3.m_color = col;
    }
  }
}

void WDebugRenderer::DrawTexturedTriangles(const WDebugRendererContext& context, WArrayPtr<WDebugRendererTexturedTriangle> triangles, const WColor& color, const WTexture2DResourceHandle& hTexture, bool bTwoSided)
{
  if (triangles.IsEmpty())
    return;

  WResourceLock<WTexture2DResource> pTexture(hTexture, WResourceAcquireMode::AllowLoadingFallback);
  auto hGalTexture = pTexture->GetGALTexture();

  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context).m_texTriangle3DVertices[hGalTexture];

  for (auto& triangle : triangles)
  {
    const WColorLinearUB col = triangle.m_color * color;

    for (WUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_texCoord = triangle.m_texcoord[i];
      vertex.m_color = col;
    }
  }

  if (bTwoSided)
  {
    for (auto& triangle : triangles)
    {
      const WColorLinearUB col = triangle.m_color * color;

      auto& v1 = data.ExpandAndGetRef();
      auto& v2 = data.ExpandAndGetRef();
      auto& v3 = data.ExpandAndGetRef();

      v1.m_position = triangle.m_position[0];
      v1.m_texCoord = triangle.m_texcoord[0];
      v1.m_color = col;
      v2.m_position = triangle.m_position[2];
      v2.m_texCoord = triangle.m_texcoord[2];
      v2.m_color = col;
      v3.m_position = triangle.m_position[1];
      v3.m_texCoord = triangle.m_texcoord[1];
      v3.m_color = col;
    }
  }
}

void WDebugRenderer::Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color)
{
  Vertex vertices[6];

  vertices[0].m_position = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[1].m_position = WVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_position = WVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[3].m_position = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[4].m_position = WVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[5].m_position = WVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_triangle2DVertices.PushBackRange(WMakeArrayPtr(vertices));
}

void WDebugRenderer::Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color, const WTexture2DResourceHandle& hTexture, WVec2 vScale)
{
  WResourceLock<WTexture2DResource> pTexture(hTexture, WResourceAcquireMode::AllowLoadingFallback);
  Draw2DRectangle(context, rectInPixel, fDepth, color, pTexture->GetGALTexture(), vScale);
}

void WDebugRenderer::Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color, WGALTextureHandle hResourceView, WVec2 vScale)
{
  TexVertex vertices[6];

  vertices[0].m_position = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[0].m_texCoord = WVec2(0, 0).CompMul(vScale);
  vertices[1].m_position = WVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[1].m_texCoord = WVec2(1, 1).CompMul(vScale);
  vertices[2].m_position = WVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_texCoord = WVec2(0, 1).CompMul(vScale);
  vertices[3].m_position = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[3].m_texCoord = WVec2(0, 0).CompMul(vScale);
  vertices[4].m_position = WVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[4].m_texCoord = WVec2(1, 0).CompMul(vScale);
  vertices[5].m_position = WVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[5].m_texCoord = WVec2(1, 1).CompMul(vScale);

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_texTriangle2DVertices[hResourceView].PushBackRange(WMakeArrayPtr(vertices));
}

void WDebugRenderer::Draw2DLineRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color)
{
  WDebugRendererLine lines[4];

  lines[0].m_start = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  lines[0].m_end = WVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);

  lines[1].m_start = lines[0].m_end;
  lines[1].m_end = WVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  lines[2].m_start = lines[1].m_end;
  lines[2].m_end = WVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);

  lines[3].m_start = lines[2].m_end;
  lines[3].m_end = WVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);

  Draw2DLines(context, lines, color);
}

WUInt32 WDebugRenderer::Draw2DText(const WDebugRendererContext& context, const WFormatString& text, const WVec2I32& vPositionInPixel, const WColor& color, WUInt32 uiSizeInPixel /*= 16*/, WDebugTextHAlign::Enum horizontalAlignment /*= WDebugTextHAlign::Left*/, WDebugTextVAlign::Enum verticalAlignment /*= WDebugTextVAlign::Top*/)
{
  return AddTextLines(context, text, vPositionInPixel, uiSizeInPixel, horizontalAlignment, verticalAlignment, [=](PerContextData& ref_data, WStringView sLine, WVec2 vTopLeftCorner)
    {
    auto& textLine = ref_data.m_textLines2D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel; });
}


void WDebugRenderer::DrawInfoText(const WDebugRendererContext& context, WDebugTextPlacement::Enum placement, WStringView sGroupName, const WFormatString& text, const WColor& color)
{
  W_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  WStringBuilder tmp;

  auto& e = data.m_infoTextData[(int)placement].ExpandAndGetRef();
  e.m_group = sGroupName;
  e.m_text = text.GetText(tmp);
  e.m_color = color;
}

WUInt32 WDebugRenderer::Draw3DText(const WDebugRendererContext& context, const WFormatString& text, const WVec3& vGlobalPosition, const WColor& color, WUInt32 uiSizeInPixel /*= 16*/, WDebugTextHAlign::Enum horizontalAlignment /*= WDebugTextHAlign::Center*/, WDebugTextVAlign::Enum verticalAlignment /*= WDebugTextVAlign::Bottom*/)
{
  return AddTextLines(context, text, WVec2I32(0), uiSizeInPixel, horizontalAlignment, verticalAlignment, [&](PerContextData& ref_data, WStringView sLine, WVec2 vTopLeftCorner)
    {
    auto& textLine = ref_data.m_textLines3D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel;
    textLine.m_position = vGlobalPosition; });
}

void WDebugRenderer::AddPersistentCross(const WDebugRendererContext& context, float fSize, const WColor& color, WMatOrTransform mTransform, WTime duration)
{
  W_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Crosses.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_fSize = fSize;
  item.m_Timeout = data.m_Now + duration;
}

void WDebugRenderer::AddPersistentLineSphere(const WDebugRendererContext& context, float fRadius, const WColor& color, WMatOrTransform mTransform, WTime duration)
{
  W_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Spheres.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_fRadius = fRadius;
  item.m_Timeout = data.m_Now + duration;
}

void WDebugRenderer::AddPersistentLineBox(const WDebugRendererContext& context, const WVec3& vHalfSize, const WColor& color, WMatOrTransform mTransform, WTime duration)
{
  W_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Boxes.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_vHalfSize = vHalfSize;
  item.m_Timeout = data.m_Now + duration;
}

void WDebugRenderer::AddPersistentLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform, WTime duration)
{
  W_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Lines.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color = color;
  item.m_Lines = lines;
  item.m_Timeout = data.m_Now + duration;
}


void WDebugRenderer::AddPersistentInfoText(const WDebugRendererContext& context, WDebugTextPlacement::Enum placement, const WFormatString& text, WTime duration, const WColor& color /*= WColor::White*/)
{
  W_LOCK(s_Mutex);

  WStringBuilder tmp;

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_InfoText.ExpandAndGetRef();
  item.m_sText = text.GetText(tmp);
  item.m_Placement = placement;
  item.m_Color = color;
  item.m_Timeout = data.m_Now + duration;
}

void WDebugRenderer::DrawAngle(const WDebugRendererContext& context, WAngle startAngle, WAngle endAngle, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform0, WVec3 vForwardAxis /*= WVec3::MakeAxisX()*/, WVec3 vRotationAxis /*= WVec3::MakeAxisZ()*/)
{
  const WMat4& transform = mTransform0.m_Mat4;

  WTempHybridArray<WDebugRendererTriangle, 64> tris;
  WTempHybridArray<WDebugRendererLine, 64> lines;

  startAngle.NormalizeRange();
  endAngle.NormalizeRange();

  if (startAngle > endAngle)
    startAngle -= WAngle::MakeFromDegree(360);

  const WAngle range = endAngle - startAngle;
  const WUInt32 uiTesselation = WMath::Max(1u, (WUInt32)(range / WAngle::MakeFromDegree(5)));
  const WAngle step = range / (float)uiTesselation;

  WQuat qStart = WQuat::MakeFromAxisAndAngle(vRotationAxis, startAngle);

  WQuat qStep = WQuat::MakeFromAxisAndAngle(vRotationAxis, step);

  WVec3 vCurDir = qStart * vForwardAxis;

  if (lineColor.a > 0)
  {
    WDebugRendererLine& l1 = lines.ExpandAndGetRef();
    l1.m_start.SetZero();
    l1.m_end = vCurDir;
  }

  for (WUInt32 i = 0; i < uiTesselation; ++i)
  {
    const WVec3 vNextDir = qStep * vCurDir;

    if (solidColor.a > 0)
    {
      WDebugRendererTriangle& tri1 = tris.ExpandAndGetRef();
      tri1.m_position[0] = transform.GetTranslationVector();
      tri1.m_position[1] = transform.TransformPosition(vNextDir);
      tri1.m_position[2] = transform.TransformPosition(vCurDir);

      WDebugRendererTriangle& tri2 = tris.ExpandAndGetRef();
      tri2.m_position[0] = transform.GetTranslationVector();
      tri2.m_position[1] = transform.TransformPosition(vCurDir);
      tri2.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (lineColor.a > 0)
    {
      WDebugRendererLine& l1 = lines.ExpandAndGetRef();
      l1.m_start.SetZero();
      l1.m_end = vNextDir;

      WDebugRendererLine& l2 = lines.ExpandAndGetRef();
      l2.m_start = vCurDir;
      l2.m_end = vNextDir;
    }

    vCurDir = vNextDir;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void WDebugRenderer::DrawOpeningCone(const WDebugRendererContext& context, WAngle halfAngle, const WColor& colorInside, const WColor& colorOutside, WMatOrTransform mTransform0, WVec3 vForwardAxis /*= WVec3::MakeAxisX()*/)
{
  const WMat4& transform = mTransform0.m_Mat4;

  WTempHybridArray<WDebugRendererTriangle, 64> trisInside;
  WTempHybridArray<WDebugRendererTriangle, 64> trisOutside;

  halfAngle = WMath::Clamp(halfAngle, WAngle(), WAngle::MakeFromDegree(180));

  const WAngle refAngle = halfAngle <= WAngle::MakeFromDegree(90) ? halfAngle : WAngle::MakeFromDegree(180) - halfAngle;
  const WUInt32 uiTesselation = WMath::Max(8u, (WUInt32)(refAngle / WAngle::MakeFromDegree(2)));

  const WVec3 tangentAxis = vForwardAxis.GetOrthogonalVector().GetNormalized();

  WQuat tilt = WQuat::MakeFromAxisAndAngle(tangentAxis, halfAngle);

  WQuat step = WQuat::MakeFromAxisAndAngle(vForwardAxis, WAngle::MakeFromDegree(360) / (float)uiTesselation);

  WVec3 vCurDir = tilt * vForwardAxis;

  for (WUInt32 i = 0; i < uiTesselation; ++i)
  {
    const WVec3 vNextDir = step * vCurDir;

    if (colorInside.a > 0)
    {
      WDebugRendererTriangle& tri = trisInside.ExpandAndGetRef();
      tri.m_position[0] = transform.GetTranslationVector();
      tri.m_position[1] = transform.TransformPosition(vCurDir);
      tri.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (colorOutside.a > 0)
    {
      WDebugRendererTriangle& tri = trisOutside.ExpandAndGetRef();
      tri.m_position[0] = transform.GetTranslationVector();
      tri.m_position[1] = transform.TransformPosition(vNextDir);
      tri.m_position[2] = transform.TransformPosition(vCurDir);
    }

    vCurDir = vNextDir;
  }


  DrawSolidTriangles(context, trisInside, colorInside);
  DrawSolidTriangles(context, trisOutside, colorOutside);
}

void WDebugRenderer::DrawLimitCone(const WDebugRendererContext& context, WAngle halfAngle1, WAngle halfAngle2, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform0)
{
  const WMat4& transform = mTransform0.m_Mat4;

  constexpr WUInt32 NUM_LINES = 32;
  WTempHybridArray<WDebugRendererLine, NUM_LINES * 2> lines;
  WTempHybridArray<WDebugRendererTriangle, NUM_LINES * 2> tris;

  // no clue how this works
  // copied 1:1 from NVIDIA's PhysX SDK: Cm::visualizeLimitCone
  {
    float scale = 1.0f;

    const float tanQSwingZ = WMath::Tan(halfAngle1 / 4.0f);
    const float tanQSwingY = WMath::Tan(halfAngle2 / 4.0f);

    WVec3 prev(0);
    for (WUInt32 i = 0; i <= NUM_LINES; i++)
    {
      const float angle = 2 * WMath::Pi<float>() / NUM_LINES * i;
      const float c = WMath::Cos(WAngle::MakeFromRadian(angle)), s = WMath::Sin(WAngle::MakeFromRadian(angle));
      const WVec3 rv(0, -tanQSwingZ * s, tanQSwingY * c);
      const float rv2 = rv.GetLengthSquared();
      const float r = (1 / (1 + rv2));
      const WQuat q = WQuat(0, r * 2 * rv.y, r * 2 * rv.z, r * (1 - rv2));
      const WVec3 a = q * WVec3(1.0f, 0, 0) * scale;

      if (lineColor.a > 0)
      {
        auto& l1 = lines.ExpandAndGetRef();
        l1.m_start = prev;
        l1.m_end = a;

        auto& l2 = lines.ExpandAndGetRef();
        l2.m_start.SetZero();
        l2.m_end = a;
      }

      if (solidColor.a > 0)
      {
        auto& t1 = tris.ExpandAndGetRef();
        t1.m_position[0] = transform.GetTranslationVector();
        t1.m_position[1] = transform.TransformPosition(prev);
        t1.m_position[2] = transform.TransformPosition(a);

        auto& t2 = tris.ExpandAndGetRef();
        t2.m_position[0] = transform.GetTranslationVector();
        t2.m_position[1] = transform.TransformPosition(a);
        t2.m_position[2] = transform.TransformPosition(prev);
      }

      prev = a;
    }
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void WDebugRenderer::DrawCylinder(const WDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform0, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, WBasisAxis::Enum cylinderAxis /*= WBasisAxis::PositiveX*/)
{
  const WQuat tilt = WBasisAxis::GetBasisRotation(WBasisAxis::PositiveX, cylinderAxis);
  const WMat4 transform = mTransform0.m_Mat4 * tilt.GetAsMat4();

  constexpr WUInt32 NUM_SEGMENTS = 16;
  WTempHybridArray<WDebugRendererLine, NUM_SEGMENTS * 3> lines;
  WTempHybridArray<WDebugRendererTriangle, NUM_SEGMENTS * 2 * 2> tris;

  const WAngle step = WAngle::MakeFromDegree(360) / float(NUM_SEGMENTS);
  WAngle angle = {};

  WVec3 vCurCircle(0, 1 /*WMath::Cos(angle)*/, 0 /*WMath::Sin(angle)*/);

  const bool bSolid = solidColor.a > 0;
  const bool bLine = lineColor.a > 0;

  const WVec3 vLastCircle(0, WMath::Cos(-step), WMath::Sin(-step));
  const WVec3 vLastStart = transform.TransformPosition(WVec3(0, vLastCircle.y * fRadiusStart, vLastCircle.z * fRadiusStart));
  const WVec3 vLastEnd = transform.TransformPosition(WVec3(fLength, vLastCircle.y * fRadiusEnd, vLastCircle.z * fRadiusEnd));

  for (WUInt32 i = 0; i < NUM_SEGMENTS; ++i)
  {
    angle += step;
    const WVec3 vNextCircle(0, WMath::Cos(angle), WMath::Sin(angle));

    WVec3 vCurStart = vCurCircle * fRadiusStart;
    WVec3 vNextStart = vNextCircle * fRadiusStart;

    WVec3 vCurEnd(fLength, vCurCircle.y * fRadiusEnd, vCurCircle.z * fRadiusEnd);
    WVec3 vNextEnd(fLength, vNextCircle.y * fRadiusEnd, vNextCircle.z * fRadiusEnd);

    if (bLine)
    {
      lines.PushBack({vCurStart, vNextStart});
      lines.PushBack({vCurEnd, vNextEnd});
      lines.PushBack({vCurStart, vCurEnd});
    }

    if (bSolid)
    {
      vCurStart = transform.TransformPosition(vCurStart);
      vCurEnd = transform.TransformPosition(vCurEnd);
      vNextStart = transform.TransformPosition(vNextStart);
      vNextEnd = transform.TransformPosition(vNextEnd);

      tris.PushBack({vCurStart, vNextStart, vNextEnd});
      tris.PushBack({vCurStart, vNextEnd, vCurEnd});

      if (bCapStart)
        tris.PushBack({vLastStart, vNextStart, vCurStart});

      if (bCapEnd)
        tris.PushBack({vLastEnd, vCurEnd, vNextEnd});
    }

    vCurCircle = vNextCircle;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void WDebugRenderer::DrawArrow(const WDebugRendererContext& context, float fSize, const WColor& color, WMatOrTransform mTransform, WVec3 vForwardAxis /*= WVec3::MakeAxisX()*/)
{
  vForwardAxis.Normalize();
  const WVec3 right = vForwardAxis.GetOrthogonalVector().GetNormalized();
  const WVec3 up = vForwardAxis.CrossRH(right).GetNormalized();
  const WVec3 endPoint = vForwardAxis * fSize;
  const WVec3 endPoint2 = vForwardAxis * fSize * 0.9f;
  const float tipSize = fSize * 0.1f;

  WDebugRendererLine lines[9];
  lines[0] = WDebugRendererLine(WVec3::MakeZero(), endPoint);
  lines[1] = WDebugRendererLine(endPoint, endPoint2 + right * tipSize);
  lines[2] = WDebugRendererLine(endPoint, endPoint2 + up * tipSize);
  lines[3] = WDebugRendererLine(endPoint, endPoint2 - right * tipSize);
  lines[4] = WDebugRendererLine(endPoint, endPoint2 - up * tipSize);
  lines[5] = WDebugRendererLine(lines[1].m_end, lines[2].m_end);
  lines[6] = WDebugRendererLine(lines[2].m_end, lines[3].m_end);
  lines[7] = WDebugRendererLine(lines[3].m_end, lines[4].m_end);
  lines[8] = WDebugRendererLine(lines[4].m_end, lines[1].m_end);

  DrawLines(context, lines, color, mTransform);
}

// static
float WDebugRenderer::GetTextGlyphWidth(WUInt32 uiSizeInPixel /*= 16*/)
{
  // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
  return WMath::Ceil(uiSizeInPixel * cvar_AppTextScale * (8.0f / 16.0f));
}

// static
float WDebugRenderer::GetTextLineHeight(WUInt32 uiSizeInPixel /*= 16*/)
{
  return WMath::Ceil(uiSizeInPixel * cvar_AppTextScale * (20.0f / 16.0f));
}

// static
float WDebugRenderer::GetTextScale()
{
  return cvar_AppTextScale;
}

// static
void WDebugRenderer::SetTextScale(float fScale)
{
  cvar_AppTextScale = fScale;
}

// static
void WDebugRenderer::RenderWorldSpace(const WRenderViewContext& renderViewContext)
{
  W_PROFILE_SCOPE("WDebugRenderer::RenderWorldSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void WDebugRenderer::RenderInternalWorldSpace(const WDebugRendererContext& context, const WRenderViewContext& renderViewContext)
{
  {
    W_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = WClock::GetGlobalClock()->GetLastUpdateTime();

    // persistent crosses
    {
      WUInt32 uiNumItems = data.m_Crosses.GetCount();
      for (WUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Crosses[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Crosses.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          WDebugRenderer::DrawCross(context, WVec3::MakeZero(), item.m_fSize, item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent spheres
    {
      WUInt32 uiNumItems = data.m_Spheres.GetCount();
      for (WUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Spheres[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Spheres.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          WDebugRenderer::DrawLineSphere(context, WBoundingSphere::MakeFromCenterAndRadius(WVec3::MakeZero(), item.m_fRadius), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent boxes
    {
      WUInt32 uiNumItems = data.m_Boxes.GetCount();
      for (WUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Boxes[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Boxes.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          WDebugRenderer::DrawLineBox(context, WBoundingBox::MakeFromMinMax(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent lines
    {
      WUInt32 uiNumItems = data.m_Lines.GetCount();
      for (WUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Lines[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Lines.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          WDebugRenderer::DrawLines(context, item.m_Lines.GetArrayPtr(), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
  {
    return;
  }

  PerContextData* pData = pDoubleBufferedContextData->m_pData[WRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
  {
    return;
  }

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

  WBindGroupBuilder& bindGroupRenderPass = WRenderContext::GetDefaultInstance()->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);

  // 3D Lines that show through geometry (no depth test, distance fade).
  // Rendered first so the depth-tested regular line buffer below can override the visible portions.
  // Globally disabled via the CVar 'Debug.RenderOccluded'.
  if (cvar_DebugRenderOccluded)
  {
    WUInt32 uiNumLineVertices = pData->m_lineOccludedVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_lineOccludedVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines].GetNewBuffer();
        const WUInt32 uiNumLineVerticesInBatch = WMath::Min<WUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        W_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_VertexAttributes), WGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("OCCLUDED_PASS", "FALSE");

  // 3D Lines without occlusion
  {
    WUInt32 uiNumLineVertices = pData->m_lineVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_lineVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines].GetNewBuffer();
        const WUInt32 uiNumLineVerticesInBatch = WMath::Min<WUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        W_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_VertexAttributes), WGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // LineBoxes
  {
    WUInt32 uiNumLineBoxes = pData->m_lineBoxes.GetCount();
    if (uiNumLineBoxes != 0)
    {
      CreateDataBuffer(BufferType::LineBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);

      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_lineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::LineBoxes].GetNewBuffer();
        const WUInt32 uiNumLineBoxesInBatch = WMath::Min<WUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);
        bindGroupRenderPass.BindBuffer("boxData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // SolidBoxes
  {
    WUInt32 uiNumSolidBoxes = pData->m_solidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_solidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::SolidBoxes].GetNewBuffer();
        bindGroupRenderPass.BindBuffer("boxData", hBuffer);
        const WUInt32 uiNumSolidBoxesInBatch = WMath::Min<WUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        unsigned int uiRenderedInstances = uiNumSolidBoxesInBatch;
        if (renderViewContext.m_pCamera->IsStereoscopic())
          uiRenderedInstances *= 2;

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiRenderedInstances).IgnoreResult();

        uiNumSolidBoxes -= uiNumSolidBoxesInBatch;
        pSolidBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // Triangles
  {
    WUInt32 uiNumTriangleVertices = pData->m_triangleVertices.GetCount();
    if (uiNumTriangleVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles3D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangleVertices.GetData();
      while (uiNumTriangleVertices > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Triangles3D].GetNewBuffer();

        const WUInt32 uiNumTriangleVerticesInBatch = WMath::Min<WUInt32>(uiNumTriangleVertices, TRIANGLE_VERTICES_PER_BATCH);
        W_ASSERT_DEV(uiNumTriangleVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_VertexAttributes), WGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumTriangleVertices -= uiNumTriangleVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 3D triangles
  {
    for (auto itTex = pData->m_texTriangle3DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      auto hTexture = itTex.Key();
      const auto format = pDevice->GetTexture(hTexture)->GetDescription().m_Format;
      const bool bMonochrome = WGALResourceFormat::GetChannelCount(format) == 1;

      bindGroupRenderPass.BindTexture("BaseTexture", hTexture);

      const auto& verts = itTex.Value();

      WUInt32 uiNumVertices = verts.GetCount();
      if (uiNumVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles3D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MONOCHROME", bMonochrome ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNumVertices > 0)
        {
          WGALBufferHandle hBuffer = s_DataBuffer[BufferType::TexTriangles3D].GetNewBuffer();
          const WUInt32 uiNumVerticesInBatch = WMath::Min<WUInt32>(uiNumVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          W_ASSERT_DEV(uiNumVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pTriangleData, uiNumVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

          renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_TexVertexAttributes), WGALPrimitiveTopology::Triangles, uiNumVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNumVertices -= uiNumVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // Text
  {
    pData->m_glyphs.Clear();

    for (auto& textLine : pData->m_textLines3D)
    {
      WVec3 screenPos;
      if (renderViewContext.m_pViewData->ComputeScreenSpacePos(textLine.m_position, screenPos).Succeeded() && screenPos.z > 0.0f)
      {
        renderViewContext.m_pViewData->ConvertScreenNormalizedPosToPixelPos(screenPos);

        textLine.m_topLeftCorner.x += WMath::Round(screenPos.x);
        textLine.m_topLeftCorner.y += WMath::Round(screenPos.y);

        AppendGlyphs(pData->m_glyphs, textLine);
      }
    }

    WUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);

      bindGroupRenderPass.BindTexture("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const WUInt32 uiNumGlyphsInBatch = WMath::Min<WUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        bindGroupRenderPass.BindBuffer("glyphData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

// static
void WDebugRenderer::RenderScreenSpace(const WRenderViewContext& renderViewContext)
{
  W_PROFILE_SCOPE("WDebugRenderer::RenderScreenSpace");

  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void WDebugRenderer::RenderInternalScreenSpace(const WDebugRendererContext& context, const WRenderViewContext& renderViewContext)
{
  {
    W_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = WClock::GetGlobalClock()->GetLastUpdateTime();

    // persistent info text
    {
      WUInt32 uiNumItems = data.m_InfoText.GetCount();
      for (WUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_InfoText[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_InfoText.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          WDebugRenderer::DrawInfoText(context, item.m_Placement, "__Persistent", item.m_sText.GetView(), item.m_Color);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
  {
    return;
  }

  PerContextData* pData = pDoubleBufferedContextData->m_pData[WRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
  {
    return;
  }

  // draw info text
  {
    static_assert((int)WDebugTextPlacement::ENUM_COUNT == 6);

    WDebugTextHAlign::Enum ha[(int)WDebugTextPlacement::ENUM_COUNT] = {
      WDebugTextHAlign::Left,
      WDebugTextHAlign::Center,
      WDebugTextHAlign::Right,
      WDebugTextHAlign::Left,
      WDebugTextHAlign::Center,
      WDebugTextHAlign::Right};

    WDebugTextVAlign::Enum va[(int)WDebugTextPlacement::ENUM_COUNT] = {
      WDebugTextVAlign::Top,
      WDebugTextVAlign::Top,
      WDebugTextVAlign::Top,
      WDebugTextVAlign::Bottom,
      WDebugTextVAlign::Bottom,
      WDebugTextVAlign::Bottom};

    int lineHeight = (int)GetTextLineHeight();

    WInt32 resX = (WInt32)renderViewContext.m_pViewData->m_ViewPortRect.width;
    WInt32 resY = (WInt32)renderViewContext.m_pViewData->m_ViewPortRect.height;

    WVec2I32 anchor[(int)WDebugTextPlacement::ENUM_COUNT] = {
      WVec2I32(10, 10),
      WVec2I32(resX / 2, 10),
      WVec2I32(resX - 10, 10),
      WVec2I32(10, resY - 10),
      WVec2I32(resX / 2, resY - 10),
      WVec2I32(resX - 10, resY - 10)};

    for (WUInt32 corner = 0; corner < (WUInt32)WDebugTextPlacement::ENUM_COUNT; ++corner)
    {
      auto& cd = pData->m_infoTextData[corner];

      // InsertionSort is stable
      WSorting::InsertionSort(cd, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool
        { return lhs.m_group < rhs.m_group; });

      WVec2I32 pos = anchor[corner];
      int offset = va[corner] == WDebugTextVAlign::Top ? lineHeight : -lineHeight;

      for (WUInt32 i = 0; i < cd.GetCount(); ++i)
      {
        // add some space between groups
        if (i > 0 && cd[i - 1].m_group != cd[i].m_group)
          pos.y += offset;

        pos.y += offset * Draw2DText(context, cd[i].m_text.GetData(), pos, cd[i].m_color, 16, ha[corner], va[corner]);
      }
    }
  }

  // update the frame counter
  pDoubleBufferedContextData->m_uiLastRenderedFrame = WRenderWorld::GetFrameCounter();

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  WGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

  // 2D Rectangles
  {
    WUInt32 uiNum2DVertices = pData->m_triangle2DVertices.GetCount();
    if (uiNum2DVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangle2DVertices.GetData();
      while (uiNum2DVertices > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Triangles2D].GetNewBuffer();
        const WUInt32 uiNum2DVerticesInBatch = WMath::Min<WUInt32>(uiNum2DVertices, TRIANGLE_VERTICES_PER_BATCH);
        W_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_VertexAttributes), WGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D triangles
  {
    WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
    for (auto itTex = pData->m_texTriangle2DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      auto hTexture = itTex.Key();
      const auto format = pDevice->GetTexture(hTexture)->GetDescription().m_Format;
      const bool bMonochrome = WGALResourceFormat::GetChannelCount(format) == 1;

      bindGroupRenderPass.BindTexture("BaseTexture", hTexture);

      const auto& verts = itTex.Value();

      WUInt32 uiNum2DVertices = verts.GetCount();
      if (uiNum2DVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles2D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MONOCHROME", bMonochrome ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNum2DVertices > 0)
        {
          WGALBufferHandle hBuffer = s_DataBuffer[BufferType::TexTriangles2D].GetNewBuffer();
          const WUInt32 uiNum2DVerticesInBatch = WMath::Min<WUInt32>(uiNum2DVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          W_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

          renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_TexVertexAttributes), WGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNum2DVertices -= uiNum2DVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // 2D Lines
  {
    WUInt32 uiNumLineVertices = pData->m_line2DVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_line2DVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Lines2D].GetNewBuffer();
        const WUInt32 uiNumLineVerticesInBatch = WMath::Min<WUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        W_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindMeshBuffer(WMakeArrayPtr(&hBuffer, 1), WGALBufferHandle(), WMakeArrayPtr(s_VertexAttributes), WGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // Text
  {
    WBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(W_GAL_BIND_GROUP_RENDER_PASS);
    pData->m_glyphs.Clear();

    for (auto& textLine : pData->m_textLines2D)
    {
      AppendGlyphs(pData->m_glyphs, textLine);
    }

    WUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      bindGroupRenderPass.BindTexture("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        WGALBufferHandle hBuffer = s_DataBuffer[BufferType::Glyphs].GetNewBuffer();
        const WUInt32 uiNumGlyphsInBatch = WMath::Min<WUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        bindGroupRenderPass.BindBuffer("glyphData", hBuffer);
        pGALCommandEncoder->UpdateBuffer(hBuffer, 0, WMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray(), WGALUpdateMode::AheadOfTime);

        renderViewContext.m_pRenderContext->BindNullMeshBuffer(WGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

void WDebugRenderer::OnEngineStartup()
{
  {
    WGeometry geom;
    geom.AddLineBox(WVec3(2.0f));

    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Lines);

    s_hLineBoxMeshBuffer = WResourceManager::CreateResource<WMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    // BEGIN-DOCS-CODE-SNIPPET: resource-management-create
    WGeometry geom;
    geom.AddBox(WVec3(2.0f), false);

    WMeshBufferResourceDescriptor desc;
    desc.AddStream(WMeshVertexStreamType::Position);
    desc.AllocateStreamsFromGeometry(geom, WGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = WResourceManager::CreateResource<WMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
    // END-DOCS-CODE-SNIPPET
  }

  {
    {
      WGALVertexAttribute& va = s_VertexAttributes[0];
      va.m_eSemantic = WGALVertexAttributeSemantic::Position;
      va.m_eFormat = WGALResourceFormat::XYZFloat;
      va.m_uiOffset = 0;
      va.m_uiVertexBufferSlot = 0;
    }

    {
      WGALVertexAttribute& va = s_VertexAttributes[1];
      va.m_eSemantic = WGALVertexAttributeSemantic::Color0;
      va.m_eFormat = WGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = 12;
      va.m_uiVertexBufferSlot = 0;
    }
  }

  {
    s_TexVertexAttributes[0] = s_VertexAttributes[0];
    s_TexVertexAttributes[1] = s_VertexAttributes[1];

    {
      WGALVertexAttribute& va = s_TexVertexAttributes[2];
      va.m_eSemantic = WGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = WGALResourceFormat::XYFloat;
      va.m_uiOffset = 16;
      va.m_uiVertexBufferSlot = 0;
    }
  }

  {
    WImage debugFontImage;
    WGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);
    debugFontImage.Convert(WImageFormat::R8_UNORM).AssertSuccess();

    WGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = debugFontImage.GetByteBlobPtr();
    memoryDesc.m_uiRowPitch = static_cast<WUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiSlicePitch = static_cast<WUInt32>(debugFontImage.GetDepthPitch());

    WTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = WGALResourceFormat::RUByteNormalized;
    desc.m_DescGAL.m_ResourceAccess.m_bImmutable = true;
    desc.m_InitialContent = WMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = WResourceManager::CreateResource<WTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Debug/DebugGeometry.WShader");
  s_hDebugPrimitiveShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Debug/DebugPrimitive.WShader");
  s_hDebugTexturedPrimitiveShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Debug/DebugTexturedPrimitive.WShader");
  s_hDebugTextShader = WResourceManager::LoadResource<WShaderResource>("Shaders/Debug/DebugText.WShader");

  WRenderWorld::GetRenderEvent().AddEventHandler(&OnRenderEvent);
}

void WDebugRenderer::OnEngineShutdown()
{
  WRenderWorld::GetRenderEvent().RemoveEventHandler(&OnRenderEvent);

  for (WUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyBuffer(static_cast<BufferType::Enum>(i));
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTexturedPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();

  s_PerContextData.Clear();

  s_PersistentPerContextData.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Debug, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetResolution),

    W_SCRIPT_FUNCTION_PROPERTY(DrawCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(0.1f)),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute())),

    W_SCRIPT_FUNCTION_PROPERTY(DrawLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(WVec3(1))),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute())),

    W_SCRIPT_FUNCTION_PROPERTY(DrawLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(1.0f)),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute())),

    W_SCRIPT_FUNCTION_PROPERTY(DrawSolidBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(WVec3(1))),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute())),

    W_SCRIPT_FUNCTION_PROPERTY(Draw2DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel", In, "HAlign")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(16))),

    W_SCRIPT_FUNCTION_PROPERTY(Draw3DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(16))),

    W_SCRIPT_FUNCTION_PROPERTY(DrawInfoText, In, "World", In, "Text", In, "Placement", In, "Group", In, "Color"),

    W_SCRIPT_FUNCTION_PROPERTY(AddPersistentCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(0.1f)),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(WTime::MakeFromSeconds(1)))),

    W_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(WVec3(1))),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(WTime::MakeFromSeconds(1)))),

    W_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new WFunctionArgumentAttributes(2, new WDefaultValueAttribute(1.0f)),
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(WTime::MakeFromSeconds(1)))),

    W_SCRIPT_FUNCTION_PROPERTY(DrawLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(4, new WExposeColorAlphaAttribute())),

    W_SCRIPT_FUNCTION_PROPERTY(Draw2DLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WExposeColorAlphaAttribute()),
      new WFunctionArgumentAttributes(4, new WExposeColorAlphaAttribute())),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Debug"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WVec2 WScriptExtensionClass_Debug::GetResolution()
{
  for (const WViewHandle& hView : WRenderWorld::GetMainViews())
  {
    WView* pView;
    if (WRenderWorld::TryGetView(hView, pView))
    {
      return WVec2(pView->GetViewport().width, pView->GetViewport().height);
    }
  }

  return WVec2::MakeZero();
}

// static
void WScriptExtensionClass_Debug::DrawCross(const WWorld* pWorld, const WVec3& vPosition, float fSize, const WColor& color, const WTransform& transform)
{
  WDebugRenderer::DrawCross(pWorld, vPosition, fSize, color, transform);
}

// static
void WScriptExtensionClass_Debug::DrawLineBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform)
{
  WDebugRenderer::DrawLineBox(pWorld, WBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void WScriptExtensionClass_Debug::DrawLineSphere(const WWorld* pWorld, const WVec3& vPosition, float fRadius, const WColor& color, const WTransform& transform)
{
  WDebugRenderer::DrawLineSphere(pWorld, WBoundingSphere::MakeFromCenterAndRadius(vPosition, fRadius), color, transform);
}

// static
void WScriptExtensionClass_Debug::DrawSolidBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform)
{
  WDebugRenderer::DrawSolidBox(pWorld, WBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void WScriptExtensionClass_Debug::Draw2DText(const WWorld* pWorld, WStringView sText, const WVec3& vPosition, const WColor& color, WUInt32 uiSizeInPixel, WEnum<WDebugTextHAlign> horizontalAlignment)
{
  WVec2I32 vPositionInPixel = WVec2I32(static_cast<int>(WMath::Round(vPosition.x)), static_cast<int>(WMath::Round(vPosition.y)));
  WDebugRenderer::Draw2DText(pWorld, sText, vPositionInPixel, color, uiSizeInPixel, horizontalAlignment);
}

// static
void WScriptExtensionClass_Debug::Draw3DText(const WWorld* pWorld, WStringView sText, const WVec3& vPosition, const WColor& color, WUInt32 uiSizeInPixel)
{
  WDebugRenderer::Draw3DText(pWorld, sText, vPosition, color, uiSizeInPixel);
}

// static
void WScriptExtensionClass_Debug::DrawInfoText(const WWorld* pWorld, WStringView sText, WEnum<WDebugTextPlacement> placement, WStringView sGroupName, const WColor& color)
{
  WDebugRenderer::DrawInfoText(pWorld, placement, sGroupName, sText, color);
}

// static
void WScriptExtensionClass_Debug::AddPersistentCross(const WWorld* pWorld, const WVec3& vPosition, float fSize, const WColor& color, const WTransform& transform, WTime duration)
{
  WTransform t = transform;
  t.m_vPosition += vPosition;

  WDebugRenderer::AddPersistentCross(pWorld, fSize, color, t, duration);
}

// static
void WScriptExtensionClass_Debug::AddPersistentLineBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform, WTime duration)
{
  WTransform t = transform;
  t.m_vPosition += vPosition;

  WDebugRenderer::AddPersistentLineBox(pWorld, vHalfExtents, color, t, duration);
}

// static
void WScriptExtensionClass_Debug::AddPersistentLineSphere(const WWorld* pWorld, const WVec3& vPosition, float fRadius, const WColor& color, const WTransform& transform, WTime duration)
{
  WTransform t = transform;
  t.m_vPosition += vPosition;

  WDebugRenderer::AddPersistentLineSphere(pWorld, fRadius, color, t, duration);
}

void WScriptExtensionClass_Debug::DrawLine(const WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, const WColor& startColor, const WColor& endColor)
{
  WDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  WDebugRenderer::DrawLines(pWorld, line, WColor::White);
}

void WScriptExtensionClass_Debug::Draw2DLine(const WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, const WColor& startColor, const WColor& endColor)
{
  WDebugRendererLine line[1];
  line[0].m_start = vStart;
  line[0].m_end = vEnd;
  line[0].m_startColor = startColor;
  line[0].m_endColor = endColor;

  WDebugRenderer::Draw2DLines(pWorld, line, WColor::White);
}

W_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugRenderer);
