#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetManager.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSoundEventAssetDocumentManager, 1, WRTTIDefaultAllocator<WSoundEventAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSoundEventAssetDocumentManager::WSoundEventAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WSoundEventAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_bCanCreate = false;
  m_DocTypeDesc.m_sDocumentTypeName = "Sound Event";
  m_DocTypeDesc.m_sFileExtension = "WSoundEventAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Sound_Event.svg";
  m_DocTypeDesc.m_sAssetCategory = "Sound";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WSoundEventAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Fmod_Event");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinFmodSoundEvent";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::None;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Sound Event", QPixmap(":/AssetIcons/Sound_Event.svg"));
}

WSoundEventAssetDocumentManager::~WSoundEventAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WSoundEventAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WSoundEventAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WSoundEventAssetDocument>())
      {
        new WSoundEventAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void WSoundEventAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WSoundEventAssetDocument(sPath);
}

void WSoundEventAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
