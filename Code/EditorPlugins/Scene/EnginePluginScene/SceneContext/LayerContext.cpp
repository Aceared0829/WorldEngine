#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerContext, 1, WRTTIDefaultAllocator<WLayerContext>)
{
  W_BEGIN_PROPERTIES
  {
    W_CONSTANT_PROPERTY("DocumentType", (const char*) "Layer"),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(AllocateContext),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEngineProcessDocumentContext* WLayerContext::AllocateContext(const WDocumentOpenMsgToEngine* pMsg)
{
  if (pMsg->m_DocumentMetaData.IsA<WUuid>())
  {
    return WGetStaticRTTI<WLayerContext>()->GetAllocator()->Allocate<WEngineProcessDocumentContext>();
  }
  else
  {
    return WGetStaticRTTI<WSceneContext>()->GetAllocator()->Allocate<WEngineProcessDocumentContext>();
  }
}

WLayerContext::WLayerContext()
  : WEngineProcessDocumentContext(WEngineProcessDocumentContextFlags::None)
{
}

WLayerContext::~WLayerContext() = default;

void WLayerContext::HandleMessage(const WEditorEngineDocumentMsg* pMsg)
{
  // Everything in the picking buffer needs a unique ID. As layers and scene share the same world we need to make sure no id is used twice.
  // To achieve this the scene's next ID is retrieved on every change and written back in base new IDs were used up.
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  WEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;

  if (pMsg->IsInstanceOf<WEntityMsgToEngine>())
  {
    W_LOCK(m_pWorld->GetWriteMarker());
    m_pParentSceneContext->AddLayerIndexTag(*static_cast<const WEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void WLayerContext::SceneDeinitialized()
{
  // If the scene is deinitialized the world is destroyed so there is no use tracking anything further.
  m_pWorld = nullptr;
  m_Context.Clear();
}

const WTag& WLayerContext::GetLayerTag() const
{
  return m_LayerTag;
}

void WLayerContext::OnInitialize()
{
  WUuid parentScene = m_MetaData.Get<WUuid>();
  WEngineProcessDocumentContext* pContext = GetDocumentContext(parentScene);
  m_pParentSceneContext = WDynamicCast<WSceneContext*>(pContext);

  m_pWorld = m_pParentSceneContext->GetWorld();
  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  WUInt32 uiLayerID = m_pParentSceneContext->RegisterLayer(this);
  WStringBuilder sVisibilityTag;
  sVisibilityTag.SetFormat("Layer_{}", uiLayerID);
  m_LayerTag = WTagRegistry::GetGlobalRegistry().RegisterTag(sVisibilityTag);

  WShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void WLayerContext::OnDeinitialize()
{
  if (m_pWorld)
  {
    // If the world still exists we are just unloading the layer not the scene that owns the world.
    // Thus, we need to make sure the layer objects are removed from the still existing world.
    m_Context.DeleteExistingObjects();
  }

  m_LayerTag = WTag();
  m_pParentSceneContext->UnregisterLayer(this);
  m_pParentSceneContext = nullptr;
}

WEngineProcessViewContext* WLayerContext::CreateViewContext()
{
  W_REPORT_FAILURE("Layers should not create views.");
  return nullptr;
}

void WLayerContext::DestroyViewContext(WEngineProcessViewContext* pContext)
{
  W_REPORT_FAILURE("Layers should not create views.");
}

WStatus WLayerContext::ExportDocument(const WExportDocumentMsgToEngine* pMsg)
{
  W_REPORT_FAILURE("Layers do not support export yet. THe layer content is baked into the main scene instead.");
  return WStatus("Nope");
}

void WLayerContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
}
