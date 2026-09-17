#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiDocumentContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiViewContext.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiDocumentContext, 1, WRTTIDefaultAllocator<WRmlUiDocumentContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRmlUiDocumentContext::WRmlUiDocumentContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::CreateWorld)
{
}

WRmlUiDocumentContext::~WRmlUiDocumentContext() = default;

void WRmlUiDocumentContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  W_LOCK(pWorld->GetWriteMarker());

  // Preview object
  {
    WGameObjectDesc obj;
    obj.m_sName.Assign("RmlUiPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    WRmlUiCanvas2DComponent* pComponent = nullptr;
    WRmlUiCanvas2DComponent::CreateComponent(m_pMainObject, pComponent);

    pComponent->SetPassInput(false);
    pComponent->SetOnDemandUpdate(false);
    pComponent->SetAutobindBlackboards(false); // there is no blackboard that could be bound in this context

    WStringBuilder sResourceGuid;
    WConversionUtils::ToString(GetDocumentGuid(), sResourceGuid);
    m_hMainResource = WResourceManager::LoadResource<WRmlUiResource>(sResourceGuid);

    pComponent->SetRmlResource(m_hMainResource);
  }
}

WEngineProcessViewContext* WRmlUiDocumentContext::CreateViewContext()
{
  return W_DEFAULT_NEW(WRmlUiViewContext, this);
}

void WRmlUiDocumentContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_DEFAULT_DELETE(pContext);
}

bool WRmlUiDocumentContext::UpdateThumbnailViewContext(WEngineProcessViewContext* pThumbnailViewContext)
{
  W_LOCK(m_pMainObject->GetWorld()->GetWriteMarker());

  m_pMainObject->UpdateLocalBounds();
  WBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  WRmlUiViewContext* pMeshViewContext = static_cast<WRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
