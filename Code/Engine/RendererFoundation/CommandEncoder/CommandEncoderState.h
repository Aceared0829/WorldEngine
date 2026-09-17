
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>


struct W_RENDERERFOUNDATION_DLL WGALCommandEncoderRenderState final
{
  void InvalidateState();
  WGALBufferHandle m_hVertexBuffers[W_GAL_MAX_VERTEX_BUFFER_COUNT];
  WUInt32 m_hVertexBufferOffsets[W_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  WGALBufferHandle m_hIndexBuffer;

  WGALGraphicsPipelineHandle m_hGraphicsPipeline;
  WGALComputePipelineHandle m_hComputePipeline;
  WUInt8 m_uiStencilRefValue = 0;

  WRectU32 m_ScissorRect = WRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  WRectFloat m_ViewPortRect = WRectFloat(WMath::MaxValue<float>(), WMath::MaxValue<float>(), 0.0f, 0.0f);
  float m_fViewPortMinDepth = WMath::MaxValue<float>();
  float m_fViewPortMaxDepth = -WMath::MaxValue<float>();
};

struct W_RENDERERFOUNDATION_DLL WGALCommandEncoderStats
{
  void Reset();
  void SetStatistics();
  void operator+=(const WGALCommandEncoderStats& rhs);

  // Operations
  WUInt32 m_uiInsertTimestamp = 0;
  WUInt32 m_uiBeginOcclusionQuery = 0;
  WUInt32 m_uiInsertFence = 0;
  WUInt32 m_uiCopyBuffer = 0;
  WUInt32 m_uiUpdateBuffer = 0;
  WUInt32 m_uiCopyTexture = 0;
  WUInt32 m_uiUpdateTexture = 0;
  WUInt32 m_uiResolveTexture = 0;
  WUInt32 m_uiReadbackTexture = 0;
  WUInt32 m_uiReadbackBuffer = 0;
  WUInt32 m_uiFlush = 0;
  // Compute
  WUInt32 m_uiBeginCompute = 0;
  WUInt32 m_uiDispatch = 0;
  // Rendering
  WUInt32 m_uiBeginRendering = 0;
  WUInt32 m_uiClear = 0;
  WUInt32 m_uiDraw = 0;
  // State Changes
  WUInt32 m_uiSetIndexBuffer = 0;
  WUInt32 m_uiSetVertexBuffer = 0;
  WUInt32 m_uiSetGraphicsPipeline = 0;
  WUInt32 m_uiSetComputePipeline = 0;
  // Dynamic State Changes
  WUInt32 m_uiSetViewport = 0;
  WUInt32 m_uiSetScissorRect = 0;
  WUInt32 m_uiSetStencilReference = 0;
};
