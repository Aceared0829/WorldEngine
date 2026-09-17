#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSurfaceAssetDocumentManager, 1, WRTTIDefaultAllocator<WSurfaceAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSurfaceAssetDocumentManager::WSurfaceAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Surface";
  m_DocTypeDesc.m_sFileExtension = "WSurfaceAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Surface.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WSurfaceAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Surface");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinSurface";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Surface", QPixmap(":/AssetIcons/Surface.svg"));
}

WSurfaceAssetDocumentManager::~WSurfaceAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WSurfaceAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSurfaceAssetDocument>())
      {
        new WQtSurfaceAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WSurfaceAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WSurfaceAssetDocument(sPath);
}

void WSurfaceAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
