#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WKrautTreeAssetDocumentManager, 1, WRTTIDefaultAllocator<WKrautTreeAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WKrautTreeAssetDocumentManager::WKrautTreeAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Kraut Tree";
  m_DocTypeDesc.m_sFileExtension = "WKrautTreeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  m_DocTypeDesc.m_sAssetCategory = "Terrain";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WKrautTreeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Kraut_Tree");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinKrautTree";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
}

WKrautTreeAssetDocumentManager::~WKrautTreeAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WKrautTreeAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WKrautTreeAssetDocument>())
      {
        new WQtKrautTreeAssetDocumentWindow(static_cast<WKrautTreeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WKrautTreeAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WKrautTreeAssetDocument(sPath);
}

void WKrautTreeAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
