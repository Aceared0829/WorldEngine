#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurve1DAssetDocumentManager, 1, WRTTIDefaultAllocator<WCurve1DAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WCurve1DAssetDocumentManager::WCurve1DAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Curve1D";
  m_DocTypeDesc.m_sFileExtension = "WCurve1DAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Curve1D.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WCurve1DAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Curve");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinCurve1D";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave | WAssetDocumentFlags::SupportsThumbnail;
}

WCurve1DAssetDocumentManager::~WCurve1DAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WCurve1DAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WCurve1DAssetDocument>())
      {
        new WQtCurve1DAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WCurve1DAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WCurve1DAssetDocument(sPath);
}

void WCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
