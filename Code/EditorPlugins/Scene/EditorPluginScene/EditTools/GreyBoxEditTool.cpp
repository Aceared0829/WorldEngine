#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGreyBoxEditTool, 1, WRTTIDefaultAllocator<WGreyBoxEditTool>)
W_END_DYNAMIC_REFLECTED_TYPE;

WGreyBoxEditTool::WGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WGreyBoxEditTool::GizmoEventHandler, this));
}

WGreyBoxEditTool::~WGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.RemoveEventHandler(WMakeDelegate(&WGreyBoxEditTool::GizmoEventHandler, this));
}

WEditorInputContext* WGreyBoxEditTool::GetEditorInputContextOverride()
{
  if (IsActive())
    return &m_DrawBoxGizmo;

  return nullptr;
}

WEditToolSupportedSpaces WGreyBoxEditTool::GetSupportedSpaces() const
{
  return WEditToolSupportedSpaces::WorldSpaceOnly;
}

bool WGreyBoxEditTool::GetSupportsMoveParentOnly() const
{
  return false;
}


void WGreyBoxEditTool::GetGridSettings(WGridSettingsMsgToEngine& ref_msg)
{
  WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(GetDocument());

  ref_msg.m_fGridDensity = WSnapProvider::GetTranslationSnapValue(); // negative density = local space
  ref_msg.m_vGridTangent1.SetZero();
  ref_msg.m_vGridTangent2.SetZero();

  if (pPreferences->GetShowGrid())
  {
    if (m_DrawBoxGizmo.GetCurrentMode() == WDrawBoxGizmo::ManipulateMode::DrawBase)
    {
      ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      ref_msg.m_vGridTangent1 = WVec3(1, 0, 0);
      ref_msg.m_vGridTangent2 = WVec3(0, 1, 0);
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == WDrawBoxGizmo::ManipulateMode::DrawHeight)
    {
      const WVec3 vCamDir = GetWindow()->GetFocusedViewWidget()->m_pViewConfig->m_Camera.GetDirForwards();

      ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      if (WMath::Abs(WVec3(1, 0, 0).Dot(vCamDir)) < WMath::Abs(WVec3(0, 1, 0).Dot(vCamDir)))
      {
        ref_msg.m_vGridTangent1 = WVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = WVec3(0, 0, 1);
      }
      else
      {
        ref_msg.m_vGridTangent1 = WVec3(0, 1, 0);
        ref_msg.m_vGridTangent2 = WVec3(0, 0, 1);
      }
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == WDrawBoxGizmo::ManipulateMode::None)
    {
      if (m_DrawBoxGizmo.GetDisplayGrid())
      {
        ref_msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

        ref_msg.m_vGridTangent1 = WVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = WVec3(0, 1, 0);
      }
    }
  }
}

void WGreyBoxEditTool::UpdateGizmoState()
{
  WManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);

  m_DrawBoxGizmo.SetVisible(IsActive());
  m_DrawBoxGizmo.SetTransformation(WTransform::MakeIdentity());
}

void WGreyBoxEditTool::GameObjectEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::ActiveEditToolChanged:
      UpdateGizmoState();
      break;

    default:
      break;
  }
}

void WGreyBoxEditTool::ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() &&
      !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void WGreyBoxEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WGreyBoxEditTool::GameObjectEventHandler, this));
  WManipulatorManager::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WGreyBoxEditTool::ManipulatorManagerEventHandler, this));

  m_DrawBoxGizmo.SetOwner(GetWindow(), nullptr);
}

void WGreyBoxEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DrawBoxGizmo.UpdateStatusBarText(GetWindow());
  }
}

void WGreyBoxEditTool::GizmoEventHandler(const WGizmoEvent& e)
{
  if (e.m_Type == WGizmoEvent::Type::EndInteractions)
  {
    WVec3 vCenter;
    float negx, posx, negy, posy, negz, posz;
    m_DrawBoxGizmo.GetResult(vCenter, negx, posx, negy, posy, negz, posz);

    auto* pDoc = GetDocument();
    auto* pHistory = pDoc->GetCommandHistory();

    WUuid materialGuid;

    // check if there is a material asset currently selected in the asset browser
    // if so, assign that material to the greybox component
    {
      const WUuid lastSelected = WQtAssetBrowserPanel::GetSingleton()->GetLastSelectedAsset();

      if (lastSelected.IsValid())
      {
        const auto pSubAsset = WAssetCurator::GetSingleton()->GetSubAsset(lastSelected);
        if (pSubAsset && WStringUtils::IsEqual(pSubAsset->m_pAssetInfo->m_Info->GetAssetsDocumentTypeName(), "Material"))
        {
          materialGuid = lastSelected;
        }
      }
    }

    pHistory->StartTransaction("Add Grey-Box");

    WUuid objGuid, compGuid;
    objGuid = WUuid::MakeUuid();
    compGuid = WUuid::MakeUuid();

    {
      WAddObjectCommand cmdAdd;
      cmdAdd.m_NewObjectGuid = objGuid;
      cmdAdd.m_pType = WGetStaticRTTI<WGameObject>();
      cmdAdd.m_sParentProperty = "Children";
      pHistory->AddCommand(cmdAdd).AssertSuccess();
    }
    {
      WSetObjectPropertyCommand cmdPos;
      cmdPos.m_NewValue = vCenter;
      cmdPos.m_Object = objGuid;
      cmdPos.m_sProperty = "LocalPosition";
      pHistory->AddCommand(cmdPos).AssertSuccess();
    }
    {
      WAddObjectCommand cmdComp;
      cmdComp.m_NewObjectGuid = compGuid;
      cmdComp.m_pType = WRTTI::FindTypeByName("WGreyBoxComponent");
      cmdComp.m_sParentProperty = "Components";
      cmdComp.m_Parent = objGuid;
      cmdComp.m_Index = -1;
      pHistory->AddCommand(cmdComp).AssertSuccess();
    }
    if (materialGuid.IsValid())
    {
      WStringBuilder tmp;
      WSetObjectPropertyCommand cmdMat;
      cmdMat.m_NewValue = WConversionUtils::ToString(materialGuid, tmp).GetData();
      cmdMat.m_Object = compGuid;
      cmdMat.m_sProperty = "Material";
      pHistory->AddCommand(cmdMat).AssertSuccess();
    }
    {
      WSetObjectPropertyCommand cmdSize;
      cmdSize.m_Object = compGuid;

      cmdSize.m_NewValue = negx;
      cmdSize.m_sProperty = "SizeNegX";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue = posx;
      cmdSize.m_sProperty = "SizePosX";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue = negy;
      cmdSize.m_sProperty = "SizeNegY";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue = posy;
      cmdSize.m_sProperty = "SizePosY";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue = negz;
      cmdSize.m_sProperty = "SizeNegZ";
      pHistory->AddCommand(cmdSize).AssertSuccess();

      cmdSize.m_NewValue = posz;
      cmdSize.m_sProperty = "SizePosZ";
      pHistory->AddCommand(cmdSize).AssertSuccess();
    }

    pHistory->FinishTransaction();

    pDoc->GetSelectionManager()->SetSelection(pDoc->GetObjectManager()->GetObject(objGuid));
  }
}
