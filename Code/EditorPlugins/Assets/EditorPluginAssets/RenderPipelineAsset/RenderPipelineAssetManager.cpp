#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderPipelineAssetManager, 1, WRTTIDefaultAllocator<WRenderPipelineAssetManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WRenderPipelineAssetManager::WRenderPipelineAssetManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RenderPipeline";
  m_DocTypeDesc.m_sFileExtension = "WRenderPipelineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RenderPipeline.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WRenderPipelineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_RenderPipeline");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinRenderPipeline";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("RenderPipeline", QPixmap(":/AssetIcons/RenderPipeline.svg"));
}

WRenderPipelineAssetManager::~WRenderPipelineAssetManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WRenderPipelineAssetManager::OnDocumentManagerEvent, this));
}

void WRenderPipelineAssetManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WRenderPipelineAssetDocument>())
      {
        new WQtRenderPipelineAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WRenderPipelineAssetManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WRenderPipelineAssetDocument(sPath);
}

void WRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
