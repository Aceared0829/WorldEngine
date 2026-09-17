#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WRootMotionSource, 1)
  W_ENUM_CONSTANTS(WRootMotionSource::None, WRootMotionSource::Constant)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipCurveData, 1, WRTTIDefaultAllocator<WAnimationClipCurveData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDynamicStringEnumAttribute("CustomAnimCurveNames")),
    W_MEMBER_PROPERTY("Curve", m_Curve)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WAdditiveAnimationReference, 1)
  W_ENUM_CONSTANTS(WAdditiveAnimationReference::FirstKeyFrame, WAdditiveAnimationReference::LastKeyFrame)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipAssetProperties, 4, WRTTIDefaultAllocator<WAnimationClipAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new WFileBrowserAttribute("Select Animation", WFileBrowserAttribute::MeshesWithAnimations), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", WDependencyFlags::Thumbnail)),
    // \see WAnimationClipAssetDocument::OnRefreshDynamicStringEnum()
    W_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract)->AddAttributes(new WDynamicStringEnumAttribute("AnimationClipsInSourceFile")),
    W_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    W_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    W_MEMBER_PROPERTY("Additive", m_bAdditive),
    W_MEMBER_PROPERTY("BasePreviewAnim", m_sPreviewAnim)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation", WDependencyFlags::Thumbnail)),
    W_ENUM_MEMBER_PROPERTY("AdditiveReference", WAdditiveAnimationReference, m_AdditiveReference),
    W_ENUM_MEMBER_PROPERTY("RootMotion", WRootMotionSource, m_RootMotionMode),
    W_MEMBER_PROPERTY("ConstantRootMotion", m_vConstantRootMotion),
    W_MEMBER_PROPERTY("RootMotionDistance", m_fConstantRootMotionLength),
    W_MEMBER_PROPERTY("AdjustScale", m_fAnimationPositionScale)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0001f, 10000.0f), new WGroupAttribute("Adjustments")),
    W_ARRAY_MEMBER_PROPERTY("Curves", m_Curves),
    W_MEMBER_PROPERTY("EventTrack", m_EventTrack)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipAssetDocument, 7, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAnimationClipAssetProperties::WAnimationClipAssetProperties() = default;
WAnimationClipAssetProperties::~WAnimationClipAssetProperties() = default;

void WAnimationClipAssetDocument::OnRefreshDynamicStringEnum(WDynamicStringEnum::RefreshValuesEvent& e)
{
  if (e.m_sEnumName != "AnimationClipsInSourceFile"_wsv)
    return;

  e.m_pEnum->Clear();

  if (e.m_pDocument == nullptr)
    return;

  const WAssetCurator::WLockedSubAsset asset = WAssetCurator::GetSingleton()->GetSubAsset(e.m_pDocument->GetGuid());

  if (!asset.isValid())
    return;

  const WAssetInfoFile* pInfo = asset->m_pAssetInfo->GetTransformInfo();

  if (pInfo == nullptr)
    return;

  const WVariant clips = pInfo->GetValue(WAssetInfoFile::Keys::AvailableClips);

  if (!clips.IsA<WVariantArray>())
    return;

  for (const WVariant& clip : clips.Get<WVariantArray>())
  {
    e.m_pEnum->AddValidValue(clip.ConvertTo<WString>());
  }
}

void WAnimationClipAssetProperties::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != WGetStaticRTTI<WAnimationClipAssetProperties>())
    return;

  auto& props = *e.m_pPropertyStates;

  const bool bAdditive = e.m_pObject->GetTypeAccessor().GetValue("Additive").ConvertTo<bool>();
  props["AdditiveReference"].m_Visibility = bAdditive ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  props["BasePreviewAnim"].m_Visibility = bAdditive ? WPropertyUiState::Default : WPropertyUiState::Invisible;

  const WInt64 motionType = e.m_pObject->GetTypeAccessor().GetValue("RootMotion").ConvertTo<WInt64>();
  props["ConstantRootMotion"].m_Visibility = WPropertyUiState::Invisible;
  props["RootMotionDistance"].m_Visibility = WPropertyUiState::Invisible;

  switch (motionType)
  {
    case WRootMotionSource::Constant:
      props["ConstantRootMotion"].m_Visibility = WPropertyUiState::Default;
      props["RootMotionDistance"].m_Visibility = WPropertyUiState::Default;
      break;

    default:
      break;
  }
}

WAnimationClipAssetDocument::WAnimationClipAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WAnimationClipAssetProperties>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
}

void WAnimationClipAssetDocument::SetCommonAssetUiState(WCommonAssetUiState::Enum state, double value)
{
  switch (state)
  {
    case WCommonAssetUiState::SimulationSpeed:
      m_fSimulationSpeed = value;
      break;
    default:
      break;
  }

  // handles standard booleans and broadcasts the event
  return SUPER::SetCommonAssetUiState(state, value);
}

double WAnimationClipAssetDocument::GetCommonAssetUiState(WCommonAssetUiState::Enum state) const
{
  switch (state)
  {
    case WCommonAssetUiState::SimulationSpeed:
      return m_fSimulationSpeed;
    default:
      break;
  }

  return SUPER::GetCommonAssetUiState(state);
}

WTransformStatus WAnimationClipAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WProgressRange range("Transforming Asset", 2, false);

  WAnimationClipAssetProperties* pProp = GetProperties();

  WAnimationClipResourceDescriptor desc;

  range.BeginNextStep("Importing Animations");

  WStringBuilder sAbsFilename = pProp->m_sSourceFile;
  if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return WStatus(WFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  WUniquePtr<WModelImporter2::Importer> pImporter = WModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return WStatus("No known importer for this file type.");

  WEditableSkeleton skeleton;

  WModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  // opt.m_pSkeletonOutput = &skeleton; // TODO: may be needed later to optimize the clip
  opt.m_pAnimationOutput = &desc;
  opt.m_bAdditiveAnimation = pProp->m_bAdditive;
  opt.m_AdditiveReference = (pProp->m_AdditiveReference == WAdditiveAnimationReference::FirstKeyFrame) ? WModelImporter2::AdditiveReference::FirstKeyFrame : WModelImporter2::AdditiveReference::LastKeyFrame;
  opt.m_sAnimationToImport = pProp->m_sAnimationClipToExtract;
  opt.m_uiFirstAnimKeyframe = pProp->m_uiFirstFrame;
  opt.m_uiNumAnimKeyframes = pProp->m_uiNumFrames;
  opt.m_fAnimationPositionScale = pProp->m_fAnimationPositionScale;

  const WResult res = pImporter->Import(opt);

  if (res.Succeeded())
  {
    if (pProp->m_RootMotionMode == WRootMotionSource::Constant)
    {
      desc.m_vConstantRootMotion = pProp->m_vConstantRootMotion;

      if (pProp->m_fConstantRootMotionLength > 0.0f)
      {
        desc.m_vConstantRootMotion.SetLength(pProp->m_fConstantRootMotionLength, 0.01f).IgnoreResult();
      }
    }

    // copy named custom curves
    desc.m_CustomCurves.SetCount(pProp->m_Curves.GetCount());
    for (WUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
    {
      desc.m_CustomCurves[i].m_sName.Assign(pProp->m_Curves[i].m_sName);
      pProp->m_Curves[i].m_Curve.ConvertToRuntimeData(desc.m_CustomCurves[i].m_Curve);
      desc.m_CustomCurves[i].m_Curve.SortControlPoints();
      desc.m_CustomCurves[i].m_Curve.CreateLinearApproximation();
    }

    range.BeginNextStep("Writing Result");

    pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

    W_SUCCEED_OR_RETURN(desc.Serialize(stream));
  }

  // Fills the drop down of the 'UseAnimationClip' property, so that a clip can be picked without
  // opening the source file again.
  if (!pImporter->m_OutputAnimationNames.IsEmpty())
  {
    WVariantArray clipNames;
    clipNames.Reserve(pImporter->m_OutputAnimationNames.GetCount());

    for (const auto& sName : pImporter->m_OutputAnimationNames)
    {
      clipNames.PushBack(WVariant(sName));
    }

    GetTransformInfo().SetValue(WAssetInfoFile::Keys::AvailableClips, WVariant(clipNames));
  }

  if (res.Failed())
    return WStatus("Model importer was unable to read this asset.");

  return WStatus(W_SUCCESS);
}

WTransformStatus WAnimationClipAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails would remain black for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }
  if (!GetProperties()->m_sPreviewAnim.IsEmpty())
  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewAnim";
    msg.m_sPayload = GetProperties()->m_sPreviewAnim;
    SendMessageToEngine(&msg);
  }

  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

WUuid WAnimationClipAssetDocument::InsertEventTrackCpAt(WInt64 iTickX, const char* szValue)
{
  WObjectCommandAccessor accessor(GetCommandHistory());
  WObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const WAbstractProperty* pTrackProp = WGetStaticRTTI<WAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  WUuid trackGuid = accessor.Get<WUuid>(GetPropertyObject(), pTrackProp);

  WUuid newObjectGuid;
  W_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, WGetStaticRTTI<WEventTrackControlPointData>(), newObjectGuid).Succeeded(), "");
  const WDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  W_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  W_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAnimationClipAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WAnimationClipAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WAnimationClipAssetDocumentGenerator::WAnimationClipAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

WAnimationClipAssetDocumentGenerator::~WAnimationClipAssetDocumentGenerator() = default;

void WAnimationClipAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_Single";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }

  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_All";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }
}

bool WAnimationClipAssetDocumentGenerator::NeedsImport(WStringView sInputFileAbs, WStringView sMode) const
{
  // In this mode the clip documents are named after the animations inside the file, which are only
  // known after parsing it. Always import, the loop over the clips skips the ones that exist.
  if (sMode == "AnimationClipImport_All")
    return true;

  return SUPER::NeedsImport(sInputFileAbs, sMode);
}

WStatus WAnimationClipAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WStringBuilder title;
  title.SetFormat("Select Preview Mesh for Animation Clip '{}'", sInputFileAbs.GetFileName());

  WStringBuilder sPreviewMesh;

  // The preview mesh is only used for previewing the clip in the editor, so leaving it empty is fine.
  // Without a user there is nobody to close this dialog, which would block the editor indefinitely.
  if (!pApp->IsInUnattendedMode())
  {
    WQtAssetBrowserDlg dlg(nullptr, WUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skinned", title);
    if (dlg.exec() != 0)
    {
      if (dlg.GetSelectedAssetGuid().IsValid())
      {
        WConversionUtils::ToString(dlg.GetSelectedAssetGuid(), sPreviewMesh);
      }
    }
  }

  if (sMode == "AnimationClipImport_Single")
  {
    WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
    if (pDoc == nullptr)
      return WStatus("Could not create target document");

    out_generatedDocuments.PushBack(pDoc);

    WAnimationClipAssetDocument* pAssetDoc = WDynamicCast<WAnimationClipAssetDocument*>(pDoc);

    auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
    accessor.SetValue("File", sInputFileRel.GetView());
    accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

    WLog::Success("Imported animation clip: '{}'", sOutFile);

    return WStatus(W_SUCCESS);
  }

  if (sMode == "AnimationClipImport_All")
  {
    WModelImporter2::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;

    WUniquePtr<WModelImporter2::Importer> pImporter = WModelImporter2::RequestImporterForFileType(opt.m_sSourceFile);
    if (pImporter == nullptr)
      return WStatus("No known importer for this file type.");

    if (pImporter->Import(opt).Failed())
      return WStatus("Failed to import asset.");

    WStringBuilder sFilename;
    WStringBuilder sOutFile2;

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      WPathUtils::MakeValidFilename(clip, '-', sFilename);
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);

      if (WOSFile::ExistsFile(sOutFile2))
      {
        WLog::Info("Skipping animation clip import, file has been imported before: '{}'", sOutFile2);
        continue;
      }

      WDocument* pDoc = pApp->CreateDocument(sOutFile2, WDocumentFlags::None);
      if (pDoc == nullptr)
        return WStatus("Could not create target document");

      out_generatedDocuments.PushBack(pDoc);

      WAnimationClipAssetDocument* pAssetDoc = WDynamicCast<WAnimationClipAssetDocument*>(pDoc);

      auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
      accessor.SetValue("File", sInputFileRel.GetView());
      accessor.SetValue("UseAnimationClip", clip);
      accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

      WLog::Success("Imported animation clip: '{}'", sOutFile2);
    }

    return WStatus(W_SUCCESS);
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return WStatus(W_FAILURE);
}
