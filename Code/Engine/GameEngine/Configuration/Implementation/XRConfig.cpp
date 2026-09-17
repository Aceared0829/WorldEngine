#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Configuration/XRConfig.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WXRConfig, 2, WRTTIDefaultAllocator<WXRConfig>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("EnableXR", m_bEnableXR),
    // HololensRenderPipeline.WRenderPipelineAsset
    W_MEMBER_PROPERTY("XRRenderPipeline", m_sXRRenderPipeline)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new WDefaultValueAttribute(WStringView("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"))),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WXRConfig::SaveRuntimeData(WChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("WXRConfig", 2);

  inout_stream << m_bEnableXR;
  inout_stream << m_sXRRenderPipeline;

  inout_stream.EndChunk();
}

void WXRConfig::LoadRuntimeData(WChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "WVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "WXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WVRConfig_1_2 : public WGraphPatch
{
public:
  WVRConfig_1_2()
    : WGraphPatch("WVRConfig", 5)
  {
  }
  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

WVRConfig_1_2 g_WVRConfig_1_2;

W_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
