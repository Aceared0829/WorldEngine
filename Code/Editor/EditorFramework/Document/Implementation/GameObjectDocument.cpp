#include <EditorFramework/EditorFrameworkPCH.h>

#include "EditorFramework/Panels/LogPanel/LogPanel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectMetaData, 1, WRTTINoAllocator)
{
  //W_BEGIN_PROPERTIES
  //{
  //  //W_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectDocument, 2, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEvent<const WGameObjectDocumentEvent&> WGameObjectDocument::s_GameObjectDocumentEvents;

WGameObjectDocument::WGameObjectDocument(WStringView sDocumentPath, WDocumentObjectManager* pObjectManager, WAssetDocEngineConnection engineConnectionType)
  : WAssetDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
  using Meta = WObjectMetaData<WUuid, WGameObjectMetaData>;
  m_GameObjectMetaData = W_DEFAULT_NEW(Meta);

  W_ASSERT_DEV(engineConnectionType == WAssetDocEngineConnection::FullObjectMirroring,
    "WGameObjectDocument only supports full mirroring engine connection types. The parameter only exists for interface compatibility.");

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons = true;
  m_CurrentMode.m_bRenderVisualizers = true;
}

WGameObjectDocument::~WGameObjectDocument()
{
  UnsubscribeGameObjectEventHandlers();
  DeallocateEditTools();
}

void WGameObjectDocument::SubscribeGameObjectEventHandlers()
{
  m_SelectionManagerEventHandlerID = GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectDocument::SelectionManagerEventHandler, this));
  m_ObjectPropertyEventHandlerID = GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WGameObjectDocument::ObjectPropertyEventHandler, this));
  m_ObjectStructureEventHandlerID = GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WGameObjectDocument::ObjectStructureEventHandler, this));
  m_ObjectEventHandlerID = GetObjectManager()->m_ObjectEvents.AddEventHandler(WMakeDelegate(&WGameObjectDocument::ObjectEventHandler, this));

  s_GameObjectDocumentEvents.AddEventHandler(WMakeDelegate(&WGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void WGameObjectDocument::UnsubscribeGameObjectEventHandlers()
{
  GetSelectionManager()->m_Events.RemoveEventHandler(m_SelectionManagerEventHandlerID);
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_ObjectPropertyEventHandlerID);
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_ObjectStructureEventHandlerID);
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(m_ObjectEventHandlerID);

  s_GameObjectDocumentEvents.RemoveEventHandler(WMakeDelegate(&WGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void WGameObjectDocument::GameObjectDocumentEventHandler(const WGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    // case WGameObjectDocumentEvent::Type::GameMode_StartingExternal: // the external player doesn't log to the editor panel, so don't need to clear that
    case WGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case WGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      auto pEditorPrefsUser = WPreferences::QueryPreferences<WEditorPreferencesUser>();
      if (pEditorPrefsUser && pEditorPrefsUser->m_bClearEditorLogsOnPlay)
      {
        WQtLogPanel::GetSingleton()->CombinedLog->GetLog()->Clear();

        // on play, the engine log has a lot of activity, so makes sense to clear that first
        WQtLogPanel::GetSingleton()->EngineLog->GetLog()->Clear();

        // but I think we usually want to keep the editor log around
        // WQtLogPanel::GetSingleton()->EditorLog->GetLog()->Clear();
      }
    }
    break;
    default:
      break;
  }
}

WEditorInputContext* WGameObjectDocument::GetEditorInputContextOverride()
{
  if (GetActiveEditTool() && GetActiveEditTool()->GetEditorInputContextOverride() != nullptr)
  {
    return GetActiveEditTool()->GetEditorInputContextOverride();
  }

  return nullptr;
}

void WGameObjectDocument::SetEditToolConfigDelegate(WDelegate<void(WGameObjectEditTool*)> configDelegate)
{
  m_EditToolConfigDelegate = configDelegate;
}

bool WGameObjectDocument::IsActiveEditTool(const WRTTI* pEditToolType) const
{
  if (m_pActiveEditTool == nullptr)
    return pEditToolType == nullptr;

  if (pEditToolType == nullptr)
    return false;

  return m_pActiveEditTool->IsInstanceOf(pEditToolType);
}

void WGameObjectDocument::SetActiveEditTool(const WRTTI* pEditToolType)
{
  WGameObjectEditTool* pEditTool = nullptr;

  if (pEditToolType != nullptr)
  {
    auto it = m_CreatedEditTools.Find(pEditToolType);
    if (it.IsValid())
    {
      pEditTool = it.Value();
    }
    else
    {
      W_ASSERT_DEBUG(m_EditToolConfigDelegate.IsValid(), "Window did not specify a delegate to configure edit tools");

      pEditTool = pEditToolType->GetAllocator()->Allocate<WGameObjectEditTool>();
      m_CreatedEditTools[pEditToolType] = pEditTool;

      m_EditToolConfigDelegate(pEditTool);
    }
  }

  if (m_pActiveEditTool == pEditTool)
  {
    if (m_pActiveEditTool == nullptr)
    {
      // if there is currently no active edit tool, cycle through the manipulators available on the selected object
      WManipulatorManager::GetSingleton()->CycleActiveManipulator(this);
    }

    return;
  }

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(false);

  m_pActiveEditTool = pEditTool;

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(true);

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::SetAddAmbientLight(bool b)
{
  if (m_bAddAmbientLight == b)
    return;

  m_bAddAmbientLight = b;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::AddAmbientLightChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Ambient Light: {}", m_bAddAmbientLight ? "ON" : "OFF"));
}

void WGameObjectDocument::SetPickTransparent(bool b)
{
  if (m_bPickTransparent == b)
    return;

  m_bPickTransparent = b;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::PickTransparentChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Select Transparent: {}", m_bPickTransparent ? "ON" : "OFF"));

  if (m_bPickTransparent == false)
  {
    // make sure no transparent object is currently selected
    GetSelectionManager()->Clear();
  }
}

void WGameObjectDocument::SetActiveParent(WUuid object)
{
  if (m_ActiveParent != object)
  {
    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::ActiveParentFlag);
    }

    m_ActiveParent = object;

    if (auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(m_ActiveParent))
    {
      m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::ActiveParentFlag);
    }
  }
}

void WGameObjectDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Transform in {}", m_bGizmoWorldSpace ? "World Space" : "Object Space"));
}

bool WGameObjectDocument::GetGizmoWorldSpace() const
{
  return m_bGizmoWorldSpace;
}

void WGameObjectDocument::SetGizmoMoveParentOnly(bool bMoveParent)
{
  if (m_bGizmoMoveParentOnly == bMoveParent)
    return;

  m_bGizmoMoveParentOnly = bMoveParent;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Move Parent Only: {}", m_bGizmoMoveParentOnly ? "ON" : "OFF"));
}

void WGameObjectDocument::DetermineNodeName(const WDocumentObject* pObject, const WUuid& prefabGuid, WStringBuilder& out_sResult, QIcon* out_pIcon /*= nullptr*/) const
{
  // tries to find a good name for a node by looking at the attached components and their properties

  bool bHasIcon = false;

  if (prefabGuid.IsValid())
  {
    auto pInfo = WAssetCurator::GetSingleton()->GetSubAsset(prefabGuid);

    if (pInfo)
    {
      WStringBuilder sPath = pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      sPath = sPath.GetFileName();

      out_sResult.Set("Prefab: ", sPath);
    }
    else
      out_sResult = "Prefab: Invalid Asset";
  }

  const bool bHasChildren = pObject->GetTypeAccessor().GetCount("Children") > 0;

  WStringBuilder tmp;

  const WInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
  for (WInt32 i = 0; i < iComponents; i++)
  {
    WVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
    auto pChild = GetObjectManager()->GetObject(value.Get<WUuid>());
    W_ASSERT_DEBUG(pChild->GetTypeAccessor().GetType()->IsDerivedFrom<WComponent>(), "Non-component found in component set.");
    // take the first components name
    if (!bHasIcon && out_pIcon != nullptr)
    {
      bHasIcon = true;

      WColor color = WColor::MakeZero();

      if (auto pCatAttr = pChild->GetTypeAccessor().GetType()->GetAttributeByType<WCategoryAttribute>())
      {
        color = WColorScheme::GetCategoryColor(pCatAttr->GetCategory(), WColorScheme::CategoryColorUsage::SceneTreeIcon);
      }

      WStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pChild->GetTypeAccessor().GetType()->GetTypeName(), ".svg");
      *out_pIcon = WQtUiServices::GetCachedIconResource(sIconName.GetData(), color);
    }

    if (out_sResult.IsEmpty())
    {
      // try to translate the component name, that will typically make it a nice clean name already
      out_sResult = WTranslate(pChild->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));

      // if no translation is available, clean up the component name in a simple way
      if (out_sResult.EndsWith_NoCase("Component"))
        out_sResult.Shrink(0, 9);
      if (out_sResult.StartsWith("W"))
        out_sResult.Shrink(2, 0);

      if (auto pInDev = pChild->GetTypeAccessor().GetType()->GetAttributeByType<WInDevelopmentAttribute>())
      {
        out_sResult.AppendFormat(" [ {} ]", pInDev->GetString());
      }
    }

    if (prefabGuid.IsValid())
      continue;

    const auto& properties = pChild->GetTypeAccessor().GetType()->GetProperties();

    for (auto pProperty : properties)
    {
      const auto type = pProperty->GetSpecificType();

      // search for string properties that also have an asset browser property -> they reference an asset, so this is most likely the most
      // relevant property
      if ((type == WGetStaticRTTI<const char*>() || type == WGetStaticRTTI<WString>() || type == WGetStaticRTTI<WStringView>()) && pProperty->GetAttributeByType<WAssetBrowserAttribute>() != nullptr)
      {
        WStringBuilder sValue;
        if (pProperty->GetCategory() == WPropertyCategory::Member)
        {
          sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<WString>();
        }
        else if (pProperty->GetCategory() == WPropertyCategory::Array)
        {
          const WInt32 iCount = pChild->GetTypeAccessor().GetCount(pProperty->GetPropertyName());
          if (iCount > 0)
          {
            sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName(), 0).ConvertTo<WString>();
          }
        }

        // if the property is a full asset guid reference, convert it to a file name
        if (WConversionUtils::IsStringUuid(sValue))
        {
          const WUuid AssetGuid = WConversionUtils::ConvertStringToUuid(sValue);

          auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

          if (pAsset)
            sValue = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          else
            sValue = "<unknown>";
        }

        // only use the file name for our display
        sValue = sValue.GetFileName();

        if (!sValue.IsEmpty())
          out_sResult.Append(": ", sValue);

        return;
      }
    }
  }

  if (!bHasIcon && out_pIcon)
  {
    *out_pIcon = WQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Object.svg");
  }

  if (!out_sResult.IsEmpty())
    return;

  if (bHasChildren)
    out_sResult = "Group";
  else
    out_sResult = "Object";
}


void WGameObjectDocument::QueryCachedNodeName(const WDocumentObject* pObject, WStringBuilder& out_sResult, WUuid* out_pPrefabGuid, QIcon* out_pIcon /*= nullptr*/) const
{
  auto pMetaScene = m_GameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  auto pMetaDoc = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  const WUuid prefabGuid = pMetaDoc->m_CreateFromPrefab;

  if (out_pPrefabGuid != nullptr)
    *out_pPrefabGuid = prefabGuid;

  out_sResult = pMetaScene->m_CachedNodeName;
  if (out_pIcon)
    *out_pIcon = pMetaScene->m_Icon;
  m_GameObjectMetaData->EndReadMetaData();
  m_DocumentObjectMetaData->EndReadMetaData();

  if (out_sResult.IsEmpty())
  {
    // the cached node name is only determined once
    // after that only a node rename (EditRole) will currently trigger a cache cleaning and thus a reevaluation
    // this is to prevent excessive re-computation of the name, which is quite involved

    QIcon icon;
    DetermineNodeName(pObject, prefabGuid, out_sResult, &icon);
    WString sNodeName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();
    if (!sNodeName.IsEmpty())
    {
      out_sResult = sNodeName;
    }
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaWrite->m_CachedNodeName = out_sResult;
    pMetaWrite->m_Icon = icon;
    m_GameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (out_pIcon != nullptr)
      *out_pIcon = icon;
  }
}


void WGameObjectDocument::GenerateFullDisplayName(const WDocumentObject* pRoot, WStringBuilder& out_sFullPath) const
{
  if (pRoot == nullptr || pRoot == GetObjectManager()->GetRootObject())
    return;

  GenerateFullDisplayName(pRoot->GetParent(), out_sFullPath);

  if (!pRoot->GetType()->IsDerivedFrom<WComponent>())
  {
    WStringBuilder sObjectName;
    QueryCachedNodeName(pRoot, sObjectName);

    out_sFullPath.AppendPath(sObjectName);
  }
}

WTransform WGameObjectDocument::GetGlobalTransform(const WDocumentObject* pObject) const
{
  if (!m_GlobalTransforms.Contains(pObject))
  {
    ComputeGlobalTransform(pObject);
  }

  return WSimdConversion::ToTransform(m_GlobalTransforms[pObject]);
}

void WGameObjectDocument::SetGlobalTransform(const WDocumentObject* pObject, const WTransform& t, WUInt8 uiTransformationChanges) const
{
  WObjectAccessorBase* pAccessor = GetObjectAccessor();
  auto pHistory = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const WDocumentObject* pParent = pObject->GetParent();

  WSimdTransform tLocal;
  WSimdTransform simdT = WSimdConversion::ToTransform(t);

  if (pParent != nullptr)
  {
    if (!m_GlobalTransforms.Contains(pParent))
    {
      ComputeGlobalTransform(pParent);
    }

    WSimdTransform tParent = m_GlobalTransforms[pParent];

    tLocal = WSimdTransform::MakeLocalTransform(tParent, simdT);
  }
  else
  {
    tLocal = simdT;
  }

  WVec3 vLocalPos = WSimdConversion::ToVec3(tLocal.m_Position);
  WVec3 vLocalScale = WSimdConversion::ToVec3(tLocal.m_Scale);
  WQuat qLocalRot = WSimdConversion::ToQuat(tLocal.m_Rotation);
  float fUniformScale = 1.0f;

  if (vLocalScale.x == vLocalScale.y && vLocalScale.x == vLocalScale.z)
  {
    fUniformScale = vLocalScale.x;
    vLocalScale.Set(1.0f);
  }

  // unfortunately when we are dragging an object the 'temporary' transaction is undone every time before the new commands are sent
  // that means the values that we read here, are always the original values before the object was modified at all
  // therefore when the original position and the new position are identical, that means the user dragged the object to the previous
  // position it does NOT mean that there is no change, in fact there is a change, just back to the original value

  // if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>() != vLocalPos)
  if ((uiTransformationChanges & TransformationChanges::Translation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalPosition", vLocalPos).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<WQuat>() != qLocalRot)
  if ((uiTransformationChanges & TransformationChanges::Rotation) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalRotation", qLocalRot).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>() != vLocalScale)
  if ((uiTransformationChanges & TransformationChanges::Scale) != 0)
  {
    pAccessor->SetValueByName(pObject, "LocalScaling", vLocalScale).LogFailure();
    pAccessor->SetValueByName(pObject, "LocalUniformScaling", fUniformScale).LogFailure();
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void WGameObjectDocument::SetGlobalTransformParentOnly(const WDocumentObject* pObject, const WTransform& t, WUInt8 uiTransformationChanges) const
{
  WTempHybridArray<WTransform, 16> childTransforms;
  const auto& children = pObject->GetChildren();

  childTransforms.SetCountUninitialized(children.GetCount());

  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const WDocumentObject* pChild = children[i];
    childTransforms[i] = GetGlobalTransform(pChild);
  }

  SetGlobalTransform(pObject, t, uiTransformationChanges);

  for (WUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const WDocumentObject* pChild = children[i];
    SetGlobalTransform(pChild, childTransforms[i], TransformationChanges::All);
  }
}

void WGameObjectDocument::InvalidateGlobalTransformValue(const WDocumentObject* pObject) const
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}

WResult WGameObjectDocument::ComputeObjectTransformation(const WDocumentObject* pObject, WTransform& out_result) const
{
  const WDocumentObject* pObj = pObject;

  while (pObj && !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
  {
    pObj = pObj->GetParent();
  }

  if (pObj)
  {
    out_result = ComputeGlobalTransform(pObj);
    return W_SUCCESS;
  }
  else
  {
    out_result.SetIdentity();
    return W_FAILURE;
  }
}

bool WGameObjectDocument::GetGizmoMoveParentOnly() const
{
  return m_bGizmoMoveParentOnly;
}

void WGameObjectDocument::DeallocateEditTools()
{
  for (auto it = m_CreatedEditTools.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_CreatedEditTools.Clear();
}

void WGameObjectDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  SubscribeGameObjectEventHandlers();
}


void WGameObjectDocument::AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const
{
  WAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_GameObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void WGameObjectDocument::RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable)
{
  WAssetDocument::RestoreMetaDataAfterLoading(graph, bUndoable);

  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void WGameObjectDocument::TriggerExpandScenegraph() const
{
  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::TriggerExpandScenegraph;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::TriggerShowSelectionInScenegraph() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::TriggerShowSelectionInScenegraph;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::TriggerFocusOnSelection(bool bAllViews) const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  WGameObjectEvent e;
  e.m_Type = bAllViews ? WGameObjectEvent::Type::TriggerFocusOnSelection_All : WGameObjectEvent::Type::TriggerFocusOnSelection_Hovered;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::TriggerSnapPivotToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::TriggerSnapEachObjectToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::SnapCameraToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.GetCount() != 1)
    return;

  WTransform trans;
  if (ComputeObjectTransformation(selection[0], trans).Failed())
    return;

  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const WCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const WVec3 vForward = trans.m_qRotation * WVec3(1, 0, 0);
  const WVec3 vUp = trans.m_qRotation * WVec3(0, 0, 1);

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(trans.m_vPosition, vForward, pCamera->GetFovOrDim(), &vUp);
}


void WGameObjectDocument::MoveCameraHere()
{
  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr)
    return;

  if (ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
    return;

  const WCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const WVec3 vCurPos = pCamera->GetCenterPosition();
  const WVec3 vDirToPos = ctxt.m_pLastPickingResult->m_vPickedPosition - vCurPos;

  // don't move the entire distance, keep some distance to the target position
  WVec3 vPos = vCurPos + 0.9f * vDirToPos;
  WVec3 vCamDir = pCamera->GetCenterDirForwards();
  WVec3 vCamUp = pCamera->GetCenterDirUp();

  // if the projection mode of the view is orthographic, ignore the direction
  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
  {
    const auto& oldCam = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

    vCamDir = oldCam.GetCenterDirForwards();
    vCamUp = oldCam.GetCenterDirUp();

    switch (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective)
    {
      case WSceneViewPerspective::Orthogonal_Front:
        vPos.x = oldCam.GetCenterPosition().x;
        break;
      case WSceneViewPerspective::Orthogonal_Right:
        vPos.y = oldCam.GetCenterPosition().y;
        break;
      case WSceneViewPerspective::Orthogonal_Top:
        vPos.z = oldCam.GetCenterPosition().z;
        break;

      default:
        break;
    }
  }
  else
  {
    // in ortho modes it is fine to move just anywhere, and we often don't pick a real object,
    // because of the wireframe picking

    // however, in perspective modes, don't move, if we haven't picked any real object
    // this happens for example when one picks the sky -> you would end up far away
    if (!ctxt.m_pLastPickingResult->m_PickedComponent.IsValid() && !ctxt.m_pLastPickingResult->m_PickedOther.IsValid())
      return;
  }

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(vPos, vCamDir, pCamera->GetFovOrDim(), &vCamUp);
}

void WGameObjectDocument::ScheduleSendObjectSelection()
{
  m_iResendSelection = 2;
}

void WGameObjectDocument::SendGameWorldToEngine()
{
  SendDocumentOpenMessage(true);
}

void WGameObjectDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::SimulationSpeedChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Simulation Speed: {0}%%", (WInt32)(m_fSimulationSpeed * 100.0f)));
}

void WGameObjectDocument::SetPauseSimulation(bool b)
{
  if (m_bPauseSimulation == b)
    return;

  m_bPauseSimulation = b;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::SimulationSpeedChanged;
  m_GameObjectEvents.Broadcast(e);
}

void WGameObjectDocument::SetRenderSelectionOverlay(bool b)
{
  if (m_CurrentMode.m_bRenderSelectionOverlay == b)
    return;

  m_CurrentMode.m_bRenderSelectionOverlay = b;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::RenderSelectionOverlayChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Selection Overlay: {}", m_CurrentMode.m_bRenderSelectionOverlay ? "ON" : "OFF"));
}


void WGameObjectDocument::SetRenderVisualizers(bool b)
{
  if (m_CurrentMode.m_bRenderVisualizers == b)
    return;

  m_CurrentMode.m_bRenderVisualizers = b;

  WVisualizerManager::GetSingleton()->SetVisualizersActive(GetActiveSubDocument(), m_CurrentMode.m_bRenderVisualizers);

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::RenderVisualizersChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Visualizers: {}", m_CurrentMode.m_bRenderVisualizers ? "ON" : "OFF"));
}

void WGameObjectDocument::SetRenderShapeIcons(bool b)
{
  if (m_CurrentMode.m_bRenderShapeIcons == b)
    return;

  m_CurrentMode.m_bRenderShapeIcons = b;

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::RenderShapeIconsChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(WFmt("Shape Icons: {}", m_CurrentMode.m_bRenderShapeIcons ? "ON" : "OFF"));
}

void WGameObjectDocument::ObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "LocalPosition" || e.m_sProperty == "LocalRotation" || e.m_sProperty == "LocalScaling" ||
      e.m_sProperty == "LocalUniformScaling")
  {
    InvalidateGlobalTransformValue(e.m_pObject);
  }

  if (e.m_sProperty == "Name")
  {
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(e.m_pObject->GetGuid());
    pMetaWrite->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(WGameObjectMetaData::CachedName);
  }
}

void WGameObjectDocument::ObjectStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_pObject && e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
  {
    switch (e.m_EventType)
    {
      case WDocumentObjectStructureEvent::Type::BeforeObjectMoved:
      {
        // make sure the cache is filled with a proper value
        GetGlobalTransform(e.m_pObject);
      }
      break;

      case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      {
        // read cached value, hopefully it was not invalidated in between BeforeObjectMoved and AfterObjectMoved
        WTransform t = GetGlobalTransform(e.m_pObject);

        SetGlobalTransform(e.m_pObject, t, TransformationChanges::All);
      }
      break;

      default:
        break;
    }
  }
  else
  {
    switch (e.m_EventType)
    {
      case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
        if (e.m_sParentProperty == "Components")
        {
          if (e.m_pPreviousParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pPreviousParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(WGameObjectMetaData::CachedName);
          }

          if (e.m_pNewParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pNewParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(WGameObjectMetaData::CachedName);
          }
        }
        break;

      default:
        break;
    }
  }
}


void WGameObjectDocument::ObjectEventHandler(const WDocumentObjectEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      // clean up object meta data upon object destruction, because we can :-P
      if (GetObjectManager()->GetObject(e.m_pObject->GetGuid()) == nullptr)
      {
        // make sure there is no object with this GUID still "added" to the document
        // this can happen if two objects use the same GUID, only one object can be "added" at a time, but multiple objects with the same
        // GUID may exist the same GUID is in use, when a prefab is recreated (updated) and the GUIDs are restored, such that references
        // don't change the object that is being destroyed is typically referenced by a command that was in the redo-queue that got purged

        m_DocumentObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
        m_GameObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
      }
    }
    break;

    default:
      break;
  }
}


void WGameObjectDocument::SelectionManagerEventHandler(const WSelectionManagerEvent& e)
{
  ScheduleSendObjectSelection();

  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  if (pPreferences->m_bExpandSceneTreeOnSelection)
  {
    TriggerShowSelectionInScenegraph();
  }
}

void WGameObjectDocument::SendObjectSelection()
{
  if (m_iResendSelection <= 0)
    return;

  --m_iResendSelection;

  const auto& sel = GetSelectionManager()->GetRuntimeOverrideSelection().IsEmpty() ? GetSelectionManager()->GetSelection() : GetSelectionManager()->GetRuntimeOverrideSelection();

  WObjectSelectionMsgToEngine msg;
  WStringBuilder sTemp;
  WStringBuilder sGuid;

  for (const auto& item : sel)
  {
    WConversionUtils::ToString(item->GetGuid(), sGuid);

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  GetEditorEngineConnection()->SendMessage(&msg);
}
// static
WTransform WGameObjectDocument::QueryLocalTransform(const WDocumentObject* pObject)
{
  const WVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>();
  const WVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>();
  const WQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<WQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return WTransform(vTranslation, qRotation, vScaling * fScaling);
}

// static
WSimdTransform WGameObjectDocument::QueryLocalTransformSimd(const WDocumentObject* pObject)
{
  const WVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<WVec3>();
  const WVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>();
  const WQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<WQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return WSimdTransform(WSimdConversion::ToVec3(vTranslation), WSimdConversion::ToQuat(qRotation), WSimdConversion::ToVec3(vScaling * fScaling));
}


WTransform WGameObjectDocument::ComputeGlobalTransform(const WDocumentObject* pObject) const
{
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != WGetStaticRTTI<WGameObject>())
  {
    m_GlobalTransforms[pObject] = WSimdTransform::MakeIdentity();
    return WTransform::MakeIdentity();
  }

  const WSimdTransform tParent = WSimdConversion::ToTransform(ComputeGlobalTransform(pObject->GetParent()));
  const WSimdTransform tLocal = QueryLocalTransformSimd(pObject);

  WSimdTransform tGlobal = WSimdTransform::MakeGlobalTransform(tParent, tLocal);

  m_GlobalTransforms[pObject] = tGlobal;

  return WSimdConversion::ToTransform(tGlobal);
}

void WGameObjectDocument::ComputeTopLevelSelectedGameObjects(WDeque<WSelectedGameObject>& out_selection)
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  out_selection.Clear();

  auto hType = WGetStaticRTTI<WGameObject>();

  auto pSelMan = GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (WUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    WSelectedGameObject& sgo = out_selection.ExpandAndGetRef();
    sgo.m_pObject = Selection[sel];
    sgo.m_GlobalTransform = GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<WVec3>();
    sgo.m_fLocalUniformScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();
  }
}

void WGameObjectDocument::HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg)
{
  SUPER::HandleEngineMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WDocumentOpenResponseMsgToEditor>())
  {
    ScheduleSendObjectSelection();
  }
}

// the following method is similar to "WAssetCurator::ReplaceAssetReferenceInObject"

void WGameObjectDocument::FindAssetUsages(WStringView sAssetToFind, WDynamicArray<AssetUsage>& out_usages, WUInt32 uiMaxResults) const
{
  out_usages.Clear();
  FindAssetUsagesInternal(sAssetToFind, GetObjectManager()->GetRootObject(), out_usages, uiMaxResults);
}

void WGameObjectDocument::FindAssetUsagesInternal(WStringView sAssetToFind, const WDocumentObject* pObject, WDynamicArray<AssetUsage>& out_usages, WUInt32 uiMaxResults) const
{
  auto pAccessor = GetObjectAccessor();

  const WRTTI* pType = pObject->GetTypeAccessor().GetType();
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (const WAbstractProperty* pProp : properties)
  {
    // Check if this is an asset reference property
    const WAssetBrowserAttribute* pAssetAttr = pProp->GetAttributeByType<WAssetBrowserAttribute>();
    if (pAssetAttr == nullptr)
      continue;

    // Must be string type
    const auto propVarType = pProp->GetSpecificType()->GetVariantType();
    if (propVarType != WVariantType::String && propVarType != WVariantType::StringView)
      continue;

    // Skip temporary properties
    if (pProp->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WVariant value;
          if (pAccessor->GetValue(pObject, pProp, value).Succeeded())
          {
            const WString& sValue = value.Get<WString>();
            if (sValue == sAssetToFind)
            {
              WStringBuilder sFullPath;
              GenerateFullDisplayName(pObject, sFullPath);

              auto& au = out_usages.ExpandAndGetRef();
              au.m_sObjectName = sFullPath;
              au.m_ObjectGuid = pObject->GetGuid();

              if (out_usages.GetCount() >= uiMaxResults)
                return;
            }
          }
        }
      }
      break;

      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WInt32 iCount = pAccessor->GetCount(pObject, pProp);

          for (WInt32 i = 0; i < iCount; ++i)
          {
            WVariant value;
            if (pAccessor->GetValue(pObject, pProp, value, i).Succeeded())
            {
              const WString& sValue = value.Get<WString>();
              if (sValue == sAssetToFind)
              {
                WStringBuilder sFullPath;
                GenerateFullDisplayName(pObject, sFullPath);

                auto& au = out_usages.ExpandAndGetRef();
                au.m_sObjectName = sFullPath;
                au.m_ObjectGuid = pObject->GetGuid();

                if (out_usages.GetCount() >= uiMaxResults)
                  return;
              }
            }
          }
        }
      }
      break;

      case WPropertyCategory::Map:
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
        {
          WDynamicArray<WVariant> keys;
          if (pAccessor->GetKeys(pObject, pProp, keys).Succeeded())
          {
            for (const WVariant& key : keys)
            {
              WVariant value;
              if (pAccessor->GetValue(pObject, pProp, value, key).Succeeded())
              {
                const WString& sValue = value.Get<WString>();
                if (sValue == sAssetToFind)
                {
                  WStringBuilder sFullPath;
                  GenerateFullDisplayName(pObject, sFullPath);

                  auto& au = out_usages.ExpandAndGetRef();
                  au.m_sObjectName = sFullPath;
                  au.m_ObjectGuid = pObject->GetGuid();

                  if (out_usages.GetCount() >= uiMaxResults)
                    return;
                }
              }
            }
          }
        }
      }
      break;

      default:
        break;
    }
  }


  // Process children recursively
  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType() != nullptr &&
        pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    FindAssetUsagesInternal(sAssetToFind, pChild, out_usages, uiMaxResults);

    if (out_usages.GetCount() >= uiMaxResults)
      return;
  }
}
