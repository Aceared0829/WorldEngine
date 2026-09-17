#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLUTAssetDocumentManager, 1, WRTTIDefaultAllocator<WLUTAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WLUTAssetDocumentManager::WLUTAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WLUTAssetDocumentManager::OnDocumentManagerEvent, this));

  // LUT asset source files
  WAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "cube");

  m_DocTypeDesc.m_sDocumentTypeName = "LUT";
  m_DocTypeDesc.m_sFileExtension = "WLUTAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/LUT.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WLUTAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "WBinLUT";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::None;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_3D");

  WQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/LUT.svg"));

  // WQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/Render_Target.svg"));
}

WLUTAssetDocumentManager::~WLUTAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WLUTAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WLUTAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WLUTAssetDocument>())
      {
        new WQtLUTAssetDocumentWindow(static_cast<WLUTAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WLUTAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  WLUTAssetDocument* pDoc = new WLUTAssetDocument(sPath);
  out_pDocument = pDoc;
}

void WLUTAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
