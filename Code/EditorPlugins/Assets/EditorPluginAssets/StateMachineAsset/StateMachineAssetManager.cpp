#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetManager.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineAssetManager, 1, WRTTIDefaultAllocator<WStateMachineAssetManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WStateMachineAssetManager::WStateMachineAssetManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WStateMachineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "StateMachine";
  m_DocTypeDesc.m_sFileExtension = "WStateMachineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/StateMachine.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WStateMachineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_StateMachine");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinStateMachine";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("StateMachine", WSvgThumbnailToPixmap(":/AssetIcons/StateMachine.svg"));
}

WStateMachineAssetManager::~WStateMachineAssetManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WStateMachineAssetManager::OnDocumentManagerEvent, this));
}

void WStateMachineAssetManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WStateMachineAssetDocument>())
      {
        new WQtStateMachineAssetDocumentWindow(e.m_pDocument); // Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WStateMachineAssetManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WStateMachineAssetDocument(sPath);
}

void WStateMachineAssetManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
