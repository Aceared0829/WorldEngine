#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WDecalMode, 1)
  W_ENUM_CONSTANT(WDecalMode::BaseColor),
  W_ENUM_CONSTANT(WDecalMode::BaseColorNormal),
  W_ENUM_CONSTANT(WDecalMode::BaseColorORM),
  W_ENUM_CONSTANT(WDecalMode::BaseColorNormalORM),
  W_ENUM_CONSTANT(WDecalMode::BaseColorEmissive)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalAssetProperties, 4, WRTTIDefaultAllocator<WDecalAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Mode", WDecalMode, m_Mode),
    W_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    W_MEMBER_PROPERTY("AlphaMask", m_sAlphaMask)->AddAttributes(new WFileBrowserAttribute("Select Alpha Mask", WFileBrowserAttribute::ImagesLdrOnly)),
    W_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new WFileBrowserAttribute("Select Base Color Map", WFileBrowserAttribute::ImagesLdrOnly)),
    W_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new WFileBrowserAttribute("Select Normal Map", WFileBrowserAttribute::ImagesLdrOnly), new WDefaultValueAttribute(WStringView("Textures/NeutralNormal.tga"))), // wrap in WStringView to prevent a memory leak report
    W_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new WFileBrowserAttribute("Select ORM Map", WFileBrowserAttribute::ImagesLdrOnly)),
    W_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new WFileBrowserAttribute("Select Emissive Map", WFileBrowserAttribute::ImagesLdrOnly)),
    W_MEMBER_PROPERTY("NumVariationsX", m_uiNumVariationsX)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
    W_MEMBER_PROPERTY("NumVariationsY", m_uiNumVariationsY)->AddAttributes(new WDefaultValueAttribute(1), new WClampValueAttribute(1, 16)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDecalAssetProperties::WDecalAssetProperties() = default;

void WDecalAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WDecalAssetProperties>())
  {
    WInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<WInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Normal"].m_Visibility = WPropertyUiState::Invisible;
    props["ORM"].m_Visibility = WPropertyUiState::Invisible;
    props["Emissive"].m_Visibility = WPropertyUiState::Invisible;

    if (mode == WDecalMode::BaseColorNormal)
    {
      props["Normal"].m_Visibility = WPropertyUiState::Default;
    }
    else if (mode == WDecalMode::BaseColorORM)
    {
      props["ORM"].m_Visibility = WPropertyUiState::Default;
    }
    else if (mode == WDecalMode::BaseColorNormalORM)
    {
      props["Normal"].m_Visibility = WPropertyUiState::Default;
      props["ORM"].m_Visibility = WPropertyUiState::Default;
    }
    else if (mode == WDecalMode::BaseColorEmissive)
    {
      props["Emissive"].m_Visibility = WPropertyUiState::Default;
    }
  }
}

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalAssetDocument, 6, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDecalAssetDocument::WDecalAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WDecalAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
}

WTransformStatus WDecalAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  return static_cast<WDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

WTransformStatus WDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
{
  const WDecalAssetProperties* pProp = GetProperties();

  QStringList arguments;

  const WStringBuilder sThumbnail = GetThumbnailFilePath();

  arguments << "-usage";
  arguments << "Color";

  {
    // Thumbnail
    const WStringBuilder sDir = sThumbnail.GetFileDirectory();
    W_SUCCEED_OR_RETURN(WOSFile::CreateDirectoryStructure(sDir));

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
  }

  if (pProp->m_uiNumVariationsX > 1 || pProp->m_uiNumVariationsY > 1)
  {
    // only show the first variation, otherwise the thumbnail would show the entire grid
    arguments << "-gridX";
    arguments << QString::number(WMath::Max<WUInt8>(1, pProp->m_uiNumVariationsX));

    arguments << "-gridY";
    arguments << QString::number(WMath::Max<WUInt8>(1, pProp->m_uiNumVariationsY));
  }

  {
    WQtEditorApp* pEditorApp = WQtEditorApp::GetSingleton();

    WStringBuilder sAbsBaseColor = pProp->m_sBaseColor;
    if (!sAbsBaseColor.IsEmpty() && !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsBaseColor))
    {
      return WStatus(WFmt("Failed to make path absolute: '{}'", sAbsBaseColor));
    }

    WStringBuilder sAbsAlphaMask = pProp->m_sAlphaMask;
    if (!sAbsAlphaMask.IsEmpty() && !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsAlphaMask))
    {
      return WStatus(WFmt("Failed to make path absolute: '{}'", sAbsAlphaMask));
    }

    if (sAbsBaseColor.IsEmpty() && sAbsAlphaMask.IsEmpty())
    {
      return WStatus("Decal has neither a base color nor an alpha mask texture");
    }

    if (sAbsBaseColor.IsEmpty())
    {
      // only an alpha mask is given, the decal is opaque white with the mask's shape
      arguments << "-in0";
      arguments << QString(sAbsAlphaMask.GetData());

      arguments << "-rgb";
      arguments << "white";

      arguments << "-a";
      arguments << "in0.r";
    }
    else
    {
      arguments << "-in0";
      arguments << QString(sAbsBaseColor.GetData());

      if (!sAbsAlphaMask.IsEmpty())
      {
        arguments << "-in1";
        arguments << QString(sAbsAlphaMask.GetData());

        arguments << "-rgb";
        arguments << "in0.rgb";

        arguments << "-a";
        arguments << "in1.r";
      }
      else
      {
        arguments << "-rgba";
        arguments << "in0.rgba";
      }
    }
  }

  W_SUCCEED_OR_RETURN(WQtEditorApp::GetSingleton()->ExecuteTool("WTexConv", arguments, 180, WLog::GetThreadLocalLogSystem()));

  {
    WUInt64 uiThumbnailHash = WAssetCurator::GetSingleton()->GetAssetThumbnailHash(GetGuid());
    W_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return WStatus(W_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDecalAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WDecalAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WDecalAssetDocumentGenerator::WDecalAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
}

WDecalAssetDocumentGenerator::~WDecalAssetDocumentGenerator() = default;

void WDecalAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const WStringBuilder baseFilename = sAbsInputFile.GetFileName();

  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = isDecal ? WAssetDocGeneratorPriority::HighPriority : WAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "DecalImport.All";
    info.m_sIcon = ":/AssetIcons/Decal.svg";
  }
}

WStatus WDecalAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WDecalAssetDocument* pAssetDoc = WDynamicCast<WDecalAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", sInputFileRel.GetView());

  WLog::Success("Imported decal: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}
