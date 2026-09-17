#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCollectionAssetDocumentManager, 1, WRTTIDefaultAllocator<WCollectionAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WCollectionAssetDocumentManager::WCollectionAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Collection";
  m_DocTypeDesc.m_sFileExtension = "WCollectionAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Collection.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AssetCollection");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinCollection";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Collection", QPixmap(":/AssetIcons/Collection.svg"));
}

WCollectionAssetDocumentManager::~WCollectionAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}


void WCollectionAssetDocumentManager::GetAssetTypesRequiringTransformForSceneExport(WSet<WTempHashedString>& inout_assetTypes)
{
  inout_assetTypes.Insert(WTempHashedString(m_DocTypeDesc.m_sDocumentTypeName));
}

void WCollectionAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WCollectionAssetDocument>())
      {
        new WQtCollectionAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WCollectionAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WCollectionAssetDocument(sPath);
}

void WCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
