#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetManager.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationGraphAssetManager, 1, WRTTIDefaultAllocator<WAnimationGraphAssetManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAnimationGraphAssetManager::WAnimationGraphAssetManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WAnimationGraphAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Graph";
  m_DocTypeDesc.m_sFileExtension = "WAnimationGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AnimationGraph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WAnimationGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinAnimGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave;

  WQtImageCache::GetSingleton()->RegisterTypeImage("Animation Graph", QPixmap(":/AssetIcons/AnimationGraph.svg"));
}

WAnimationGraphAssetManager::~WAnimationGraphAssetManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WAnimationGraphAssetManager::OnDocumentManagerEvent, this));
}

void WAnimationGraphAssetManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WAnimationGraphAssetDocument>())
      {
        new WQtAnimationGraphAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WAnimationGraphAssetManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WAnimationGraphAssetDocument(sPath);
}

void WAnimationGraphAssetManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
