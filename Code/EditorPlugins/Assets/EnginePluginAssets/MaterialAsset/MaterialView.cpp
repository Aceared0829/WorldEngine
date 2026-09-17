#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WMaterialViewContext::WMaterialViewContext(WMaterialContext* pMaterialContext)
  : WEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

WMaterialViewContext::~WMaterialViewContext() = default;

void WMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(WVec3(2.4f, -2.4f, 1.0f), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WViewHandle WMaterialViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Material Editor - View");
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  return pView->GetHandle();
}
