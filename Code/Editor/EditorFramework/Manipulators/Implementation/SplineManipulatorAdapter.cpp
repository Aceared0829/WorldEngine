#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Utilities/StringAlgorithms.h>

WSplineManipulatorAdapter::WSplineManipulatorAdapter() = default;
WSplineManipulatorAdapter::~WSplineManipulatorAdapter() = default;

// static
WResult WSplineManipulatorAdapter::BuildSpline(const WDocumentObject* pSplineComponent, WStringView sClosedPropertyName, WSpline& out_spline, WStringView sNodeName /*= WStringView()*/, WUInt32* out_pNodeIndex /*= nullptr*/)
{
  out_spline.m_ControlPoints.Clear();
  out_spline.m_bClosed = false;

  if (pSplineComponent == nullptr)
    return W_FAILURE;

  const WDocumentObject* pParent = pSplineComponent->GetParent();
  if (pParent == nullptr)
    return W_FAILURE;

  out_spline.m_bClosed = pSplineComponent->GetTypeAccessor().GetValue(sClosedPropertyName).ConvertTo<bool>();

  const WIReflectedTypeAccessor& parentAccessor = pParent->GetTypeAccessor();
  const WInt32 iChildCount = parentAccessor.GetCount("Children");
  for (WInt32 i = 0; i < iChildCount; ++i)
  {
    WVariant val = parentAccessor.GetValue("Children", i);
    if (!val.IsA<WUuid>())
      continue;

    const WDocumentObject* pChild = pParent->GetDocumentObjectManager()->GetObject(val.Get<WUuid>());
    if (pChild == nullptr)
      continue;

    const WDocumentObject* pNodeComponent = nullptr;
    for (const WDocumentObject* pComp : pChild->GetChildren())
    {
      if (pComp->GetParentProperty() == "Components" && pComp->GetType()->GetTypeName() == "WSplineNodeComponent")
      {
        pNodeComponent = pComp;
        break;
      }
    }

    if (pNodeComponent == nullptr)
      continue;

    WSpline::ControlPoint cp;
    if (FillControlPointFromNodeComponent(pNodeComponent, cp).Succeeded())
    {
      out_spline.m_ControlPoints.PushBack(cp);

      if (out_pNodeIndex != nullptr && !sNodeName.IsEmpty())
      {
        WVariant name = pChild->GetTypeAccessor().GetValue("Name");
        if (name.IsA<WString>() && name.Get<WString>() == sNodeName)
          *out_pNodeIndex = out_spline.m_ControlPoints.GetCount() - 1;
      }
    }
  }

  out_spline.CalculateUpDirAndAutoTangents();

  return W_SUCCESS;
}

// static
WResult WSplineManipulatorAdapter::FillControlPointFromNodeComponent(const WDocumentObject* pNodeComponent, WSpline::ControlPoint& out_cp)
{
  {
    WVariant v = pNodeComponent->GetParent()->GetTypeAccessor().GetValue("LocalPosition");
    if (!v.IsA<WVec3>())
      return W_FAILURE;

    out_cp.m_vPos = WSimdConversion::ToVec3(v.Get<WVec3>());
  }

  {
    WUInt32 uiTangentModeIn = pNodeComponent->GetTypeAccessor().GetValue("TangentModeIn").ConvertTo<WUInt32>();
    WVariant v = pNodeComponent->GetTypeAccessor().GetValue("CustomTangentIn");
    if (!v.IsA<WVec3>())
      return W_FAILURE;

    out_cp.SetTangentIn(WSimdConversion::ToVec3(v.Get<WVec3>()), static_cast<WSplineTangentMode::Enum>(uiTangentModeIn));
  }

  {
    WUInt32 uiTangentModeOut = pNodeComponent->GetTypeAccessor().GetValue("TangentModeOut").ConvertTo<WUInt32>();
    WVariant v = pNodeComponent->GetTypeAccessor().GetValue("CustomTangentOut");
    if (!v.IsA<WVec3>())
      return W_FAILURE;

    out_cp.SetTangentOut(WSimdConversion::ToVec3(v.Get<WVec3>()), static_cast<WSplineTangentMode::Enum>(uiTangentModeOut));
  }

  return W_SUCCESS;
}

void WSplineManipulatorAdapter::Finalize()
{
  const WSplineManipulatorAttribute* pAttr = static_cast<const WSplineManipulatorAttribute*>(m_pManipulatorAttr);

  auto HasSplineProperties = [&](const WDocumentObject* pObj) -> bool
  {
    const WRTTI* pType = pObj->GetTypeAccessor().GetType();
    return pType->FindPropertyByName(pAttr->GetBindTo()) != nullptr &&
           pType->FindPropertyByName(pAttr->GetClosedProperty()) != nullptr;
  };

  // m_pObject may not directly expose the spline properties. Walk up the parent chain.
  // At each level, also check components (objects stored in the "Components" array property),
  // since the relevant object may be a component of a game object rather than the game object itself.
  while (m_pObject != nullptr)
  {
    if (HasSplineProperties(m_pObject))
      break;

    WVariantArray componentUuids;
    if (m_pObject->GetTypeAccessor().GetValues("Components", componentUuids))
    {
      const WDocumentObject* pFoundComponent = nullptr;
      for (const auto& v : componentUuids)
      {
        if (v.IsA<WUuid>())
        {
          const WDocumentObject* pComponent = m_pObject->GetDocumentObjectManager()->GetObject(v.Get<WUuid>());
          if (pComponent != nullptr && HasSplineProperties(pComponent))
          {
            pFoundComponent = pComponent;
            break;
          }
        }
      }

      if (pFoundComponent != nullptr)
      {
        m_pObject = pFoundComponent;
        break;
      }
    }

    m_pObject = m_pObject->GetParent();
  }
}

void WSplineManipulatorAdapter::Update()
{
  BuildSpline();
  ConfigureGizmos();
}

void WSplineManipulatorAdapter::ClickGizmoEventHandler(const WGizmoEvent& e)
{
  if (e.m_Type != WGizmoEvent::Type::Interaction)
    return;

  e.m_pGizmo->GetOwnerView()->ClearLastPickedObject();

  WInt32 index = -1;

  for (WUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    if (&m_Gizmos[i] == e.m_pGizmo)
    {
      index = i;
      break;
    }
  }

  W_ASSERT_DEBUG(index >= 0, "Gizmo event from unknown gizmo.");
  if (index < 0)
    return;

  auto& gizmo = m_Gizmos[index];
  if (m_Spline.m_bClosed)
  {
    ++index;
  }

  WStringBuilder sNewNodeName;
  MakeUniqueName(index, sNewNodeName);

  index = WMath::Min<WInt32>(index, m_Spline.m_ControlPoints.GetCount());

  auto pObjectAcessor = GetObjectAccessor();

  pObjectAcessor->StartTransaction("Add Spline Node");

  // Add a new child game object
  WUuid gameObjectUuid;
  {
    const WDocumentObject* pSplineObject = m_pObject->GetParent();
    const WRTTI* pGameObjectType = WRTTI::FindTypeByName("WGameObject");

    if (pObjectAcessor->AddObjectByName(pSplineObject, "Children", index, pGameObjectType, gameObjectUuid).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }

    const WDocumentObject* pNewObject = pObjectAcessor->GetObject(gameObjectUuid);

    if (pObjectAcessor->SetValueByName(pNewObject, "Name", sNewNodeName.GetView()).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }

    const WTransform invOwnerTransform = GetObjectTransform().GetInverse();
    const WVec3 localPos = invOwnerTransform.TransformPosition(gizmo.GetTransformation().m_vPosition);

    if (pObjectAcessor->SetValueByName(pNewObject, "LocalPosition", localPos).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }
  }

  // Add a new WSplineNodeComponent to the new game object
  WUuid componentUuid;
  {
    const WDocumentObject* pNewObject = pObjectAcessor->GetObject(gameObjectUuid);
    const WRTTI* pSplineNodeType = WRTTI::FindTypeByName("WSplineNodeComponent");

    if (pObjectAcessor->AddObjectByName(pNewObject, "Components", 0, pSplineNodeType, componentUuid).Failed())
    {
      pObjectAcessor->CancelTransaction();
      return;
    }
  }

  pObjectAcessor->FinishTransaction();

  Update();

  // Defer the selection change to avoid destroying this adapter (and its gizmos) while
  // their event dispatch is still on the call stack. SetSelection triggers ClearAdapters
  // via the manipulator manager, which deletes 'this'. The document outlives the adapter.
  const WDocument* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  QTimer::singleShot(0, [pDoc, gameObjectUuid]()
    {
      if (auto pSelMan = pDoc->GetSelectionManager())
      {
        if (const WDocumentObject* pObj = pDoc->GetObjectManager()->GetObject(gameObjectUuid))
        {
          pSelMan->SetSelection(pObj);
        }
      } });
}

/// Returns the position on the given spline segment at 50% arc-length.
/// Uses uniform sampling with a local parameter t in [0, 1] to avoid the
/// Bezier overshoot that occurs with EvaluatePosition(segmentIndex + 0.5f)
/// when adjacent segments have very different lengths.
static WVec3 EvaluateSegmentArcLengthMidpoint(const WSpline& spline, WUInt32 uiSegment)
{
  constexpr WUInt32 uiNumSamples = 16;

  float fCumulative[uiNumSamples + 1];
  fCumulative[0] = 0.0f;

  WSimdVec4f vPrev = spline.EvaluatePosition(uiSegment, 0.0f);
  for (WUInt32 k = 1; k <= uiNumSamples; ++k)
  {
    const float fLocalT = static_cast<float>(k) / uiNumSamples;
    const WSimdVec4f vCur = spline.EvaluatePosition(uiSegment, fLocalT);
    fCumulative[k] = fCumulative[k - 1] + (vCur - vPrev).GetLength<3>();
    vPrev = vCur;
  }

  const float fHalfLength = fCumulative[uiNumSamples] * 0.5f;

  if (fHalfLength < WMath::SmallEpsilon<float>())
    return WSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, 0.5f));

  for (WUInt32 k = 1; k <= uiNumSamples; ++k)
  {
    if (fCumulative[k] >= fHalfLength)
    {
      const float fFrac = WMath::Unlerp(fCumulative[k - 1], fCumulative[k], fHalfLength);
      const float fLocalT = (static_cast<float>(k - 1) + fFrac) / uiNumSamples;
      return WSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, fLocalT));
    }
  }

  return WSimdConversion::ToVec3(spline.EvaluatePosition(uiSegment, 0.5f));
}

void WSplineManipulatorAdapter::UpdateGizmoTransform()
{
  const WTransform ownerTransform = GetObjectTransform();
  auto MakeGizmoTransform = [&](const WVec3& offset)
  {
    WTransform t = WTransform::Make(ownerTransform.TransformPosition(offset));
    t.m_vScale.Set(0.1f);
    return t;
  };

  const WUInt32 uiNumCPs = m_Spline.m_ControlPoints.GetCount();

  WUInt32 uiFirstGizmo = 0;
  WUInt32 uiNumGizmos = m_Gizmos.GetCount();

  if (!m_Spline.m_bClosed)
  {
    if (uiNumCPs == 0)
    {
      m_Gizmos.PeekFront().SetTransformation(MakeGizmoTransform(WVec3(-0.5, 0, 0)));
      m_Gizmos.PeekBack().SetTransformation(MakeGizmoTransform(WVec3(0.5, 0, 0)));
    }
    else
    {
      auto& cp0 = m_Spline.m_ControlPoints[0];
      auto& cp1 = m_Spline.m_ControlPoints.PeekBack();

      WVec3 dir0 = WSimdConversion::ToVec3(cp0.m_vPosTangentIn);
      dir0.NormalizeIfNotZero(WVec3(-1, 0, 0)).IgnoreResult();
      m_Gizmos.PeekFront().SetTransformation(MakeGizmoTransform(WSimdConversion::ToVec3(cp0.m_vPos) + dir0));

      WVec3 dir1 = WSimdConversion::ToVec3(cp1.m_vPosTangentOut);
      dir1.NormalizeIfNotZero(WVec3(1, 0, 0)).IgnoreResult();
      m_Gizmos.PeekBack().SetTransformation(MakeGizmoTransform(WSimdConversion::ToVec3(cp1.m_vPos) + dir1));
    }

    uiFirstGizmo = 1;
    uiNumGizmos = uiNumGizmos - 2;
  }

  for (WUInt32 i = 0; i < uiNumGizmos; ++i)
  {
    auto& gizmo = m_Gizmos[uiFirstGizmo + i];

    const WVec3 offset = EvaluateSegmentArcLengthMidpoint(m_Spline, i);
    gizmo.SetTransformation(MakeGizmoTransform(offset));
  }
}

void WSplineManipulatorAdapter::BuildSpline()
{
  m_Spline.m_ControlPoints.Clear();
  m_Spline.m_bClosed = false;

  const WSplineManipulatorAttribute* pAttr = static_cast<const WSplineManipulatorAttribute*>(m_pManipulatorAttr);
  if (pAttr->GetBindTo().IsEmpty() || pAttr->GetClosedProperty().IsEmpty())
    return;

  BuildSpline(m_pObject, pAttr->GetClosedProperty(), m_Spline).AssertSuccess();
}

void WSplineManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);
  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  const WUInt32 numGizmos = WMath::Max(m_Spline.m_ControlPoints.GetCount() + (m_Spline.m_bClosed ? 0 : 1), 2u);
  m_Gizmos.SetCount(numGizmos);

  for (WUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& click = m_Gizmos[i];
    if (click.IsVisible() && !click.m_GizmoEvents.IsEmpty())
      continue; // already configured

    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(WColorScheme::LightUI(WColorScheme::Red));
    click.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WSplineManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

void WSplineManipulatorAdapter::MakeUniqueName(WInt32 iIndex, WStringBuilder& ref_sName)
{
  // Collect names of existing spline node children in array order.
  WDynamicArray<WString> nodeNames;
  const WDocumentObject* pParent = m_pObject->GetParent();
  if (pParent != nullptr)
  {
    const WIReflectedTypeAccessor& parentAccessor = pParent->GetTypeAccessor();
    const WInt32 iChildCount = parentAccessor.GetCount("Children");
    for (WInt32 i = 0; i < iChildCount; ++i)
    {
      WVariant val = parentAccessor.GetValue("Children", i);
      if (!val.IsA<WUuid>())
        continue;

      const WDocumentObject* pChild = pParent->GetDocumentObjectManager()->GetObject(val.Get<WUuid>());
      if (pChild == nullptr)
        continue;

      bool bHasSplineNode = false;
      for (const WDocumentObject* pComp : pChild->GetChildren())
      {
        if (pComp->GetParentProperty() == "Components" && pComp->GetType()->GetTypeName() == "WSplineNodeComponent")
        {
          bHasSplineNode = true;
          break;
        }
      }

      if (!bHasSplineNode)
        continue;

      WVariant name = pChild->GetTypeAccessor().GetValue("Name");
      nodeNames.PushBack(name.IsA<WString>() ? name.Get<WString>() : WString());
    }
  }

  if (nodeNames.IsEmpty())
  {
    ref_sName = "1";
    return;
  }

  auto IsUniqueName = [&](WStringView sName) -> bool
  {
    for (const auto& s : nodeNames)
    {
      if (s == sName)
        return false;
    }
    return true;
  };

  const WStringView sLeft = (iIndex > 0 && iIndex <= (WInt32)nodeNames.GetCount()) ? nodeNames[iIndex - 1].GetView() : WStringView();
  const WStringView sRight = (iIndex < (WInt32)nodeNames.GetCount()) ? nodeNames[iIndex].GetView() : WStringView();

  WStringAlgorithms::ComputeNameBetween(sLeft, sRight, ref_sName);

  if (IsUniqueName(ref_sName))
    return;

  // Fallback: append an incrementing sub-index until the name is unique
  WStringBuilder sBase = ref_sName;
  WUInt32 uiSuffix = 1;
  do
  {
    ref_sName = sBase;
    ref_sName.AppendFormat(".{}", uiSuffix);
    ++uiSuffix;
  } while (!IsUniqueName(ref_sName));
}
