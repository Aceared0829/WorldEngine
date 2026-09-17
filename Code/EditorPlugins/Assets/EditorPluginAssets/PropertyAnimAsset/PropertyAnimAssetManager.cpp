#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPropertyAnimAssetDocumentManager, 1, WRTTIDefaultAllocator<WPropertyAnimAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WPropertyAnimAssetDocumentManager::WPropertyAnimAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "PropertyAnim";
  m_DocTypeDesc.m_sFileExtension = "WPropertyAnimAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/PropertyAnim.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WPropertyAnimAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Property_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinPropertyAnim";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("PropertyAnim", QPixmap(":/AssetIcons/PropertyAnim.svg"));
}

WPropertyAnimAssetDocumentManager::~WPropertyAnimAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WPropertyAnimAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WPropertyAnimAssetDocument>())
      {
        new WQtPropertyAnimAssetDocumentWindow(static_cast<WPropertyAnimAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WPropertyAnimAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WPropertyAnimAssetDocument(sPath);
}

void WPropertyAnimAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
