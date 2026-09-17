#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WNonUniformBoxManipulatorAdapter::WNonUniformBoxManipulatorAdapter() = default;
WNonUniformBoxManipulatorAdapter::~WNonUniformBoxManipulatorAdapter() = default;

void WNonUniformBoxManipulatorAdapter::QueryGridSettings(WGridSettingsMsgToEngine& out_gridSettings)
{
  out_gridSettings.m_vGridCenter = m_Gizmo.GetTransformation().m_vPosition;

  // if density != 0, it is enabled at least in ortho mode
  out_gridSettings.m_fGridDensity = WSnapProvider::GetTranslationSnapValue();

  // to be active in perspective mode, tangents have to be non-zero
  out_gridSettings.m_vGridTangent1.SetZero();
  out_gridSettings.m_vGridTangent2.SetZero();
}

void WNonUniformBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);

  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WNonUniformBoxManipulatorAdapter::GizmoEventHandler, this));
}

void WNonUniformBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WNonUniformBoxManipulatorAttribute* pAttr = static_cast<const WNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->HasSixAxis())
  {
    const float fNegX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegXProperty()));
    const float fPosX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosXProperty()));
    const float fNegY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegYProperty()));
    const float fPosY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosYProperty()));
    const float fNegZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegZProperty()));
    const float fPosZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosZProperty()));

    m_Gizmo.SetSize(WVec3(fNegX, fNegY, fNegZ), WVec3(fPosX, fPosY, fPosZ));
  }
  else
  {
    const float fSizeX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeXProperty()));
    const float fSizeY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeYProperty()));
    const float fSizeZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeZProperty()));

    m_Gizmo.SetSize(WVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, WVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, true);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void WNonUniformBoxManipulatorAdapter::GizmoEventHandler(const WGizmoEvent& e)
{
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
      const WNonUniformBoxManipulatorAttribute* pAttr = static_cast<const WNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const WVec3 neg = m_Gizmo.GetNegSize();
      const WVec3 pos = m_Gizmo.GetPosSize();

      if (pAttr->HasSixAxis())
      {
        ChangeProperties(pAttr->GetNegXProperty(), neg.x, pAttr->GetPosXProperty(), pos.x, pAttr->GetNegYProperty(), neg.y, pAttr->GetPosYProperty(),
          pos.y, pAttr->GetNegZProperty(), neg.z, pAttr->GetPosZProperty(), pos.z);
      }
      else
      {
        ChangeProperties(pAttr->GetSizeXProperty(), pos.x * 2, pAttr->GetSizeYProperty(), pos.y * 2, pAttr->GetSizeZProperty(), pos.z * 2);
      }
    }
    break;
  }
}

void WNonUniformBoxManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
