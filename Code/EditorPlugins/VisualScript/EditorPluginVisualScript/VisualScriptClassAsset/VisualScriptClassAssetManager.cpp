#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAssetManager.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualScriptClassAssetManager, 1, WRTTIDefaultAllocator<WVisualScriptClassAssetManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WVisualScriptClassAssetManager::WVisualScriptClassAssetManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WVisualScriptClassAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "VisualScriptClass";
  m_DocTypeDesc.m_sFileExtension = "WVisualScriptClassAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/VisualScript.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WVisualScriptClassAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinVisualScriptClass";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("VisualScriptClass", QPixmap(":/AssetIcons/VisualScript.svg"));
}

WVisualScriptClassAssetManager::~WVisualScriptClassAssetManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WVisualScriptClassAssetManager::OnDocumentManagerEvent, this));
}

void WVisualScriptClassAssetManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WVisualScriptClassAssetDocument>())
      {
        new WQtVisualScriptWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WVisualScriptClassAssetManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WVisualScriptClassAssetDocument(sPath);
}

void WVisualScriptClassAssetManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
