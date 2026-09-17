#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonAssetDocument, 12, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static WTransform CalculateTransformationMatrix(const WEditableSkeleton* pProp)
{
  const float us = WMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = WMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = WMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = WMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  WBasisAxis::Enum forwardDir = WBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  WTransform t;
  t.SetIdentity();
  t.m_vScale.Set(us);

  // prevent mirroring in the rotation matrix, because we can't generate a quaternion from that
  if (!flipFwd)
  {
    switch (forwardDir)
    {
      case WBasisAxis::PositiveX:
        forwardDir = WBasisAxis::NegativeX;
        t.m_vScale.x *= -1;
        break;
      case WBasisAxis::PositiveY:
        forwardDir = WBasisAxis::NegativeY;
        t.m_vScale.y *= -1;
        break;
      case WBasisAxis::PositiveZ:
        forwardDir = WBasisAxis::NegativeZ;
        t.m_vScale.z *= -1;
        break;
      case WBasisAxis::NegativeX:
        forwardDir = WBasisAxis::PositiveX;
        t.m_vScale.x *= -1;
        break;
      case WBasisAxis::NegativeY:
        forwardDir = WBasisAxis::PositiveY;
        t.m_vScale.y *= -1;
        break;
      case WBasisAxis::NegativeZ:
        forwardDir = WBasisAxis::PositiveZ;
        t.m_vScale.z *= -1;
        break;
    }
  }

  WMat3 rot = WBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, 1.0f);
  t.m_qRotation = WQuat::MakeFromMat3(rot);

  return t;
}

WSkeletonAssetDocument::WSkeletonAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WEditableSkeleton>(sDocumentPath, WAssetDocEngineConnection::Simple, true)
{
}

WSkeletonAssetDocument::~WSkeletonAssetDocument() = default;

void WSkeletonAssetDocument::PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WEditableSkeleton>())
  {
    auto& props = *e.m_pPropertyStates;

    const WInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<WInt64>();
    const bool bCustomTransform = importTransform == 127;
    props["RightDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["UpDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["FlipForwardDir"].m_Visibility = bCustomTransform ? WPropertyUiState::Default : WPropertyUiState::Invisible;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WEditableSkeletonJoint>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool overrideSurface = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();
    const WSkeletonJointType::Enum jointType = (WSkeletonJointType::Enum)e.m_pObject->GetTypeAccessor().GetValue("JointType").ConvertTo<WInt32>();

    const bool bHasStiffness = jointType == WSkeletonJointType::SwingTwist;
    const bool bHasSwing = jointType == WSkeletonJointType::SwingTwist;
    const bool bHasTwist = jointType == WSkeletonJointType::SwingTwist;

    props["CollisionLayer"].m_Visibility = overrideCollisionLayer ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["Surface"].m_Visibility = overrideSurface ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    props["LocalRotation"].m_Visibility = bHasStiffness ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["Stiffness"].m_Visibility = bHasStiffness ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SwingLimitY"].m_Visibility = bHasSwing ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["SwingLimitZ"].m_Visibility = bHasSwing ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["TwistLimitHalfAngle"].m_Visibility = bHasTwist ? WPropertyUiState::Default : WPropertyUiState::Invisible;
    props["TwistLimitCenterAngle"].m_Visibility = bHasTwist ? WPropertyUiState::Default : WPropertyUiState::Invisible;

    return;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == WGetStaticRTTI<WEditableSkeletonBoneShape>())
  {
    auto& props = *e.m_pPropertyStates;

    const WSkeletonJointGeometryType::Enum geomType = (WSkeletonJointGeometryType::Enum)e.m_pObject->GetTypeAccessor().GetValue("Geometry").ConvertTo<WInt32>();

    props["Offset"].m_Visibility = WPropertyUiState::Invisible;
    props["Rotation"].m_Visibility = WPropertyUiState::Invisible;
    props["Length"].m_Visibility = WPropertyUiState::Invisible;
    props["Width"].m_Visibility = WPropertyUiState::Invisible;
    props["Thickness"].m_Visibility = WPropertyUiState::Invisible;

    if (geomType == WSkeletonJointGeometryType::None)
      return;

    props["Length"].m_sNewLabelText = "Length";
    props["Width"].m_sNewLabelText = "Width";
    props["Thickness"].m_sNewLabelText = "Thickness";

    props["Offset"].m_Visibility = WPropertyUiState::Default;
    props["Rotation"].m_Visibility = WPropertyUiState::Default;

    if (geomType == WSkeletonJointGeometryType::Box)
    {
      props["Length"].m_Visibility = WPropertyUiState::Default;
      props["Width"].m_Visibility = WPropertyUiState::Default;
      props["Thickness"].m_Visibility = WPropertyUiState::Default;
    }
    else if (geomType == WSkeletonJointGeometryType::Sphere)
    {
      props["Thickness"].m_Visibility = WPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }
    else if (geomType == WSkeletonJointGeometryType::Capsule || geomType == WSkeletonJointGeometryType::CapsuleSideways)
    {
      props["Length"].m_Visibility = WPropertyUiState::Default;

      props["Thickness"].m_Visibility = WPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }

    return;
  }
}

WStatus WSkeletonAssetDocument::WriteResource(WStreamWriter& inout_stream, const WEditableSkeleton& skeleton, WUInt16* out_pNumBones) const
{
  WSkeletonResourceDescriptor desc;
  desc.m_RootTransform = CalculateTransformationMatrix(&skeleton);
  skeleton.FillResourceDescriptor(desc);

  if (out_pNumBones != nullptr)
  {
    *out_pNumBones = desc.m_Skeleton.GetJointCount();
  }

  W_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return WStatus(W_SUCCESS);
}

void WSkeletonAssetDocument::SetRenderBones(bool bEnable)
{
  if (m_bRenderBones == bEnable)
    return;

  m_bRenderBones = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::SetRenderColliders(bool bEnable)
{
  if (m_bRenderColliders == bEnable)
    return;

  m_bRenderColliders = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::SetRenderJoints(bool bEnable)
{
  if (m_bRenderJoints == bEnable)
    return;

  m_bRenderJoints = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::SetRenderSwingLimits(bool bEnable)
{
  if (m_bRenderSwingLimits == bEnable)
    return;

  m_bRenderSwingLimits = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::SetRenderTwistLimits(bool bEnable)
{
  if (m_bRenderTwistLimits == bEnable)
    return;

  m_bRenderTwistLimits = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::SetRenderPreviewMesh(bool bEnable)
{
  if (m_bRenderPreviewMesh == bEnable)
    return;

  m_bRenderPreviewMesh = bEnable;

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void WSkeletonAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // expose all the bones as parameters
  // such that we can create components that modify these bones

  auto* desc = GetProperties();
  WExposedParameters* pExposedParams = W_DEFAULT_NEW(WExposedParameters);


  {
    WExposedBone bone;
    bone.m_sName = "<root-transform>";
    bone.m_Transform = CalculateTransformationMatrix(desc);

    WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
    param->m_sName = "<root-transform>";
    param->m_DefaultValue.CopyTypedObject(&bone, WGetStaticRTTI<WExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);
  }

  auto Traverse = [&](WEditableSkeletonJoint* pJoint, const char* szParent, auto recurse) -> void
  {
    WExposedBone bone;
    bone.m_sName = pJoint->GetName();
    bone.m_sParent = szParent;
    bone.m_Transform = pJoint->m_LocalTransform;

    WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
    param->m_sName = pJoint->GetName();
    param->m_DefaultValue.CopyTypedObject(&bone, WGetStaticRTTI<WExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);

    for (auto pChild : pJoint->m_Children)
    {
      recurse(pChild, pJoint->GetName(), recurse);
    }
  };

  for (auto ptr : desc->m_Children)
  {
    Traverse(ptr, "", Traverse);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

WTransformStatus WSkeletonAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    W_SCOPE_EXIT(m_bIsTransforming = false);

    WProgressRange range("Transforming Asset", 3, false);

    WEditableSkeleton* pProp = GetProperties();

    WUInt16 uiNumBones = 0;

    WStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (sAbsFilename.IsEmpty())
    {
      range.BeginNextStep("Writing Result");
      W_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties(), &uiNumBones));
    }
    else
    {
      if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
      {
        return WStatus(WFmt("Couldn't make path absolute: '{0};", sAbsFilename));
      }

      WUniquePtr<WModelImporter2::Importer> pImporter = WModelImporter2::RequestImporterForFileType(sAbsFilename);
      if (pImporter == nullptr)
        return WStatus("No known importer for this file type.");

      range.BeginNextStep("Importing Source File");

      WEditableSkeleton newSkeleton;

      WModelImporter2::ImportOptions opt;
      opt.m_sSourceFile = sAbsFilename;
      opt.m_pSkeletonOutput = &newSkeleton;

      if (pImporter->Import(opt).Failed())
        return WStatus("Model importer was unable to read this asset.");

      if (newSkeleton.m_Children.IsEmpty())
        return WStatus("Imported skeleton is empty.");

      range.BeginNextStep("Importing Skeleton Data");

      // merging rewrites the joint hierarchy in this document, which can't be saved back to disk
      // during background processing, so the asset has to be transformed in the editor instead
      if (transformFlags.IsSet(WTransformFlags::BackgroundProcessing) && WouldSkeletonHierarchyChange(newSkeleton))
      {
        return WTransformStatus(WTransformResult::NeedsImport);
      }

      // synchronize the old data (collision geometry etc.) with the new hierarchy
      const WEditableSkeleton* pFinalSkeleton = MergeWithNewSkeleton(newSkeleton);

      range.BeginNextStep("Writing Result");
      W_SUCCEED_OR_RETURN(WriteResource(stream, *pFinalSkeleton, &uiNumBones));

      // merge the new data with the actual asset document
      ApplyNativePropertyChangesToObjectManager(true);
    }

    GetTransformInfo().SetValue(WAssetInfoFile::Keys::NumBones, uiNumBones);
  }

  WSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = WSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return WStatus(W_SUCCESS);
}

WTransformStatus WSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails wouldn't look as desired, for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    WSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }

  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

bool WSkeletonAssetDocument::WouldSkeletonHierarchyChange(const WEditableSkeleton& newSkeleton) const
{
  auto CompareJoints = [](const auto& self, const WEditableSkeletonJoint* pOld, const WEditableSkeletonJoint* pNew) -> bool
  {
    if (!WStringUtils::IsEqual(pOld->GetName(), pNew->GetName()))
      return true;

    if (pOld->m_Children.GetCount() != pNew->m_Children.GetCount())
      return true;

    for (WUInt32 i = 0; i < pOld->m_Children.GetCount(); ++i)
    {
      if (self(self, pOld->m_Children[i], pNew->m_Children[i]))
        return true;
    }

    return false;
  };

  const WEditableSkeleton* pOldSkeleton = GetProperties();

  if (pOldSkeleton->m_Children.GetCount() != newSkeleton.m_Children.GetCount())
    return true;

  for (WUInt32 i = 0; i < pOldSkeleton->m_Children.GetCount(); ++i)
  {
    if (CompareJoints(CompareJoints, pOldSkeleton->m_Children[i], newSkeleton.m_Children[i]))
      return true;
  }

  return false;
}

const WEditableSkeleton* WSkeletonAssetDocument::MergeWithNewSkeleton(WEditableSkeleton& newSkeleton)
{
  WEditableSkeleton* pOldSkeleton = GetProperties();
  WMap<WString, const WEditableSkeletonJoint*> prevJoints;

  // map all old joints by name
  {
    auto TraverseJoints = [&prevJoints](const auto& self, WEditableSkeletonJoint* pJoint) -> void
    {
      prevJoints[pJoint->GetName()] = pJoint;

      for (WEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild);
      }
    };

    for (WEditableSkeletonJoint* pChild : pOldSkeleton->m_Children)
    {
      TraverseJoints(TraverseJoints, pChild);
    }
  }

  // copy old properties to new skeleton
  {
    auto TraverseJoints = [&prevJoints](const auto& self, WEditableSkeletonJoint* pJoint, const WTransform& root, WTransform origin) -> void
    {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      // use the parent rotation as the gizmo base rotation
      WMat4 modelTransform, fullTransform;
      modelTransform = origin.GetAsMat4();
      WMsgAnimationPoseUpdated::ComputeFullBoneTransform(root.GetAsMat4(), modelTransform, fullTransform, pJoint->m_qGizmoOffsetRotationRO);

      origin = WTransform::MakeGlobalTransform(origin, pJoint->m_LocalTransform);
      pJoint->m_vGizmoOffsetPositionRO = root.TransformPosition(origin.m_vPosition);

      for (WEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild, root, origin);
      }
    };

    for (WEditableSkeletonJoint* pChild : newSkeleton.m_Children)
    {
      TraverseJoints(TraverseJoints, pChild, CalculateTransformationMatrix(pOldSkeleton), WTransform::MakeIdentity());
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = newSkeleton.m_Children;
  newSkeleton.m_Children.Clear(); // prevent this skeleton from deallocating the joints

  return pOldSkeleton;
}


//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSkeletonAssetDocumentGenerator, 1, WRTTIDefaultAllocator<WSkeletonAssetDocumentGenerator>)
W_END_DYNAMIC_REFLECTED_TYPE;

WSkeletonAssetDocumentGenerator::WSkeletonAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

WSkeletonAssetDocumentGenerator::~WSkeletonAssetDocumentGenerator() = default;

void WSkeletonAssetDocumentGenerator::GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    WAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = WAssetDocGeneratorPriority::Undecided;
    info.m_sName = "SkeletonImport";
    info.m_sIcon = ":/AssetIcons/Skeleton.svg";
  }
}

WStatus WSkeletonAssetDocumentGenerator::Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments)
{
  const WStringBuilder sOutFile = GetImportTargetPath(sInputFileAbs);

  auto pApp = WQtEditorApp::GetSingleton();

  WStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  WDocument* pDoc = pApp->CreateDocument(sOutFile, WDocumentFlags::None);
  if (pDoc == nullptr)
    return WStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  WSkeletonAssetDocument* pAssetDoc = WDynamicCast<WSkeletonAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", sInputFileRel.GetView());

  WLog::Success("Imported skeleton: '{}'", sOutFile);

  return WStatus(W_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WEditableSkeleton_1_2 : public WGraphPatch
{
public:
  WEditableSkeleton_1_2()
    : WGraphPatch("WEditableSkeleton", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

WEditableSkeleton_1_2 g_WEditableSkeleton_1_2;
