#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetManager.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImageDataAssetDocumentManager, 1, WRTTIDefaultAllocator<WImageDataAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WImageDataAssetDocumentManager::WImageDataAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WImageDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Image Data";
  m_DocTypeDesc.m_sFileExtension = "WImageDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ImageData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WImageDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "WBinImageData";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_2D");
}

WImageDataAssetDocumentManager::~WImageDataAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WImageDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WImageDataAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WImageDataAssetDocument>())
      {
        new WQtImageDataAssetDocumentWindow(static_cast<WImageDataAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WImageDataAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  WImageDataAssetDocument* pDoc = new WImageDataAssetDocument(sPath);
  out_pDocument = pDoc;
}

void WImageDataAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
