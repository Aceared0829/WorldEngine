#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleEffectAssetDocumentManager, 1, WRTTIDefaultAllocator<WParticleEffectAssetDocumentManager>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleEffectAssetDocumentManager::WParticleEffectAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Particle Effect";
  m_DocTypeDesc.m_sFileExtension = "WParticleEffectAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Particle_Effect.svg";
  m_DocTypeDesc.m_sAssetCategory = "Effects";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WParticleEffectAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Particle_Effect");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinParticleEffect";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::AutoTransformOnSave | WAssetDocumentFlags::SupportsThumbnail;
}

WParticleEffectAssetDocumentManager::~WParticleEffectAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));
}

void WParticleEffectAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WParticleEffectAssetDocument>())
      {
        new WQtParticleEffectAssetDocumentWindow(static_cast<WParticleEffectAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WParticleEffectAssetDocumentManager::InternalCreateDocument(
  WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WParticleEffectAssetDocument(sPath);
}

void WParticleEffectAssetDocumentManager::InternalGetSupportedDocumentTypes(
  WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
