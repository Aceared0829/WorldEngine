#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetManager.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomDataAssetDocumentManager, 1, WRTTIDefaultAllocator<WCustomDataAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WCustomDataAssetDocumentManager::WCustomDataAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "CustomData";
  m_DocTypeDesc.m_sFileExtension = "WCustomDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/CustomData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WCustomDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_CustomData"); // \todo should only be compatible with same type

  m_DocTypeDesc.m_sResourceFileExtension = "WBinCustomData";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("CustomData", QPixmap(":/AssetIcons/CustomData.svg"));
}

WCustomDataAssetDocumentManager::~WCustomDataAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WCustomDataAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WCustomDataAssetDocument>())
      {
        new WQtCustomDataAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WCustomDataAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WCustomDataAssetDocument(sPath);
}

void WCustomDataAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
