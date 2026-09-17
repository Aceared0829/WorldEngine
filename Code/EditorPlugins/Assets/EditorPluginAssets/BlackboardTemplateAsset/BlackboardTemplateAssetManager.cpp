#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetManager.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBlackboardTemplateAssetDocumentManager, 1, WRTTIDefaultAllocator<WBlackboardTemplateAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WBlackboardTemplateAssetDocumentManager::WBlackboardTemplateAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "BlackboardTemplate";
  m_DocTypeDesc.m_sFileExtension = "WBlackboardTemplateAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/BlackboardTemplate.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WBlackboardTemplateAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_BlackboardTemplate");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinBlackboardTemplate";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("BlackboardTemplate", QPixmap(":/AssetIcons/BlackboardTemplate.svg"));
}

WBlackboardTemplateAssetDocumentManager::~WBlackboardTemplateAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WBlackboardTemplateAssetDocument>())
      {
        new WQtBlackboardTemplateAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WBlackboardTemplateAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WBlackboardTemplateAssetDocument(sPath);
}

void WBlackboardTemplateAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
