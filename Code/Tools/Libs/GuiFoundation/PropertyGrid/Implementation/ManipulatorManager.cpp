#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Document/Document.h>

W_IMPLEMENT_SINGLETON(WManipulatorManager);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ManipulatorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WManipulatorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (WManipulatorManager::GetSingleton())
    {
      auto ptr = WManipulatorManager::GetSingleton();
      W_DEFAULT_DELETE(ptr);
    }
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WManipulatorManager::WManipulatorManager()
  : m_SingletonRegistrar(this)
{
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WManipulatorManager::PhantomTypeManagerEventHandler, this));
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WManipulatorManager::DocumentManagerEventHandler, this));
}

WManipulatorManager::~WManipulatorManager()
{
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WManipulatorManager::PhantomTypeManagerEventHandler, this));
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WManipulatorManager::DocumentManagerEventHandler, this));
}

const WManipulatorAttribute* WManipulatorManager::GetActiveManipulator(const WDocument* pDoc, const WHybridArray<WPropertySelection, 8>*& out_pSelection) const
{
  out_pSelection = nullptr;
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    out_pSelection = &(it.Value().m_Selection);

    return it.Value().m_pAttribute;
  }

  return nullptr;
}

void WManipulatorManager::InternalSetActiveManipulator(const WDocument* pDoc, const WManipulatorAttribute* pManipulator, const WArrayPtr<WPropertySelection>& selection, bool bUnhide)
{
  bool existed = false;
  auto it = m_ActiveManipulator.FindOrAdd(pDoc, &existed);

  it.Value().m_pAttribute = pManipulator;
  it.Value().m_Selection = selection;

  if (!existed)
  {
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WManipulatorManager::StructureEventHandler, this));
    pDoc->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WManipulatorManager::SelectionEventHandler, this));
  }

  auto& data = m_ActiveManipulator[pDoc];

  if (bUnhide)
  {
    data.m_bHideManipulators = false;
  }

  WManipulatorManagerEvent e;
  e.m_bHideManipulators = data.m_bHideManipulators;
  e.m_pDocument = pDoc;
  e.m_pManipulator = pManipulator;
  e.m_pSelection = &data.m_Selection;

  m_Events.Broadcast(e);
}


void WManipulatorManager::SetActiveManipulator(const WDocument* pDoc, const WManipulatorAttribute* pManipulator, const WArrayPtr<WPropertySelection>& selection)
{
  InternalSetActiveManipulator(pDoc, pManipulator, selection, true);
}

void WManipulatorManager::ClearActiveManipulator(const WDocument* pDoc)
{
  WTempHybridArray<WPropertySelection, 8> clearSel;

  InternalSetActiveManipulator(pDoc, nullptr, clearSel, false);
}

void WManipulatorManager::HideActiveManipulator(const WDocument* pDoc, bool bHide)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid() && it.Value().m_bHideManipulators != bHide)
  {
    it.Value().m_bHideManipulators = bHide;

    if (bHide)
    {
      WTempHybridArray<WPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void WManipulatorManager::ToggleHideActiveManipulator(const WDocument* pDoc)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    it.Value().m_bHideManipulators = !it.Value().m_bHideManipulators;

    if (it.Value().m_bHideManipulators)
    {
      WTempHybridArray<WPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void WManipulatorManager::CycleActiveManipulator(const WDocument* pDoc)
{
  const WDocumentObject* pCurrentObject = pDoc->GetSelectionManager()->GetCurrentObject();
  if (pCurrentObject == nullptr)
    return;

  W_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != WManipulatorSearchStrategy::None,
    "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  // Collect available manipulator attributes from the last selected object (or its children).
  // Deduplicates by type + primary property so that e.g. multiple WSplineNodeComponent children
  // that share the same WSplineManipulatorAttribute only contribute one entry.
  WTempHybridArray<const WManipulatorAttribute*, 8> available;

  auto collectManipulators = [&](const WDocumentObject* pObj)
  {
    // Walk the full type hierarchy so attributes on base classes are included.
    for (const WRTTI* pRtti = pObj->GetTypeAccessor().GetType(); pRtti != nullptr; pRtti = pRtti->GetParentType())
    {
      for (const auto* pAttr : pRtti->GetAttributes())
      {
        if (!pAttr->GetDynamicRTTI()->IsDerivedFrom<WManipulatorAttribute>())
          continue;

        const WManipulatorAttribute* pManipAttr = static_cast<const WManipulatorAttribute*>(pAttr);

        // Skip duplicates (same type and primary property)
        bool bAlreadyAdded = false;
        for (const auto* pExisting : available)
        {
          if (pExisting->GetDynamicRTTI() == pManipAttr->GetDynamicRTTI() &&
              pExisting->m_sProperty1 == pManipAttr->m_sProperty1)
          {
            bAlreadyAdded = true;
            break;
          }
        }
        if (!bAlreadyAdded)
          available.PushBack(pManipAttr);
      }
    }
  };

  if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::SelectedObject)
  {
    collectManipulators(pCurrentObject);
  }
  else if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (const auto* pChild : pCurrentObject->GetChildren())
      collectManipulators(pChild);
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  if (available.IsEmpty())
    return;

  // Find the index of the currently active manipulator
  const WHybridArray<WPropertySelection, 8>* pSel = nullptr;
  const WManipulatorAttribute* pActive = GetActiveManipulator(pDoc, pSel);

  WInt32 iCurrentIndex = -1;
  if (pActive != nullptr)
  {
    for (WUInt32 i = 0; i < available.GetCount(); ++i)
    {
      if (available[i]->GetDynamicRTTI() == pActive->GetDynamicRTTI() &&
          available[i]->m_sProperty1 == pActive->m_sProperty1)
      {
        iCurrentIndex = static_cast<WInt32>(i);
        break;
      }
    }
  }

  // If the last (or only) manipulator is currently active, clear and stop
  if (iCurrentIndex >= static_cast<WInt32>(available.GetCount()) - 1)
  {
    ClearActiveManipulator(pDoc);
    return;
  }

  const WManipulatorAttribute* pNext = available[iCurrentIndex + 1];

  // Build a selection for pNext by searching through the current editor selection
  WTempHybridArray<WPropertySelection, 8> newSelection;
  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  auto matchesNext = [&](const WManipulatorAttribute* pManip) -> bool
  {
    return pManip->GetDynamicRTTI() == pNext->GetDynamicRTTI() &&
           pManip->m_sProperty1 == pNext->m_sProperty1 && pManip->m_sProperty2 == pNext->m_sProperty2 &&
           pManip->m_sProperty3 == pNext->m_sProperty3 && pManip->m_sProperty4 == pNext->m_sProperty4 &&
           pManip->m_sProperty5 == pNext->m_sProperty5 && pManip->m_sProperty6 == pNext->m_sProperty6;
  };

  // Returns true if pObj (or any of its base types) has a manipulator attribute matching pNext.
  auto hasMatchingManipulator = [&](const WDocumentObject* pObj) -> bool
  {
    for (const WRTTI* pRtti = pObj->GetTypeAccessor().GetType(); pRtti != nullptr; pRtti = pRtti->GetParentType())
    {
      for (const auto* pAttr : pRtti->GetAttributes())
      {
        if (pAttr->IsInstanceOf(pNext->GetDynamicRTTI()) && matchesNext(static_cast<const WManipulatorAttribute*>(pAttr)))
          return true;
      }
    }
    return false;
  };

  if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::SelectedObject)
  {
    for (const auto* pObj : selection)
    {
      if (hasMatchingManipulator(pObj))
        newSelection.ExpandAndGetRef().m_pObject = pObj;
    }
  }
  else if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (const auto* pObj : selection)
    {
      for (const auto* pChild : pObj->GetChildren())
      {
        if (hasMatchingManipulator(pChild))
        {
          newSelection.ExpandAndGetRef().m_pObject = pChild;
          break;
        }
      }
    }
  }
  else
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  InternalSetActiveManipulator(pDoc, pNext, newSelection, true);
}

void WManipulatorManager::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == WDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    auto pDoc = e.m_pObject->GetDocumentObjectManager()->GetDocument();
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        if (sel.m_pObject == e.m_pObject)
        {
          it.Value().m_Selection.RemoveAndCopy(sel);
          InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
          return;
        }
      }
    }
  }

  if (e.m_EventType == WDocumentObjectStructureEvent::Type::BeforeReset)
  {
    auto pDoc = e.m_pDocument;
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        it.Value().m_Selection.RemoveAndCopy(sel);
        InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
      }
    }
  }
}

void WManipulatorManager::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  TransferToCurrentSelection(e.m_pDocument->GetMainDocument());
}

void WManipulatorManager::TransferToCurrentSelection(const WDocument* pDoc)
{
  auto& data = m_ActiveManipulator[pDoc];
  auto pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  if (data.m_bHideManipulators)
    return;

  WTempHybridArray<WPropertySelection, 8> newSelection;

  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  W_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != WManipulatorSearchStrategy::None, "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::SelectedObject)
  {
    for (WUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& OtherAttributes = selection[i]->GetTypeAccessor().GetType()->GetAttributes();

      for (const auto pOtherAttr : OtherAttributes)
      {
        if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
        {
          auto pOtherManip = static_cast<const WManipulatorAttribute*>(pOtherAttr);

          if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
              pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
              pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
          {
            auto& newItem = newSelection.ExpandAndGetRef();
            newItem.m_pObject = selection[i];
          }
        }
      }
    }
  }

  if (pDoc->GetManipulatorSearchStrategy() == WManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (WUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& children = selection[i]->GetChildren();

      for (const auto& child : children)
      {
        const auto& OtherAttributes = child->GetTypeAccessor().GetType()->GetAttributes();

        for (const auto pOtherAttr : OtherAttributes)
        {
          if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
          {
            auto pOtherManip = static_cast<const WManipulatorAttribute*>(pOtherAttr);

            if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
                pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
                pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
            {
              auto& newItem = newSelection.ExpandAndGetRef();
              newItem.m_pObject = child;
            }
          }
        }
      }
    }
  }

  InternalSetActiveManipulator(pDoc, pAttribute, newSelection, false);
}

void WManipulatorManager::PhantomTypeManagerEventHandler(const WPhantomRttiManagerEvent& e)
{
  if (e.m_Type == WPhantomRttiManagerEvent::Type::TypeChanged || e.m_Type == WPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    for (auto it = m_ActiveManipulator.GetIterator(); it.IsValid(); ++it)
    {
      ClearActiveManipulator(it.Key());
    }
  }
}

void WManipulatorManager::DocumentManagerEventHandler(const WDocumentManager::Event& e)
{
  if (e.m_Type == WDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveManipulator(e.m_pDocument);

    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WManipulatorManager::StructureEventHandler, this));
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WManipulatorManager::SelectionEventHandler, this));

    m_ActiveManipulator.Remove(e.m_pDocument);
  }
}
