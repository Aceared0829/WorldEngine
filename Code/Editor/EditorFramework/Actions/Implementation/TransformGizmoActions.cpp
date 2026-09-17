#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGizmoAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WGizmoAction::WGizmoAction(const WActionContext& context, const char* szName, const WRTTI* pGizmoType)
  : WButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType = pGizmoType;
  m_pGameObjectDocument = static_cast<WGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    WStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName(), ".svg");
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone.svg");
  }

  UpdateState();
}

WGizmoAction::~WGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WGizmoAction::GameObjectEventHandler, this));
}

void WGizmoAction::Execute(const WVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void WGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void WGizmoAction::GameObjectEventHandler(const WGameObjectEvent& e)
{
  if (e.m_Type == WGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

WToggleWorldSpaceGizmo::WToggleWorldSpaceGizmo(const WActionContext& context, const char* szName, const WRTTI* pGizmoType)
  : WGizmoAction(context, szName, pGizmoType)
{
}

void WToggleWorldSpaceGizmo::Execute(const WVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    WGizmoAction::Execute(value);
  }
}

//////////////////////////////////////////////////////////////////////////

class WSnapTranslationMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WSnapTranslationMenuAction, WDynamicMenuAction);

public:
  WSnapTranslationMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    WSnapProvider::s_Events.AddEventHandler(WMakeDelegate(&WSnapTranslationMenuAction::SnapEvent, this));
  }

  ~WSnapTranslationMenuAction()
  {
    WSnapProvider::s_Events.RemoveEventHandler(WMakeDelegate(&WSnapTranslationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const WSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(WDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = WSnapProvider::GetTranslationSnapValue();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.01f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_01");
      e.m_UserValue = 0.01f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.05f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_05");
      e.m_UserValue = 0.05f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_1");
      e.m_UserValue = 0.1f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.2f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_2");
      e.m_UserValue = 0.2f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.25f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_25");
      e.m_UserValue = 0.25f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 0.5f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.0_5");
      e.m_UserValue = 0.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 1.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.1");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 2.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.2");
      e.m_UserValue = 2.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 4.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.4");
      e.m_UserValue = 4.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 5.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.5");
      e.m_UserValue = 5.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 8.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.8");
      e.m_UserValue = 8.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = (fValue == 10.0f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Translate.Snap.10");
      e.m_UserValue = 10.0f;
    }
  }

  virtual void Execute(const WVariant& value) override
  {
    WSnapProvider::SetTranslationSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = WSnapProvider::GetTranslationSnapValue();

    if (fValue == 0.0f)
      SetIconPath(":EditorFramework/Icons/Snap0cm.svg");
    else if (fValue == 0.01f)
      SetIconPath(":EditorFramework/Icons/Snap1cm.svg");
    else if (fValue == 0.05f)
      SetIconPath(":EditorFramework/Icons/Snap5cm.svg");
    else if (fValue == 0.1f)
      SetIconPath(":EditorFramework/Icons/Snap10cm.svg");
    else if (fValue == 0.2f)
      SetIconPath(":EditorFramework/Icons/Snap20cm.svg");
    else if (fValue == 0.25f)
      SetIconPath(":EditorFramework/Icons/Snap25cm.svg");
    else if (fValue == 0.5f)
      SetIconPath(":EditorFramework/Icons/Snap50cm.svg");
    else if (fValue == 1.0f)
      SetIconPath(":EditorFramework/Icons/Snap100cm.svg");
    else if (fValue == 2.0f)
      SetIconPath(":EditorFramework/Icons/Snap200cm.svg");
    else if (fValue == 4.0f)
      SetIconPath(":EditorFramework/Icons/Snap400cm.svg");
    else if (fValue == 5.0f)
      SetIconPath(":EditorFramework/Icons/Snap500cm.svg");
    else if (fValue == 8.0f)
      SetIconPath(":EditorFramework/Icons/Snap800cm.svg");
    else if (fValue == 10.0f)
      SetIconPath(":EditorFramework/Icons/Snap1000cm.svg");

    TriggerUpdate();
  }
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSnapTranslationMenuAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class WSnapRotationMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WSnapRotationMenuAction, WDynamicMenuAction);

public:
  WSnapRotationMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    WSnapProvider::s_Events.AddEventHandler(WMakeDelegate(&WSnapRotationMenuAction::SnapEvent, this));
  }

  ~WSnapRotationMenuAction()
  {
    WSnapProvider::s_Events.RemoveEventHandler(WMakeDelegate(&WSnapRotationMenuAction::SnapEvent, this));
  }

  void SnapEvent(const WSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(WDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = WSnapProvider::GetRotationSnapValue().GetDegree();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 0.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.0_Degree");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 1.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.1_Degree");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 5.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.5_Degree");
      e.m_UserValue = 5.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 10.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.10_Degree");
      e.m_UserValue = 10.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 15.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.15_Degree");
      e.m_UserValue = 15.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 22.5f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.22_5_Degree");
      e.m_UserValue = 22.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 30.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.30_Degree");
      e.m_UserValue = 30.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 45.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.45_Degree");
      e.m_UserValue = 45.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 90.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Rotation.Snap.90_Degree");
      e.m_UserValue = 90.0f;
    }
  }

  virtual void Execute(const WVariant& value) override
  {
    WSnapProvider::SetRotationSnapValue(WAngle::MakeFromDegree(value.Get<float>()));
  };

  void UpdateIcon()
  {
    const float fValue = WSnapProvider::GetRotationSnapValue().GetDegree();

    if (WMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0deg.svg");
    else if (WMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1deg.svg");
    else if (WMath::IsEqual(fValue, 5.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap5deg.svg");
    else if (WMath::IsEqual(fValue, 10.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap10deg.svg");
    else if (WMath::IsEqual(fValue, 15.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap15deg.svg");
    else if (WMath::IsEqual(fValue, 22.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap22deg.svg");
    else if (WMath::IsEqual(fValue, 30.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap30deg.svg");
    else if (WMath::IsEqual(fValue, 45.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap45deg.svg");
    else if (WMath::IsEqual(fValue, 90.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap90deg.svg");

    TriggerUpdate();
  }
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSnapRotationMenuAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

class WSnapScaleMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WSnapScaleMenuAction, WDynamicMenuAction);

public:
  WSnapScaleMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
    UpdateIcon();

    WSnapProvider::s_Events.AddEventHandler(WMakeDelegate(&WSnapScaleMenuAction::SnapEvent, this));
  }

  ~WSnapScaleMenuAction()
  {
    WSnapProvider::s_Events.RemoveEventHandler(WMakeDelegate(&WSnapScaleMenuAction::SnapEvent, this));
  }

  void SnapEvent(const WSnapProviderEvent& e)
  {
    UpdateIcon();
  }

  virtual void GetEntries(WDynamicArray<Item>& out_entries) override
  {
    out_entries.Clear();

    const float fValue = WSnapProvider::GetScaleSnapValue();

    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 0.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.0");
      e.m_UserValue = 0.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 0.125f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.0_125");
      e.m_UserValue = 0.125f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 0.25f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.0_25");
      e.m_UserValue = 0.25f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 0.5f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.0_5");
      e.m_UserValue = 0.5f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 1.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.1");
      e.m_UserValue = 1.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 2.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.2");
      e.m_UserValue = 2.0f;
    }
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_CheckState = WMath::IsEqual(fValue, 4.0f, 0.1f) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
      e.m_sDisplay = WTranslate("Gizmo.Scale.Snap.4");
      e.m_UserValue = 4.0f;
    }
  }

  virtual void Execute(const WVariant& value) override
  {
    WSnapProvider::SetScaleSnapValue(value.Get<float>());
  };

  void UpdateIcon()
  {
    const float fValue = WSnapProvider::GetScaleSnapValue();

    if (WMath::IsEqual(fValue, 0.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap0x.svg");
    else if (WMath::IsEqual(fValue, 0.125f, 0.05f))
      SetIconPath(":EditorFramework/Icons/Snap0125x.svg");
    else if (WMath::IsEqual(fValue, 0.25f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap025x.svg");
    else if (WMath::IsEqual(fValue, 0.5f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap05x.svg");
    else if (WMath::IsEqual(fValue, 1.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap1x.svg");
    else if (WMath::IsEqual(fValue, 2.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap2x.svg");
    else if (WMath::IsEqual(fValue, 4.0f, 0.1f))
      SetIconPath(":EditorFramework/Icons/Snap4x.svg");

    TriggerUpdate();
  }
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSnapScaleMenuAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WTransformGizmoActions::s_hGizmoCategory;
WActionDescriptorHandle WTransformGizmoActions::s_hGizmoMenu;
WActionDescriptorHandle WTransformGizmoActions::s_hNoGizmo;
WActionDescriptorHandle WTransformGizmoActions::s_hTranslateGizmo;
WActionDescriptorHandle WTransformGizmoActions::s_hRotateGizmo;
WActionDescriptorHandle WTransformGizmoActions::s_hScaleGizmo;
WActionDescriptorHandle WTransformGizmoActions::s_hDragToPositionGizmo;
WActionDescriptorHandle WTransformGizmoActions::s_hWorldSpace;
WActionDescriptorHandle WTransformGizmoActions::s_hMoveParentOnly;
WActionDescriptorHandle WTransformGizmoActions::s_SnapSettings;
WActionDescriptorHandle WTransformGizmoActions::s_SnapTranslationMenu;
WActionDescriptorHandle WTransformGizmoActions::s_SnapRotationMenu;
WActionDescriptorHandle WTransformGizmoActions::s_SnapScaleMenu;

void WTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory = W_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu = W_REGISTER_MENU("G.Gizmos");
  s_hNoGizmo = W_REGISTER_ACTION_1("Gizmo.Mode.Select", WActionScope::Document, "Gizmo", "Q", WGizmoAction, nullptr);
  s_hTranslateGizmo = W_REGISTER_ACTION_1(
    "Gizmo.Mode.Translate", WActionScope::Document, "Gizmo", "W", WToggleWorldSpaceGizmo, WGetStaticRTTI<WTranslateGizmoEditTool>());
  s_hRotateGizmo = W_REGISTER_ACTION_1(
    "Gizmo.Mode.Rotate", WActionScope::Document, "Gizmo", "E", WToggleWorldSpaceGizmo, WGetStaticRTTI<WRotateGizmoEditTool>());
  s_hScaleGizmo =
    W_REGISTER_ACTION_1("Gizmo.Mode.Scale", WActionScope::Document, "Gizmo", "R", WGizmoAction, WGetStaticRTTI<WScaleGizmoEditTool>());
  s_hDragToPositionGizmo = W_REGISTER_ACTION_1(
    "Gizmo.Mode.DragToPosition", WActionScope::Document, "Gizmo", "T", WGizmoAction, WGetStaticRTTI<WDragToPositionGizmoEditTool>());
  s_hWorldSpace = W_REGISTER_ACTION_1(
    "Gizmo.TransformSpace", WActionScope::Document, "Gizmo", "X", WTransformGizmoAction, WTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = W_REGISTER_ACTION_1("Gizmo.MoveParentOnly", WActionScope::Document, "Gizmo", "", WTransformGizmoAction,
    WTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings = W_REGISTER_ACTION_1(
    "Gizmo.SnapSettings", WActionScope::Document, "Gizmo", "End", WTransformGizmoAction, WTransformGizmoAction::ActionType::GizmoSnapSettings);

  s_SnapTranslationMenu = W_REGISTER_DYNAMIC_MENU("Gizmo.Translation.Snap.Dropdown", WSnapTranslationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapRotationMenu = W_REGISTER_DYNAMIC_MENU("Gizmo.Rotation.Snap.Dropdown", WSnapRotationMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
  s_SnapScaleMenu = W_REGISTER_DYNAMIC_MENU("Gizmo.Scale.Snap.Dropdown", WSnapScaleMenuAction, ":/EditorFramework/Icons/SnapSettings.svg");
}

void WTransformGizmoActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hGizmoCategory);
  WActionManager::UnregisterAction(s_hGizmoMenu);
  WActionManager::UnregisterAction(s_hNoGizmo);
  WActionManager::UnregisterAction(s_hTranslateGizmo);
  WActionManager::UnregisterAction(s_hRotateGizmo);
  WActionManager::UnregisterAction(s_hScaleGizmo);
  WActionManager::UnregisterAction(s_hDragToPositionGizmo);
  WActionManager::UnregisterAction(s_hWorldSpace);
  WActionManager::UnregisterAction(s_hMoveParentOnly);
  WActionManager::UnregisterAction(s_SnapSettings);
  WActionManager::UnregisterAction(s_SnapTranslationMenu);
  WActionManager::UnregisterAction(s_SnapRotationMenu);
  WActionManager::UnregisterAction(s_SnapScaleMenu);
}

void WTransformGizmoActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const WStringView sTarget = "G.Gizmos";

  pMap->MapAction(s_hGizmoMenu, "G.Edit", 4.0f);
  pMap->MapAction(s_hNoGizmo, sTarget, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sTarget, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sTarget, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sTarget, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sTarget, 4.0f);
  pMap->MapAction(s_hWorldSpace, sTarget, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sTarget, 7.0f);
  pMap->MapAction(s_SnapSettings, sTarget, 8.0f);
}

void WTransformGizmoActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const WStringView sSubPath = "GizmoCategory";

  pMap->MapAction(s_hGizmoCategory, "", 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapTranslationMenu, sSubPath, 7.0f);
  pMap->MapAction(s_SnapRotationMenu, sSubPath, 8.0f);
  pMap->MapAction(s_SnapScaleMenu, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTransformGizmoAction, 0, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

WTransformGizmoAction::WTransformGizmoAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pGameObjectDocument = static_cast<WGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
    case ActionType::GizmoToggleWorldSpace:
      SetIconPath(":/EditorFramework/Icons/WorldSpace.svg");
      break;
    case ActionType::GizmoToggleMoveParentOnly:
      SetIconPath(":/EditorFramework/Icons/TransformParent.svg");
      break;
    case ActionType::GizmoSnapSettings:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/SnapSettings.svg");
      break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

WTransformGizmoAction::~WTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WTransformGizmoAction::GameObjectEventHandler, this));
}

void WTransformGizmoAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    WQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void WTransformGizmoAction::GameObjectEventHandler(const WGameObjectEvent& e)
{
  if (e.m_Type == WGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void WTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    WGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == WEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
        case WEditToolSupportedSpaces::LocalSpaceOnly:
          SetChecked(false);
          break;
        case WEditToolSupportedSpaces::WorldSpaceOnly:
          SetChecked(true);
          break;
        case WEditToolSupportedSpaces::LocalAndWorldSpace:
          SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
          break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    WGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    const bool bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTranslateGizmoAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WTranslateGizmoAction::s_hSnappingValueMenu;
WActionDescriptorHandle WTranslateGizmoAction::s_hSnapPivotToGrid;
WActionDescriptorHandle WTranslateGizmoAction::s_hSnapObjectsToGrid;

void WTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = W_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid = W_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", WActionScope::Document, "Gizmo - Position Snap", "Ctrl+End", WTranslateGizmoAction, WTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid = W_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", WActionScope::Document, "Gizmo - Position Snap", "", WTranslateGizmoAction, WTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void WTranslateGizmoAction::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hSnappingValueMenu);
  WActionManager::UnregisterAction(s_hSnapPivotToGrid);
  WActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void WTranslateGizmoAction::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSnappingValueMenu, "G.Gizmos", 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 1.0f);
}

WTranslateGizmoAction::WTranslateGizmoAction(const WActionContext& context, const char* szName, ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const WGameObjectDocument*>(context.m_pDocument);
  m_Type = type;
}

void WTranslateGizmoAction::Execute(const WVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
