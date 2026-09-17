#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineTangentManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WSplineTangentManipulatorAdapter::WSplineTangentManipulatorAdapter() = default;
WSplineTangentManipulatorAdapter::~WSplineTangentManipulatorAdapter() = default;

void WSplineTangentManipulatorAdapter::Finalize()
{
  ConfigureGizmos();
}

void WSplineTangentManipulatorAdapter::Update()
{
  BuildSpline();
  UpdateGizmoTransform();
}

void WSplineTangentManipulatorAdapter::TangentGizmoEventHandler(const WGizmoEvent& e)
{
  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];

  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
      m_vLastTangent = m_bIsTangentIn ? WSimdConversion::ToVec3(cp.m_vPosTangentIn) : WSimdConversion::ToVec3(cp.m_vPosTangentOut);
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
      WVec3 newTangent;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const WQuat qRot = m_RotateGizmo.GetRotationResult();
        newTangent = qRot * m_vLastTangent;
      }
      else
      {
        W_ASSERT_DEV(e.m_pGizmo == &m_ScaleGizmo, "Implementation error");

        const float fScale = m_ScaleGizmo.GetScalingResult().x;
        newTangent = m_vLastTangent * fScale;
      }

      const WSplineTangentManipulatorAttribute* pAttr = static_cast<const WSplineTangentManipulatorAttribute*>(m_pManipulatorAttr);

      if (CustomTangentsLinked())
      {
        WStringBuilder sOtherTangentModeProp = pAttr->GetTangentModeProperty();
        WStringBuilder sOtherCustomTangentProp = pAttr->GetCustomTangentProperty();

        if (m_bIsTangentIn)
        {
          sOtherTangentModeProp.Shrink(0, 2);
          sOtherTangentModeProp.Append("Out");

          sOtherCustomTangentProp.Shrink(0, 2);
          sOtherCustomTangentProp.Append("Out");
        }
        else
        {
          sOtherTangentModeProp.Shrink(0, 3);
          sOtherTangentModeProp.Append("In");

          sOtherCustomTangentProp.Shrink(0, 3);
          sOtherCustomTangentProp.Append("In");
        }

        ChangeProperties(pAttr->GetTangentModeProperty(), WSplineTangentMode::Custom, pAttr->GetCustomTangentProperty(), newTangent, sOtherTangentModeProp, WSplineTangentMode::Custom, sOtherCustomTangentProp, -newTangent);
      }
      else
      {
        ChangeProperties(pAttr->GetTangentModeProperty(), WSplineTangentMode::Custom, pAttr->GetCustomTangentProperty(), newTangent);
      }
    }
    break;
  }
}

void WSplineTangentManipulatorAdapter::UpdateGizmoTransform()
{
  if (m_uiNodeIndex == WInvalidIndex || m_Spline.m_ControlPoints.IsEmpty())
    return;

  auto& cp = m_Spline.m_ControlPoints[m_uiNodeIndex];
  WTransform ownerTransform = GetObjectTransform();

  // Remove scale since is does not have any effect on tangents
  ownerTransform.m_vScale.Set(1.0f);

  const WVec3 forwardDir = m_bIsTangentIn ? WSimdConversion::ToVec3(cp.m_vPosTangentIn) : WSimdConversion::ToVec3(cp.m_vPosTangentOut);
  const WVec3 upDir = WSimdConversion::ToVec3(cp.m_vUpDirAndRoll);

  const WTransform gizmoTransform = [&]()
  {
    WVec3 vFwd = forwardDir;
    vFwd.NormalizeIfNotZero().IgnoreResult();

    const WVec3 vRight = upDir.CrossRH(vFwd).GetNormalized();
    const WVec3 vUp2 = vFwd.CrossRH(vRight).GetNormalized();

    WMat3 mLook;
    mLook.SetColumn(0, vFwd);
    mLook.SetColumn(1, vRight);
    mLook.SetColumn(2, vUp2);

    WTransform t = WTransform::Make(WVec3::MakeZero(), WQuat::MakeFromMat3(mLook));

    return WTransform::MakeGlobalTransform(ownerTransform, t);
  }();

  m_RotateGizmo.SetTransformation(gizmoTransform);
  m_ScaleGizmo.SetTransformation(gizmoTransform);
}

void WSplineTangentManipulatorAdapter::BuildSpline()
{
  const WDocumentObject* pSplineObject = nullptr;
  WStringView sNodeName;
  {
    const WDocumentObject* pSplineTangentObject = m_pObject->GetParent();
    sNodeName = pSplineTangentObject->GetTypeAccessor().GetValue("Name").Get<WString>();

    if (pSplineTangentObject->GetParent() == nullptr)
      return;

    pSplineObject = pSplineTangentObject->GetParent();
  }

  const WDocumentObject* pSplineComponent = nullptr;
  {
    WVariantArray componentUuids;
    if (!pSplineObject->GetTypeAccessor().GetValues("Components", componentUuids))
      return;

    for (const auto& v : componentUuids)
    {
      if (v.IsA<WUuid>())
      {
        pSplineComponent = pSplineObject->GetDocumentObjectManager()->GetObject(v.Get<WUuid>());
        if (pSplineComponent != nullptr && pSplineComponent->GetType()->GetTypeName() == "WSplineComponent")
          break;
      }
    }

    if (pSplineComponent == nullptr)
      return;
  }

  WSplineManipulatorAdapter::BuildSpline(pSplineComponent, "Closed", m_Spline, sNodeName, &m_uiNodeIndex).AssertSuccess();
}

void WSplineTangentManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);
  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.EnableAxis(false, true, true);
  m_RotateGizmo.SetVisible(true);
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WSplineTangentManipulatorAdapter::TangentGizmoEventHandler, this));

  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.EnableAxis(true, false, false, false);
  m_ScaleGizmo.SetVisible(true);
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WSplineTangentManipulatorAdapter::TangentGizmoEventHandler, this));

  {
    const WSplineTangentManipulatorAttribute* pAttr = static_cast<const WSplineTangentManipulatorAttribute*>(m_pManipulatorAttr);
    m_bIsTangentIn = pAttr->GetTangentModeProperty().EndsWith("In");
  }
}

bool WSplineTangentManipulatorAdapter::CustomTangentsLinked() const
{
  WVariant linkedVar = m_pObject->GetTypeAccessor().GetValue("LinkCustomTangents");
  return linkedVar.IsA<bool>() && linkedVar.Get<bool>();
}
