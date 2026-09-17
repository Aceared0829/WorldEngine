#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

/// Delegate object for the scene graph filter model.
///
/// Handles matching game objects against the search text by checking component type names
/// and, when the "ref:" keyword is used, whether any component property references a specific asset GUID.
class WGameObjectFilter
{
public:
  bool Filter(QModelIndex index, const WSearchPatternFilter& filter)
  {
    if (filter.GetSearchText() != m_sLastFilterText)
    {
      ParseFilter(filter.GetSearchText());
    }

    const WQtDocumentTreeModel* pModel = qobject_cast<const WQtDocumentTreeModel*>(index.model());
    if (pModel == nullptr)
      return false;

    const WDocumentObject* pObj = pModel->GetObject(index);
    WObjectAccessorBase* pAcc = pModel->GetDocumentTree()->GetDocument()->GetObjectAccessor();
    WVariant comp;

    const WInt32 iNum = pAcc->GetCountByName(pObj, "Components");
    for (WInt32 i = 0; i < iNum; ++i)
    {
      if (pAcc->GetValueByName(pObj, "Components", comp, i).Failed())
        continue;

      const WDocumentObject* pCompObj = pAcc->GetObject(comp.Get<WUuid>());

      if (m_bGuidSearch)
      {
        if (ComponentReferencesGuid(pCompObj, m_SearchGuid))
          return true;
      }
      else
      {
        if (filter.PassesFilters(pCompObj->GetType()->GetTypeName()))
          return true;
      }
    }

    return false;
  }

private:
  void ParseFilter(const WString& sText)
  {
    m_sLastFilterText = sText;
    m_bGuidSearch = false;

    const char* szRef = WStringUtils::FindSubString_NoCase(sText.GetData(), "ref:");
    if (szRef != nullptr)
    {
      const char* szGuid = szRef + strlen("ref:");
      if (WConversionUtils::IsStringUuid(szGuid))
      {
        m_SearchGuid = WConversionUtils::ConvertStringToUuid(szGuid);
        m_bGuidSearch = true;
      }
    }
  }

  static bool ComponentReferencesGuid(const WDocumentObject* pCompObj, const WUuid& searchGuid)
  {
    const WIReflectedTypeAccessor& acc = pCompObj->GetTypeAccessor();

    WDynamicArray<const WAbstractProperty*> properties;
    acc.GetType()->GetAllProperties(properties);

    for (const WAbstractProperty* pProp : properties)
    {
      if (pProp->GetAttributeByType<WAssetBrowserAttribute>() == nullptr)
        continue;

      const auto propVarType = pProp->GetSpecificType()->GetVariantType();
      if (propVarType != WVariantType::String && propVarType != WVariantType::StringView)
        continue;

      switch (pProp->GetCategory())
      {
        case WPropertyCategory::Member:
        {
          const WVariant val = acc.GetValue(pProp->GetPropertyName());
          if (val.CanConvertTo<WString>())
          {
            const WString sVal = val.ConvertTo<WString>();
            if (WConversionUtils::IsStringUuid(sVal) && WConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
              return true;
          }
          break;
        }
        case WPropertyCategory::Array:
        {
          const WInt32 iCount = acc.GetCount(pProp->GetPropertyName());
          for (WInt32 i = 0; i < iCount; ++i)
          {
            const WVariant val = acc.GetValue(pProp->GetPropertyName(), i);
            if (val.CanConvertTo<WString>())
            {
              const WString sVal = val.ConvertTo<WString>();
              if (WConversionUtils::IsStringUuid(sVal) && WConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
                return true;
            }
          }
          break;
        }
        case WPropertyCategory::Map:
        {
          WDynamicArray<WVariant> keys;
          acc.GetKeys(pProp->GetPropertyName(), keys);
          for (const WVariant& key : keys)
          {
            const WVariant val = acc.GetValue(pProp->GetPropertyName(), key);
            if (val.CanConvertTo<WString>())
            {
              const WString sVal = val.ConvertTo<WString>();
              if (WConversionUtils::IsStringUuid(sVal) && WConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
                return true;
            }
          }
          break;
        }
        default:
          break;
      }
    }
    return false;
  }

  WString m_sLastFilterText;
  bool m_bGuidSearch = false;
  WUuid m_SearchGuid;
};

WQtDocumentTreeView::WQtDocumentTreeView(QWidget* pParent)
  : WQtItemView<QTreeView>(pParent)
{
  setObjectName("WQtDocumentTreeView");
}

WQtDocumentTreeView::WQtDocumentTreeView(QWidget* pParent, WDocument* pDocument, std::unique_ptr<WQtDocumentTreeModel> pModel, WSelectionManager* pSelection)
  : WQtItemView<QTreeView>(pParent)
{
  setObjectName("WQtDocumentTreeView");

  Initialize(pDocument, std::move(pModel), pSelection);
}

void WQtDocumentTreeView::Initialize(WDocument* pDocument, std::unique_ptr<WQtDocumentTreeModel> pModel, WSelectionManager* pSelection)
{
  m_pDocument = pDocument;
  m_pModel = std::move(pModel);
  m_pSelectionManager = pSelection;
  if (m_pSelectionManager == nullptr)
  {
    // If no selection manager is provided, fall back to the default selection.
    m_pSelectionManager = m_pDocument->GetSelectionManager();
  }

  m_pGameObjectFilter = std::make_unique<WGameObjectFilter>();
  m_pFilterModel.reset(new WQtTreeSearchFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());
  m_pFilterModel->SetCustomFilterFunc(WMakeDelegate(&WGameObjectFilter::Filter, m_pGameObjectFilter.get()));

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pFilterModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);
  setUniformRowHeights(true);

  W_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
              SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr,
    "signal/slot connection failed");
  m_pSelectionManager->m_Events.AddEventHandler(WMakeDelegate(&WQtDocumentTreeView::SelectionEventHandler, this));

  WSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject = nullptr;
  e.m_Type = WSelectionManagerEvent::Type::SelectionSet;
  SelectionEventHandler(e);
}

WQtDocumentTreeView::~WQtDocumentTreeView()
{
  m_pSelectionManager->m_Events.RemoveEventHandler(WMakeDelegate(&WQtDocumentTreeView::SelectionEventHandler, this));
}

void WQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  WDeque<const WDocumentObject*> sel;

  foreach (QModelIndex index, selection)
  {
    if (index.isValid() && index.column() == 0)
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const WDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((WSelectionManager*)m_pSelectionManager)->SetSelection(sel);
}

void WQtDocumentTreeView::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case WSelectionManagerEvent::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;

    case WSelectionManagerEvent::Type::SelectionSet:
    case WSelectionManagerEvent::Type::ObjectAdded:
    case WSelectionManagerEvent::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      QModelIndex currentIndex;
      for (const WDocumentObject* pObject : m_pSelectionManager->GetSelection())
      {
        currentIndex = m_pModel->ComputeModelIndex(pObject);
        currentIndex = m_pFilterModel->mapFromSource(currentIndex);

        if (currentIndex.isValid())
          selection.select(currentIndex, currentIndex);
      }
      if (currentIndex.isValid())
      {
        // We need to change the current index as well because the current index can trigger side effects. E.g. deleting the current index row triggers a selection change event.
        selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::SelectCurrent);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;

    case WSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void WQtDocumentTreeView::EnsureLastSelectedItemVisible()
{
  if (m_pSelectionManager->GetSelection().IsEmpty())
    return;

  const WDocumentObject* pObject = m_pSelectionManager->GetSelection().PeekBack();
  W_ASSERT_DEBUG(m_pModel->GetDocumentTree()->GetDocument() == pObject->GetDocumentObjectManager()->GetDocument(), "Selection is from a different document.");

  auto index = m_pModel->ComputeModelIndex(pObject);
  index = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void WQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void WQtDocumentTreeView::SetAllowDeleteObjects(bool bAllow)
{
  m_bAllowDeleteObjects = bAllow;
}

bool WQtDocumentTreeView::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (WQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;

    if (pEvent->type() == QEvent::KeyPress && keyEvent == QKeySequence::Delete)
    {
      if (m_bAllowDeleteObjects)
      {
        m_pDocument->DeleteSelectedObjects();
      }
      pEvent->accept();
      return true;
    }
  }

  return QTreeView::event(pEvent);
}
