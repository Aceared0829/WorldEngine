#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMaterialAssetDocumentManager, 1, WRTTIDefaultAllocator<WMaterialAssetDocumentManager>)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

const char* const WMaterialAssetDocumentManager::s_szShaderOutputTag = "VISUAL_SHADER";

WMaterialAssetDocumentManager::WMaterialAssetDocumentManager()
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WMaterialAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  WAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Material", "WMaterial");

  m_DocTypeDesc.m_sDocumentTypeName = "Material";
  m_DocTypeDesc.m_sFileExtension = "WMaterialAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Material.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = WGetStaticRTTI<WMaterialAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Material");

  m_DocTypeDesc.m_sResourceFileExtension = "WBinMaterial";
  m_DocTypeDesc.m_AssetDocumentFlags = WAssetDocumentFlags::SupportsThumbnail | WAssetDocumentFlags::AutoTransformOnSave;
}

WMaterialAssetDocumentManager::~WMaterialAssetDocumentManager()
{
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

WString WMaterialAssetDocumentManager::GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDescriptor, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  if (sOutputTag.IsEqual(s_szShaderOutputTag))
  {
    WStringBuilder sRelativePath(sDocumentPath);
    sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
    WAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "autogen.WShader", false);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, sDataDirectory, sDocumentPath, sOutputTag, pAssetProfile);
}


bool WMaterialAssetDocumentManager::IsOutputUpToDate(WStringView sDocumentPath, WStringView sOutputTag, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  if (sOutputTag.IsEqual(s_szShaderOutputTag))
  {
    const WString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, sDocumentPath, sOutputTag);

    WStringBuilder sExpectedHeader;
    sExpectedHeader.SetFormat("//{0}|{1}\n", uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());

    WFileReader file;
    if (file.Open(sTargetFile, 256).Failed())
      return false;

    // this might happen if writing to the file failed
    if (file.GetFileSize() < sExpectedHeader.GetElementCount())
      return false;

    WUInt8 Temp[256] = {0};
    const WUInt32 uiRead = (WUInt32)file.ReadBytes(Temp, sExpectedHeader.GetElementCount());
    WStringBuilder sFileHeader = WStringView((const char*)&Temp[0], (const char*)&Temp[uiRead]);

    return sFileHeader.IsEqual(sExpectedHeader);
  }

  return WAssetDocumentManager::IsOutputUpToDate(sDocumentPath, sOutputTag, uiHash, pTypeDescriptor);
}

WStringView WMaterialAssetDocumentManager::GetOutputDocumentType(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sOutputTag, const WPlatformProfile* pAssetProfile) const
{
  if (sOutputTag == s_szShaderOutputTag)
  {
    return "Shader";
  }
  return SUPER::GetOutputDocumentType(pTypeDesc, sOutputTag, pAssetProfile);
}

void WMaterialAssetDocumentManager::OnDocumentManagerEvent(const WDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case WDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == WGetStaticRTTI<WMaterialAssetDocument>())
      {
        new WQtMaterialAssetDocumentWindow(static_cast<WMaterialAssetDocument*>(e.m_pDocument)); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void WMaterialAssetDocumentManager::InternalCreateDocument(WStringView sDocumentTypeName, WStringView sPath, bool bCreateNewDocument, WDocument*& out_pDocument, const WDocumentObject* pOpenContext)
{
  out_pDocument = new WMaterialAssetDocument(sPath);
}

void WMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(WDynamicArray<const WDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
