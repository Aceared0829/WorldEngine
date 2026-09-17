#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Utils/CoreRenderProfile.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCoreRenderProfileConfig, 1, WRTTIDefaultAllocator<WCoreRenderProfileConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ShadowAtlasTextureSize", m_uiShadowAtlasTextureSize)->AddAttributes(new WDefaultValueAttribute(4096), new WClampValueAttribute(512, 8192)),
    W_MEMBER_PROPERTY("MaxShadowMapSize", m_uiMaxShadowMapSize)->AddAttributes(new WDefaultValueAttribute(1024), new WClampValueAttribute(64, 4096)),
    W_MEMBER_PROPERTY("MinShadowMapSize", m_uiMinShadowMapSize)->AddAttributes(new WDefaultValueAttribute(64), new WClampValueAttribute(8, 512)),
    W_MEMBER_PROPERTY("RuntimeDecalAtlasTextureSize", m_uiRuntimeDecalAtlasTextureSize)->AddAttributes(new WDefaultValueAttribute(3072), new WClampValueAttribute(512, 8192)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WCoreRenderProfileConfig::SaveRuntimeData(WChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("WCoreRenderProfileConfig", 2);

  inout_stream << m_uiShadowAtlasTextureSize;
  inout_stream << m_uiMaxShadowMapSize;
  inout_stream << m_uiMinShadowMapSize;
  inout_stream << m_uiRuntimeDecalAtlasTextureSize;

  inout_stream.EndChunk();
}

void WCoreRenderProfileConfig::LoadRuntimeData(WChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "WCoreRenderProfileConfig" && chunk.m_uiChunkVersion >= 1)
  {
    inout_stream >> m_uiShadowAtlasTextureSize;
    inout_stream >> m_uiMaxShadowMapSize;
    inout_stream >> m_uiMinShadowMapSize;

    if (chunk.m_uiChunkVersion >= 2)
    {
      inout_stream >> m_uiRuntimeDecalAtlasTextureSize;
    }
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_CoreRenderProfile);
