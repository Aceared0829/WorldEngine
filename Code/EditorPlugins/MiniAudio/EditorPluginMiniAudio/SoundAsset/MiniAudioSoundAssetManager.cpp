#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAssetManager.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMiniAudioSoundAssetDocumentManager, 1, WRTTIDefaultAllocator<WMiniAudioSoundAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMiniAudioSoundAssetDocumentManager::WMiniAudioSoundAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "MiniAudioSound";
  m_DocTypeDesc.m_sFileExtension = "WMiniAudioSoundAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/MiniAudioSound.svg";
  m_DocTypeDesc.m_sAssetCategory = "Sound";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WMiniAudioSoundAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_MiniAudio_Sound");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinMiniAudioSound";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Sound", QPixmap(":/AssetIcons/MiniAudioSound.svg"));
}

WMiniAudioSoundAssetDocumentManager::~WMiniAudioSoundAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WMiniAudioSoundAssetDocument>())
      {
        new WMiniAudioSoundAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void WMiniAudioSoundAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WMiniAudioSoundAssetDocument(sPath);
}

void WMiniAudioSoundAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
