#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProcGenGraphAssetDocumentManager, 1, WRTTIDefaultAllocator<WProcGenGraphAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WProcGenGraphAssetDocumentManager::WProcGenGraphAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ProcGen Graph";
  m_DocTypeDesc.m_sFileExtension = "WProcGenGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ProcGen_Graph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Construction";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WProcGenGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ProcGen_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinProcGenGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("ProcGen Graph", QPixmap(":/AssetIcons/ProcGen_Graph.svg"));
}

WProcGenGraphAssetDocumentManager::~WProcGenGraphAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WProcGenGraphAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WProcGenGraphAssetDocument>())
      {
        new WProcGenGraphAssetDocumentWindow(static_cast<WProcGenGraphAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WProcGenGraphAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WProcGenGraphAssetDocument(sPath);
}

void WProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
