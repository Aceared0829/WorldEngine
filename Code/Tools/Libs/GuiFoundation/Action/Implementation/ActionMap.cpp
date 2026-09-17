#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WActionMapDescriptor, WNoBase, 0, WRTTINoAllocator);
//  W_BEGIN_PROPERTIES
//  W_END_PROPERTIES;
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WActionMap public functions
////////////////////////////////////////////////////////////////////////

WActionMap::WActionMap(WStringView sParentMapping)
{
  m_sParentMapping = sParentMapping;
}

WActionMap::~WActionMap() = default;

void WActionMap::MapAction(WActionDescriptorHandle hAction, WStringView sPath, WStringView sSubPath, float fOrder)
{
  TempActionMapDescriptor& desc = m_TempActions.ExpandAndGetRef();
  desc.m_hAction = hAction;
  desc.m_sPath = sPath;
  desc.m_sSubPath = sSubPath;
  desc.m_fOrder = fOrder;
  m_uiEditCounter++;
}

void WActionMap::MapAction(WActionDescriptorHandle hAction, WStringView sPath, float fOrder)
{
  TempActionMapDescriptor& desc = m_TempActions.ExpandAndGetRef();
  desc.m_hAction = hAction;
  desc.m_sPath = sPath;
  desc.m_fOrder = fOrder;
  m_uiEditCounter++;
}

void WActionMap::HideAction(WActionDescriptorHandle hAction, WStringView sPath)
{
  TempActionMapDescriptor& desc = m_TempHiddenActions.ExpandAndGetRef();
  desc.m_hAction = hAction;
  desc.m_sPath = sPath;
  m_uiEditCounter++;
}

void WActionMap::MapActionInternal(WActionDescriptorHandle hAction, WStringView sPath, WStringView sSubPath, float fOrder)
{
  WStringBuilder sFullPath = sPath;

  if (!sPath.IsEmpty() && sPath.FindSubString("/") == nullptr)
  {
    if (SearchPathForAction(sPath, sFullPath).Failed())
    {
      sFullPath = sPath;
    }
  }

  sFullPath.AppendPath(sSubPath);

  MapActionInternal(hAction, sFullPath, fOrder);
}

void WActionMap::MapActionInternal(WActionDescriptorHandle hAction, WStringView sPath, float fOrder)
{
  WStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");
  WActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sCleanPath;
  d.m_fOrder = fOrder;

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    WStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

  W_VERIFY(MapActionInternal(d).IsValid(), "Mapping Failed");
}

WUuid WActionMap::MapActionInternal(const WActionMapDescriptor& desc)
{
  WUuid ParentGUID;
  if (!FindObjectByPath(desc.m_sPath, ParentGUID))
  {
    return WUuid();
  }

  auto it = m_Descriptors.Find(ParentGUID);

  WTreeNode<WActionMapDescriptor>* pParent = nullptr;
  if (it.IsValid())
  {
    pParent = it.Value();
  }

  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    const WActionMapDescriptor* pDesc = GetDescriptor(pParent);
    if (pDesc->m_hAction.GetDescriptor()->m_Type == WActionType::Action)
    {
      WLog::Error("Can't map descriptor '{0}' as its parent is an action itself and thus can't have any children.",
        desc.m_hAction.GetDescriptor()->m_sActionName);
      return WUuid();
    }
  }

  if (GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName) != nullptr)
  {
    WLog::Error("Can't map descriptor as its name is already present: {0}", desc.m_hAction.GetDescriptor()->m_sActionName);
    return WUuid();
  }

  WInt32 iIndex = 0;
  for (iIndex = 0; iIndex < (WInt32)pParent->GetChildren().GetCount(); ++iIndex)
  {
    const WTreeNode<WActionMapDescriptor>* pChild = pParent->GetChildren()[iIndex];
    const WActionMapDescriptor* pDesc = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
      break;
  }

  WTreeNode<WActionMapDescriptor>* pChild = pParent->InsertChild(desc, iIndex);

  m_Descriptors.Insert(pChild->GetGuid(), pChild);

  return pChild->GetGuid();
}


WResult WActionMap::UnmapActionInternal(const WUuid& guid)
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return W_FAILURE;

  WTreeNode<WActionMapDescriptor>* pNode = it.Value();
  if (WTreeNode<WActionMapDescriptor>* pParent = pNode->GetParent())
  {
    pParent->RemoveChild(pNode->GetParentIndex());
  }
  m_Descriptors.Remove(it);
  return W_SUCCESS;
}

WResult WActionMap::UnmapActionInternal(WActionDescriptorHandle hAction, WStringView sPath)
{
  WStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");
  WActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sCleanPath;
  d.m_fOrder = 0.0f; // unused.

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    WStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

  return UnmapActionInternal(d);
}

WResult WActionMap::UnmapActionInternal(const WActionMapDescriptor& desc)
{
  WTreeNode<WActionMapDescriptor>* pParent = nullptr;
  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    WUuid ParentGUID;
    if (!FindObjectByPath(desc.m_sPath, ParentGUID))
      return W_FAILURE;

    auto it = m_Descriptors.Find(ParentGUID);
    if (!it.IsValid())
      return W_FAILURE;

    pParent = it.Value();
  }

  if (auto* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName))
  {
    return UnmapActionInternal(pChild->GetGuid());
  }
  return W_FAILURE;
}

bool WActionMap::FindObjectByPath(WStringView sPath, WUuid& out_guid) const
{
  out_guid = WUuid();
  if (sPath.IsEmpty())
    return true;

  WStringBuilder sPathBuilder(sPath);
  WTempHybridArray<WStringView, 8> parts;
  sPathBuilder.Split(false, parts, "/");

  const WTreeNode<WActionMapDescriptor>* pParent = &m_Root;
  for (const WStringView& name : parts)
  {
    pParent = GetChildByName(pParent, name);
    if (pParent == nullptr)
      return false;
  }

  out_guid = pParent->GetGuid();
  return true;
}

WResult WActionMap::SearchPathForAction(WStringView sUniqueName, WStringBuilder& out_sPath) const
{
  out_sPath.Clear();

  if (FindObjectPathByName(&m_Root, sUniqueName, out_sPath))
  {
    return W_SUCCESS;
  }

  return W_FAILURE;
}

bool WActionMap::FindObjectPathByName(const WTreeNode<WActionMapDescriptor>* pObject, WStringView sName, WStringBuilder& out_sPath) const
{
  WStringView sObjectName;

  if (!pObject->m_Data.m_hAction.IsInvalidated())
  {
    sObjectName = pObject->m_Data.m_hAction.GetDescriptor()->m_sActionName;
  }

  out_sPath.AppendPath(sObjectName);

  if (sObjectName == sName)
    return true;

  for (const WTreeNode<WActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const WActionMapDescriptor& pDesc = pChild->m_Data;

    if (FindObjectPathByName(pChild, sName, out_sPath))
      return true;
  }

  out_sPath.PathParentDirectory();
  return false;
}

const WActionMapDescriptor* WActionMap::GetDescriptor(const WUuid& guid) const
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return nullptr;
  return GetDescriptor(it.Value());
}

const WActionMapDescriptor* WActionMap::GetDescriptor(const WTreeNode<WActionMapDescriptor>* pObject) const
{
  if (pObject == nullptr)
    return nullptr;

  return &pObject->m_Data;
}

const WTreeNode<WActionMapDescriptor>* WActionMap::GetChildByName(const WTreeNode<WActionMapDescriptor>* pObject, WStringView sName) const
{
  for (const WTreeNode<WActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const WActionMapDescriptor& pDesc = pChild->m_Data;
    if (sName.IsEqual_NoCase(pDesc.m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}

const WActionMap::TreeNode* WActionMap::BuildActionTree()
{
  WUInt32 uiCurrentTransitiveEditCounter = 0;
  WTempHybridArray<const WActionMap*, 3> mappings;
  {
    const WActionMap* pCurrent = this;
    while (pCurrent)
    {
      uiCurrentTransitiveEditCounter += pCurrent->m_uiEditCounter;
      mappings.PushBack(pCurrent);
      pCurrent = WActionMapManager::GetActionMap(pCurrent->m_sParentMapping);
    }
  }

  if (uiCurrentTransitiveEditCounter == m_uiTransitiveEditCounterOfRoot)
  {
    return &m_Root;
  }
  m_uiTransitiveEditCounterOfRoot = uiCurrentTransitiveEditCounter;

  m_Root = TreeNode();
  m_Descriptors.Clear();

  for (WInt32 i = (WInt32)mappings.GetCount() - 1; i >= 0; --i)
  {
    const WActionMap* pCurrent = mappings[i];
    for (const TempActionMapDescriptor& desc : pCurrent->m_TempActions)
    {
      if (desc.m_sSubPath.IsEmpty())
        MapActionInternal(desc.m_hAction, desc.m_sPath, desc.m_fOrder);
      else
        MapActionInternal(desc.m_hAction, desc.m_sPath, desc.m_sSubPath, desc.m_fOrder);
    }
  }

  for (WInt32 i = (WInt32)mappings.GetCount() - 1; i >= 0; --i)
  {
    const WActionMap* pCurrent = mappings[i];
    for (const TempActionMapDescriptor& desc : pCurrent->m_TempHiddenActions)
    {
      if (UnmapActionInternal(desc.m_hAction, desc.m_sPath).Failed())
      {
        WLog::Warning("Failed to hide the action at path '{}' as it does not exist", desc.m_sPath);
      }
    }
  }

  return &m_Root;
}
