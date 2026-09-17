#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WString WBoneManipulatorAdapter::s_sLastSelectedBone;

WBoneManipulatorAdapter::WBoneManipulatorAdapter() = default;
WBoneManipulatorAdapter::~WBoneManipulatorAdapter() = default;

void WBoneManipulatorAdapter::Finalize()
{
  RetrieveBones();
  ConfigureGizmos();
  MigrateSelection();
}

void WBoneManipulatorAdapter::MigrateSelection()
{
  for (WUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (m_Bones[i].m_sName == s_sLastSelectedBone)
    {
      m_Gizmos[i].m_RotateGizmo.SetVisible(true);
      m_Gizmos[i].m_ClickGizmo.SetVisible(false);
      return;
    }
  }

  // keep the last selection, even if it can't be migrated, until something else gets selected
}

void WBoneManipulatorAdapter::Update()
{
  RetrieveBones();
  UpdateGizmoTransform();
}

void WBoneManipulatorAdapter::RotateGizmoEventHandler(const WGizmoEvent& e)
{
  WUInt32 uiGizmo = WInvalidIndex;

  for (WUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_RotateGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      break;
    }
  }

  W_ASSERT_DEBUG(uiGizmo != WInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == WInvalidIndex)
    return;

  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
      BeginTemporaryInteraction();
      break;

    case WGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case WGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case WGizmoEvent::Type::Interaction:
    {
      WTransform globalGizmo = static_cast<const WGizmo*>(e.m_pGizmo)->GetTransformation();
      globalGizmo.m_vScale.Set(1);

      WMat4 mGizmo = globalGizmo.GetAsMat4();
      mGizmo = GetObjectTransform().GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_RootTransform.GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_Gizmos[uiGizmo].m_InverseOffset * mGizmo;

      WQuat rotOnly;
      rotOnly.ReconstructFromMat4(mGizmo);

      SetTransform(uiGizmo, WTransform(mGizmo.GetTranslationVector(), rotOnly));
    }
    break;
  }
}

void WBoneManipulatorAdapter::ClickGizmoEventHandler(const WGizmoEvent& e)
{
  WUInt32 uiGizmo = WInvalidIndex;
  s_sLastSelectedBone.Clear();

  for (WUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_ClickGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      s_sLastSelectedBone = m_Bones[gIdx].m_sName;
      break;
    }
  }

  W_ASSERT_DEBUG(uiGizmo != WInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == WInvalidIndex)
    return;

  switch (e.m_Type)
  {
    case WGizmoEvent::Type::Interaction:
    {
      for (WUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
      {
        m_Gizmos[i].m_RotateGizmo.SetVisible(false);
        m_Gizmos[i].m_ClickGizmo.SetVisible(true);
      }

      m_Gizmos[uiGizmo].m_RotateGizmo.SetVisible(true);
      m_Gizmos[uiGizmo].m_ClickGizmo.SetVisible(false);
    }
    break;
    default:
      break;
  }
}

void WBoneManipulatorAdapter::RetrieveBones()
{
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const WBoneManipulatorAttribute* pAttr = static_cast<const WBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  WVariantArray values;

  // Exposed parameters are only stored as diffs in the component. Thus, requesting the exposed parameters only returns those that have been modified. To get all, you need to use the WExposedParameterCommandAccessor which gives you all exposed parameters from the source asset.
  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (const WExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<WExposedParametersAttribute>())
  {
    const WAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    W_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

    WExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    proxy.GetValues(m_pObject, pProperty, values).AssertSuccess();
    proxy.GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
  }
  else
  {
    pObjectAccessor->GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
    pObjectAccessor->GetValues(m_pObject, pProperty, values).AssertSuccess();
  }

  m_RootTransform.SetIdentity();

  m_Bones.Clear();
  m_Bones.SetCount(values.GetCount());

  for (WUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (values[i].GetReflectedType() == WGetStaticRTTI<WExposedBone>())
    {
      const WExposedBone* pBone = reinterpret_cast<const WExposedBone*>(values[i].GetData());

      if (pBone->m_sName == "<root-transform>")
      {
        m_RootTransform = pBone->m_Transform;
      }

      m_Bones[i] = *pBone;
    }
    else
    {
      // W_REPORT_FAILURE("Property is not an WExposedBone");
      m_Bones.Clear();
      return;
    }
  }
}

void WBoneManipulatorAdapter::UpdateGizmoTransform()
{
  const WMat4 ownerTransform = GetObjectTransform().GetAsMat4();

  for (WUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);
    gizmo.m_InverseOffset = gizmo.m_Offset.GetInverse();

    WMat4 mGizmo = ownerTransform * m_RootTransform.GetAsMat4() * gizmo.m_Offset * m_Bones[i].m_Transform.GetAsMat4();

    WQuat rotOnly;
    rotOnly.ReconstructFromMat4(mGizmo);

    WTransform tGizmo;
    tGizmo.m_vPosition = mGizmo.GetTranslationVector();
    tGizmo.m_qRotation = rotOnly;

    tGizmo.m_vScale.Set(0.5f);
    gizmo.m_RotateGizmo.SetTransformation(tGizmo);

    tGizmo.m_vScale.Set(0.02f);
    gizmo.m_ClickGizmo.SetTransformation(tGizmo);
  }
}

void WBoneManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);
  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmos.SetCount(m_Bones.GetCount());

  for (WUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);

    auto& rot = gizmo.m_RotateGizmo;
    rot.SetOwner(pEngineWindow, nullptr);
    rot.SetVisible(false);
    rot.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WBoneManipulatorAdapter::RotateGizmoEventHandler, this));

    auto& click = gizmo.m_ClickGizmo;
    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(WColor::Thistle);
    click.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WBoneManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

void WBoneManipulatorAdapter::SetTransform(WUInt32 uiBone, const WTransform& value)
{
  WExposedBone* pBone = &m_Bones[uiBone];

  pBone->m_Transform.m_qRotation = value.m_qRotation;
  pBone->m_Transform = value;

  WExposedBone bone;
  bone.m_sName = pBone->m_sName;
  bone.m_sParent = pBone->m_sParent;
  bone.m_Transform = value;

  WVariant var;
  var.CopyTypedObject(&bone, WGetStaticRTTI<WExposedBone>());

  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const WBoneManipulatorAttribute* pAttr = static_cast<const WBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  const WExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<WExposedParametersAttribute>();

  const WAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
  W_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

  WExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);

  // for some reason the first command in WExposedParameterCommandAccessor returns failure 'the property X does not exist' and the insert
  // command than fails with 'the property X already exists' ???

  proxy.SetValue(m_pObject, pProperty, var, m_Keys[uiBone]).AssertSuccess();
}

WMat4 WBoneManipulatorAdapter::ComputeFullTransform(WUInt32 uiBone) const
{
  const WMat4 tParent = ComputeParentTransform(uiBone);

  return tParent * m_Bones[uiBone].m_Transform.GetAsMat4();
}

WMat4 WBoneManipulatorAdapter::ComputeParentTransform(WUInt32 uiBone) const
{
  const WString& parent = m_Bones[uiBone].m_sParent;

  if (!parent.IsEmpty())
  {
    for (WUInt32 b = 0; b < m_Bones.GetCount(); ++b)
    {
      if (m_Bones[b].m_sName == parent)
      {
        return ComputeFullTransform(b);
      }
    }
  }

  return WMat4::MakeIdentity();
}
