#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

WSelectionManager::WSelectionManager(const WDocumentObjectManager* pObjectManager)
{
  auto pStorage = W_DEFAULT_NEW(Storage);
  pStorage->m_pObjectManager = pObjectManager;
  SwapStorage(pStorage);
}

WSelectionManager::~WSelectionManager()
{
  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();
}

void WSelectionManager::TreeEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      RemoveObject(e.m_pObject, true);
      break;
    default:
      return;
  }
}

bool WSelectionManager::RecursiveRemoveFromSelection(const WDocumentObject* pObject)
{
  auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

  bool bRemoved = false;
  if (it.IsValid())
  {
    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);
    bRemoved = true;
  }

  for (const WDocumentObject* pChild : pObject->GetChildren())
  {
    bRemoved = bRemoved || RecursiveRemoveFromSelection(pChild);
  }
  return bRemoved;
}

void WSelectionManager::Clear()
{
  if (!m_pSelectionStorage->m_SelectionList.IsEmpty() || !m_pSelectionStorage->m_SelectionSet.IsEmpty())
  {
    m_pSelectionStorage->m_SelectionList.Clear();
    m_pSelectionStorage->m_SelectionSet.Clear();

    WSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = nullptr;
    e.m_Type = WSelectionManagerEvent::Type::SelectionCleared;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void WSelectionManager::AddObject(const WDocumentObject* pObject)
{
  W_ASSERT_DEBUG(pObject, "Object must be valid");

  if (IsSelected(pObject))
    return;

  W_ASSERT_DEV(pObject->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
  WStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(pObject);
  if (res.LogFailure())
  {
    return;
  }

  m_pSelectionStorage->m_SelectionList.PushBack(pObject);
  m_pSelectionStorage->m_SelectionSet.Insert(pObject->GetGuid());

  WSelectionManagerEvent e;
  e.m_pDocument = GetDocument();
  e.m_pObject = pObject;
  e.m_Type = WSelectionManagerEvent::Type::ObjectAdded;

  m_pSelectionStorage->m_Events.Broadcast(e);
}

void WSelectionManager::RemoveObject(const WDocumentObject* pObject, bool bRecurseChildren)
{
  if (bRecurseChildren)
  {
    // We only want one message for the change in selection so we first everything and then fire
    // SelectionSet instead of multiple ObjectRemoved messages.
    if (RecursiveRemoveFromSelection(pObject))
    {
      WSelectionManagerEvent e;
      e.m_pDocument = GetDocument();
      e.m_pObject = nullptr;
      e.m_Type = WSelectionManagerEvent::Type::SelectionSet;
      m_pSelectionStorage->m_Events.Broadcast(e);
    }
  }
  else
  {
    auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

    if (!it.IsValid())
      return;

    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);

    WSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = pObject;
    e.m_Type = WSelectionManagerEvent::Type::ObjectRemoved;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void WSelectionManager::SetSelection(const WDocumentObject* pSingleObject)
{
  WDeque<const WDocumentObject*> objs;
  objs.PushBack(pSingleObject);
  SetSelection(objs);
}

void WSelectionManager::SetSelection(const WDeque<const WDocumentObject*>& selection)
{
  if (selection.IsEmpty())
  {
    Clear();
    return;
  }

  if (m_pSelectionStorage->m_SelectionList == selection)
    return;

  m_pSelectionStorage->m_SelectionList.Clear();
  m_pSelectionStorage->m_SelectionSet.Clear();

  m_pSelectionStorage->m_SelectionList.Reserve(selection.GetCount());

  for (WUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    // actually == nullptr should never happen, unless we have an error somewhere else
    if (selection[i] != nullptr)
    {
      W_ASSERT_DEV(selection[i]->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
      WStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(selection[i]);
      if (res.LogFailure())
      {
        continue;
      }

      if (!m_pSelectionStorage->m_SelectionSet.Contains(selection[i]->GetGuid()))
      {
        m_pSelectionStorage->m_SelectionList.PushBack(selection[i]);
        m_pSelectionStorage->m_SelectionSet.Insert(selection[i]->GetGuid());
      }
    }
  }

  {
    // Sync selection model.
    WSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = nullptr;
    e.m_Type = WSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void WSelectionManager::RefreshSelection()
{
  WDeque<const WDocumentObject*> selection = m_pSelectionStorage->m_SelectionList;
  Clear();
  SetSelection(selection);
}

void WSelectionManager::ToggleObject(const WDocumentObject* pObject)
{
  if (IsSelected(pObject))
    RemoveObject(pObject);
  else
    AddObject(pObject);
}

void WSelectionManager::SetRuntimeOverrideSelection(const WDeque<const WDocumentObject*>& selection)
{
  if (m_RuntimeOverrideSelection == selection)
    return;

  m_RuntimeOverrideSelection.Clear();
  m_RuntimeOverrideSelection.Reserve(selection.GetCount());

  for (WUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    // actually == nullptr should never happen, unless we have an error somewhere else
    if (selection[i] != nullptr)
    {
      if (!m_RuntimeOverrideSelection.Contains(selection[i]))
      {
        m_RuntimeOverrideSelection.PushBack(selection[i]);
      }
    }
  }

  {
    WSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = nullptr;
    e.m_Type = WSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection;
    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

const WDocumentObject* WSelectionManager::GetCurrentObject() const
{
  return m_pSelectionStorage->m_SelectionList.IsEmpty() ? nullptr : m_pSelectionStorage->m_SelectionList.PeekBack();
}

bool WSelectionManager::IsSelected(const WDocumentObject* pObject) const
{
  return m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid()).IsValid();
}

bool WSelectionManager::IsParentSelected(const WDocumentObject* pObject) const
{
  const WDocumentObject* pParent = pObject->GetParent();

  while (pParent != nullptr)
  {
    if (m_pSelectionStorage->m_SelectionSet.Find(pParent->GetGuid()).IsValid())
      return true;

    pParent = pParent->GetParent();
  }

  return false;
}

const WDocument* WSelectionManager::GetDocument() const
{
  return m_pSelectionStorage->m_pObjectManager->GetDocument();
}

WSharedPtr<WSelectionManager::Storage> WSelectionManager::SwapStorage(WSharedPtr<WSelectionManager::Storage> pNewStorage)
{
  W_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pSelectionStorage;

  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();

  m_pSelectionStorage = pNewStorage;

  m_pSelectionStorage->m_pObjectManager->m_StructureEvents.AddEventHandler(WMakeDelegate(&WSelectionManager::TreeEventHandler, this), m_ObjectStructureUnsubscriber);
  m_pSelectionStorage->m_Events.AddEventHandler([this](const WSelectionManagerEvent& e)
    { m_Events.Broadcast(e); },
    m_EventsUnsubscriber);

  return retVal;
}

struct WObjectHierarchyComparor
{
  using Tree = WHybridArray<const WDocumentObject*, 4>;

  WObjectHierarchyComparor(WArrayPtr<WSelectionEntry> items)
  {
    for (const WSelectionEntry& e : items)
    {
      const WDocumentObject* pObject = e.m_pObject;

      Tree& tree = lookup[pObject];
      while (pObject)
      {
        tree.PushBack(pObject);
        pObject = pObject->GetParent();
      }
      std::reverse(begin(tree), end(tree));
    }
  }

  W_ALWAYS_INLINE bool Less(const WSelectionEntry& lhs, const WSelectionEntry& rhs) const
  {
    const Tree& A = *lookup.GetValue(lhs.m_pObject);
    const Tree& B = *lookup.GetValue(rhs.m_pObject);

    const WUInt32 minSize = WMath::Min(A.GetCount(), B.GetCount());
    for (WUInt32 i = 0; i < minSize; i++)
    {
      // The first element in the loop should always be the root so there is not risk that there is no common parent.
      if (A[i] != B[i])
      {
        // These elements are the first different ones so they share the same parent.
        // We just assume that the hierarchy is integer-based for now.
        return A[i]->GetPropertyIndex().ConvertTo<WUInt32>() < B[i]->GetPropertyIndex().ConvertTo<WUInt32>();
      }
    }

    return A.GetCount() < B.GetCount();
  }

  W_ALWAYS_INLINE bool Equal(const WSelectionEntry& lhs, const WSelectionEntry& rhs) const { return lhs.m_pObject == rhs.m_pObject; }

  WMap<const WDocumentObject*, Tree> lookup;
};

void WSelectionManager::GetTopLevelSelection(WDynamicArray<WSelectionEntry>& out_entries) const
{
  out_entries.Clear();
  out_entries.Reserve(m_pSelectionStorage->m_SelectionList.GetCount());

  WUInt32 order = 0;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!IsParentSelected(pObj))
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_pObject = pObj;
      e.m_uiSelectionOrder = order++;
    }
  }

  WObjectHierarchyComparor c(out_entries);
  out_entries.Sort(c);
}

void WSelectionManager::GetTopLevelSelectionOfType(const WRTTI* pBase, WDynamicArray<WSelectionEntry>& out_entries) const
{
  out_entries.Clear();
  out_entries.Reserve(m_pSelectionStorage->m_SelectionList.GetCount());

  WUInt32 order = 0;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom(pBase))
      continue;

    if (!IsParentSelected(pObj))
    {
      auto& e = out_entries.ExpandAndGetRef();
      e.m_pObject = pObj;
      e.m_uiSelectionOrder = order++;
    }
  }

  WObjectHierarchyComparor c(out_entries);
  out_entries.Sort(c);
}
