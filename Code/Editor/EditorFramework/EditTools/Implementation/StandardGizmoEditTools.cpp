#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTranslateGizmoEditTool, 1, WRTTIDefaultAllocator<WTranslateGizmoEditTool>)
W_END_DYNAMIC_REFLECTED_TYPE;

WTranslateGizmoEditTool::WTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

WTranslateGizmoEditTool::~WTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));

  auto& events = WPreferences::QueryPreferences<WScenePreferencesUser>(GetDocument())->m_ChangedEvent;

  if (events.HasEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::OnPreferenceChange, this)))
    events.RemoveEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::OnPreferenceChange, this));
}

void WTranslateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_TranslateGizmo.UpdateStatusBarText(GetWindow());
  }
}

void WTranslateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_TranslateGizmo.SetOwner(GetWindow(), nullptr);

  WPreferences::QueryPreferences<WScenePreferencesUser>(GetDocument())
    ->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::OnPreferenceChange, this));
}

void WTranslateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_TranslateGizmo.SetVisible(visible);
}

void WTranslateGizmoEditTool::ApplyGizmoTransformation(const WTransform& transform)
{
  m_TranslateGizmo.SetTransformation(transform);
}

void WTranslateGizmoEditTool::TransformationGizmoEventHandlerImpl(const WGizmoEvent& e)
{
  WObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        (QApplication::keyboardModifiers() == Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (bDuplicate && (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<WOrthoGizmoContext>()))
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }

      // UE style move along with the moving gizmo
      // could be re-enabled, but doesn't feel useful
      // if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      //{
      //   m_TranslateGizmo.SetMovementMode(WTranslateGizmo::MovementMode::MouseDiff);
      // }
    }
    break;

    case WGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      WTransform tNew;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        const WVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        // UE style move along with the moving gizmo
        // could be re-enabled, but doesn't feel useful
        // if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        //{
        //   m_TranslateGizmo.SetMovementMode(WTranslateGizmo::MovementMode::MouseDiff);

        //  auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
        //  if (pFocusedView != nullptr)
        //  {
        //    const WVec3 d = m_TranslateGizmo.GetTranslationDiff();
        //    pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
        //  }
        //}
        // else
        {
          m_TranslateGizmo.SetMovementMode(WTranslateGizmo::MovementMode::ScreenProjection);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<WOrthoGizmoContext>())
      {
        const WOrthoGizmoContext* pOrtho = static_cast<const WOrthoGizmoContext*>(e.m_pGizmo);

        const WVec3 vTranslate = pOrtho->GetTranslationResult();

        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        // UE style move along with the moving gizmo
        // could be re-enabled, but doesn't feel useful
        // if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        //{
        //   // move the camera with the translated object
        //  auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
        //  if (pFocusedView != nullptr)
        //  {
        //    const WVec3 d = pOrtho->GetTranslationDiff();
        //    pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
        //  }
        //}
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void WTranslateGizmoEditTool::OnPreferenceChange(WPreferences* pref)
{
  WScenePreferencesUser* pPref = WDynamicCast<WScenePreferencesUser*>(pref);

  m_TranslateGizmo.SetCameraSpeed(WCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
}

void WTranslateGizmoEditTool::GetGridSettings(WGridSettingsMsgToEngine& ref_msg)
{
  auto pSceneDoc = GetDocument();
  WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(GetDocument());

  // if density != 0, it is enabled at least in ortho mode
  ref_msg.m_fGridDensity = WSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space

  // to be active in perspective mode, tangents have to be non-zero
  ref_msg.m_vGridTangent1.SetZero();
  ref_msg.m_vGridTangent2.SetZero();

  WTranslateGizmo& translateGizmo = m_TranslateGizmo;

  if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
  {
    ref_msg.m_vGridCenter = translateGizmo.GetStartPosition();

    switch (translateGizmo.GetLastHandleInteraction())
    {
      case WTranslateGizmo::HandleInteraction::AxisX:
        if (m_GridPlane == GridPlane::X)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case WTranslateGizmo::HandleInteraction::AxisY:
        if (m_GridPlane == GridPlane::Y)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case WTranslateGizmo::HandleInteraction::AxisZ:
        if (m_GridPlane == GridPlane::Z)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case WTranslateGizmo::HandleInteraction::PlaneX:
        m_GridPlane = GridPlane::X;
        break;
      case WTranslateGizmo::HandleInteraction::PlaneY:
        m_GridPlane = GridPlane::Y;
        break;
      case WTranslateGizmo::HandleInteraction::PlaneZ:
        m_GridPlane = GridPlane::Z;
        break;
      case WTranslateGizmo::HandleInteraction::None:
        break;
    }

    if (pSceneDoc->GetGizmoWorldSpace())
    {
      switch (m_GridPlane)
      {
        case GridPlane::X:
          ref_msg.m_vGridCenter.y = WMath::RoundToMultiple(ref_msg.m_vGridCenter.y, WSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = WMath::RoundToMultiple(ref_msg.m_vGridCenter.z, WSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case GridPlane::Y:
          ref_msg.m_vGridCenter.x = WMath::RoundToMultiple(ref_msg.m_vGridCenter.x, WSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = WMath::RoundToMultiple(ref_msg.m_vGridCenter.z, WSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case GridPlane::Z:
          ref_msg.m_vGridCenter.x = WMath::RoundToMultiple(ref_msg.m_vGridCenter.x, WSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.y = WMath::RoundToMultiple(ref_msg.m_vGridCenter.y, WSnapProvider::GetTranslationSnapValue() * 10);
          break;
      }
    }

    switch (m_GridPlane)
    {
      case GridPlane::X:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * WVec3(0, 1, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * WVec3(0, 0, 1);
        break;
      case GridPlane::Y:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * WVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * WVec3(0, 0, 1);
        break;
      case GridPlane::Z:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * WVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * WVec3(0, 1, 0);
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRotateGizmoEditTool, 1, WRTTIDefaultAllocator<WRotateGizmoEditTool>)
W_END_DYNAMIC_REFLECTED_TYPE;

WRotateGizmoEditTool::WRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

WRotateGizmoEditTool::~WRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void WRotateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_RotateGizmo.SetOwner(GetWindow(), nullptr);
}

void WRotateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_RotateGizmo.SetVisible(visible);
}

void WRotateGizmoEditTool::ApplyGizmoTransformation(const WTransform& transform)
{
  m_RotateGizmo.SetTransformation(transform);
}

void WRotateGizmoEditTool::TransformationGizmoEventHandlerImpl(const WGizmoEvent& e)
{
  WObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (e.m_pGizmo == &m_RotateGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case WGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      WTransform tNew;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const WQuat qRotation = m_RotateGizmo.GetRotationResult();
        const WVec3 vPivot = m_RotateGizmo.GetTransformation().m_vPosition;

        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<WOrthoGizmoContext>())
      {
        const WOrthoGizmoContext* pOrtho = static_cast<const WOrthoGizmoContext*>(e.m_pGizmo);

        const WQuat qRotation = pOrtho->GetRotationResult();

        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void WRotateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_RotateGizmo.UpdateStatusBarText(GetWindow());
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScaleGizmoEditTool, 1, WRTTIDefaultAllocator<WScaleGizmoEditTool>)
W_END_DYNAMIC_REFLECTED_TYPE;

WScaleGizmoEditTool::WScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

WScaleGizmoEditTool::~WScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void WScaleGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_ScaleGizmo.UpdateStatusBarText(GetWindow());
  }
}

void WScaleGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_ScaleGizmo.SetOwner(GetWindow(), nullptr);
}

void WScaleGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_ScaleGizmo.SetVisible(visible);
}

void WScaleGizmoEditTool::ApplyGizmoTransformation(const WTransform& transform)
{
  m_ScaleGizmo.SetTransformation(transform);
}

void WScaleGizmoEditTool::TransformationGizmoEventHandlerImpl(const WGizmoEvent& e)
{
  WObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case WGizmoEvent::Type::Interaction:
    {
      WTransform tNew;

      bool bCancel = false;

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        const WVec3 vScale = m_ScaleGizmo.GetScalingResult();
        if (vScale.x == vScale.y && vScale.x == vScale.z)
        {
          for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            float fNewScale = obj.m_fLocalUniformScaling * vScale.x;

            if (pAccessor->SetValueByName(obj.m_pObject, "LocalUniformScaling", fNewScale).Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
        else
        {
          for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            WVec3 vNewScale = obj.m_vLocalScaling.CompMul(vScale);

            if (pAccessor->SetValueByName(obj.m_pObject, "LocalScaling", vNewScale).Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<WOrthoGizmoContext>())
      {
        const WOrthoGizmoContext* pOrtho = static_cast<const WOrthoGizmoContext*>(e.m_pGizmo);

        const float fScale = pOrtho->GetScalingResult();
        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];
          const float fNewScale = obj.m_fLocalUniformScaling * fScale;

          if (pAccessor->SetValueByName(obj.m_pObject, "LocalUniformScaling", fNewScale).Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (bCancel)
        pAccessor->CancelTransaction();
      else
        pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDragToPositionGizmoEditTool, 1, WRTTIDefaultAllocator<WDragToPositionGizmoEditTool>)
W_END_DYNAMIC_REFLECTED_TYPE;

WDragToPositionGizmoEditTool::WDragToPositionGizmoEditTool()
{

  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

WDragToPositionGizmoEditTool::~WDragToPositionGizmoEditTool()
{
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(WMakeDelegate(&WTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void WDragToPositionGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DragToPosGizmo.UpdateStatusBarText(GetWindow());
  }
}

void WDragToPositionGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_DragToPosGizmo.SetOwner(GetWindow(), nullptr);
}

void WDragToPositionGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_DragToPosGizmo.SetVisible(visible);
}

void WDragToPositionGizmoEditTool::ApplyGizmoTransformation(const WTransform& transform)
{
  m_DragToPosGizmo.SetTransformation(transform);
}

void WDragToPositionGizmoEditTool::TransformationGizmoEventHandlerImpl(const WGizmoEvent& e)
{
  WObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (e.m_pGizmo == &m_DragToPosGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case WGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      WTransform tNew;

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        const WVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();
        const WQuat qRot = m_DragToPosGizmo.GetRotationResult();

        for (WUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (m_DragToPosGizmo.ModifiesRotation())
          {
            tNew.m_qRotation = qRot;
          }

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation | TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}
