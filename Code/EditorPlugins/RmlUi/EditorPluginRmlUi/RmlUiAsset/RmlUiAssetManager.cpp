#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetManager.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiAssetDocumentManager, 1, WRTTIDefaultAllocator<WRmlUiAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WRmlUiAssetDocumentManager::WRmlUiAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RmlUi";
  m_DocTypeDesc.m_sFileExtension = "WRmlUiAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RmlUi.svg";
  m_DocTypeDesc.m_sAssetCategory = "Input";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WRmlUiAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Rml_UI");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinRmlUi";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;
}

WRmlUiAssetDocumentManager::~WRmlUiAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WRmlUiAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WRmlUiAssetDocument>())
      {
        new WQtRmlUiAssetDocumentWindow(static_cast<WRmlUiAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WRmlUiAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WRmlUiAssetDocument(sPath);
}

void WRmlUiAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
