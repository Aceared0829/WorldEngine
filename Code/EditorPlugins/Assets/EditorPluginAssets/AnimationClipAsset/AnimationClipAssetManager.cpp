#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetManager.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipAssetDocumentManager, 1, WRTTIDefaultAllocator<WAnimationClipAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAnimationClipAssetDocumentManager::WAnimationClipAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Clip";
  m_DocTypeDesc.m_sFileExtension = "WAnimationClipAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WAnimationClipAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinAnimationClip";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail;

  // WQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.svg"));
}

WAnimationClipAssetDocumentManager::~WAnimationClipAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WAnimationClipAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WAnimationClipAssetDocument>())
      {
        new WQtAnimationClipAssetDocumentWindow(static_cast<WAnimationClipAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void WAnimationClipAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WAnimationClipAssetDocument(sPath);
}

void WAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
  WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
