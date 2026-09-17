#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAssetManager.h>
#include <EditorPluginAngelScript/AngelScriptWindow/AngelScriptWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAngelScriptAssetManager, 1, WRTTIDefaultAllocator<WAngelScriptAssetManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAngelScriptAssetManager::WAngelScriptAssetManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WAngelScriptAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "AngelScript";
  m_DocTypeDesc.m_sFileExtension = "WAngelScriptAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AngelScript-AS.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WAngelScriptAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinAngelScript";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("AngelScript", QPixmap(":/AssetIcons/AngelScript-Big-AS.svg"));
}

WAngelScriptAssetManager::~WAngelScriptAssetManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WAngelScriptAssetManager::OnDocumentManagerEvent, this));
}

void WAngelScriptAssetManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WAngelScriptAssetDocument>())
      {
        new WQtAngelScriptAssetDocumentWindow((WAngelScriptAssetDocument*)e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WAngelScriptAssetManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WAngelScriptAssetDocument(sPath);
}

void WAngelScriptAssetManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
